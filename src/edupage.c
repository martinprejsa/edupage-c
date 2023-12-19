#include <time.h>
#include <edupage.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/header.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <jansson.h>

static edupage_error_callback error_callback;

struct buffer {
    size_t size;
    char* data;
};

size_t discard_writer(void *buffer, size_t size, size_t nmemb, void *userp) {
    return size*nmemb;
}

size_t buffer_writer(void *buffer, size_t size, size_t nmemb, struct buffer* userp) {
    size_t realsize = size * nmemb;

    userp->data = realloc(userp->data, realsize + userp->size);
    memcpy(userp->data + userp->size, buffer , realsize);
    userp->size += realsize;

    return realsize;
}

char* header_get_phpsessid(char const * const header) {
    char const * const regex_pattern = "PHPSESSID\\=([A-Za-z0-9,-]*)";

    regex_t regex = {0};
    regmatch_t pmatch[2] = {0};
    regoff_t off, len = {0};

    if (regcomp(&regex, regex_pattern, REG_EXTENDED)) {
        return nullptr;
    }

    if(regexec(&regex, header, 2, pmatch, 0)) {
        return nullptr;
    }

    len = pmatch[1].rm_eo - pmatch[1].rm_so;

    char* sessid = calloc(sizeof(char), len+1);
    strncpy(sessid, header + pmatch[1].rm_so, len);
    sessid[len] = '\0';

    regfree(&regex);

    return sessid;
}

void edupage_set_error_callback(edupage_error_callback c) {
    error_callback = c;
}

struct EdupageClient* edupage_client_create(char const username[static 1], char const password[static 1], char const server[static 1]) {
    if(strlen(username) >= 30) {
        error_callback(EDUPAGE_ERR_INVALID_PARAMETER, "username over length limit, must be less than 30");
        return nullptr;
    }

    if(strlen(password) >= 30) {
        error_callback(EDUPAGE_ERR_INVALID_PARAMETER, "password over length limit, must be less than 30");
        return nullptr;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    CURL* handle = curl_easy_init();

    curl_easy_setopt(handle, CURLOPT_VERBOSE, false);

    char buffer[256];
    char* url_format = "https://%s.edupage.org/login/edubarLogin.php";
    sprintf(buffer, url_format, server);

    curl_easy_setopt(handle, CURLOPT_URL, buffer);
    curl_easy_setopt(handle, CURLOPT_POST, true);
    curl_easy_setopt(handle, CURLOPT_HEADER, true);

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, discard_writer);

    char* form = "username=%s&password=%s";
    sprintf(buffer, form, username, password);

    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, buffer);
    
    CURLcode code = curl_easy_perform(handle);
    
    if (code) {
        error_callback(EDUPAGE_ERR_COMMS, "couldn't communicate with server");
        return nullptr;
    }

    struct curl_header *hdr;
    size_t index = 0;
    char *phpsessid;
    do {
        curl_easy_header(handle, "set-cookie", index, CURLH_HEADER, -1, &hdr);
        phpsessid = header_get_phpsessid(hdr->value);
        index++;
    } while(index < hdr->amount && phpsessid == nullptr);

    curl_easy_header(handle, "location", 0, CURLH_HEADER, -1, &hdr);

    if (strcmp(hdr->value, "/user/")) {
        error_callback(EDUPAGE_ERR_AUTH, "failed to authorize, invalid credentials");
        free(phpsessid);
        curl_easy_cleanup(handle);
        return nullptr;
    }

    long http_code = 0;
    curl_easy_getinfo (handle, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code != 302) {
        error_callback(EDUPAGE_ERR_AUTH, "failed to authorize");
        free(phpsessid);
        curl_easy_cleanup(handle);
        return nullptr;
    }

    curl_easy_cleanup(handle);

    if (phpsessid == nullptr) {
        error_callback(EDUPAGE_ERR_AUTH, "failed to authorize, missing PHPSESSID");
        free(phpsessid);
        return nullptr;
    }

    struct EdupageClient* client = malloc(sizeof(*client));
    client->server = calloc(sizeof(*client->server), strlen(server)+1);
    strcpy(client->server, server);

    client->sessid = phpsessid;

    return client;
}

void edupage_client_destroy(struct EdupageClient * client) {
    if (client == nullptr) 
        return;
    curl_global_cleanup();
    free(client->sessid);
    free(client->server);
    free(client);
}

char const * const edupage_client_get_meal(struct EdupageClient const client[static 1], const time_t t) {
    CURL* handle = curl_easy_init();

    char url_buff[256];
    char* url_format = "https://%s.edupage.org/menu/?date=%s";

    char date_buff[20] = {0};
    strftime(date_buff, 20, "%Y%m%d", localtime(&t));

    sprintf(url_buff, url_format, client->server, date_buff);

    curl_easy_setopt(handle, CURLOPT_URL, url_buff);
    curl_easy_setopt(handle, CURLOPT_HEADER, false);

    struct curl_slist *headers = nullptr;
    sprintf(url_buff, "Cookie: PHPSESSID=%s;", client->sessid);

    headers = curl_slist_append(headers, url_buff); 

    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    struct buffer b = {0};

    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, buffer_writer);

    CURLcode code = curl_easy_perform(handle);

    curl_easy_cleanup(handle);
    curl_slist_free_all(headers);

    size_t body_size = sizeof(char) * b.size;
    char * body = calloc(sizeof(char), body_size+1);
    memcpy(body, b.data, body_size);
    body[b.size] = '\0';
    free(b.data);
        
    if (code) {
        error_callback(EDUPAGE_ERR_COMMS, "couldn't communicate with server");
        return nullptr;
    }

    char const * const regex_pattern = "edupageData\\:.(\\{.*\\}),";

    regex_t regex = {0};
    regmatch_t pmatch[2] = {0};
    regoff_t off, len = {0};

    if (regcomp(&regex, regex_pattern, REG_EXTENDED)) {
        free(body);
        error_callback(EDUPAGE_ERR_AUTH, "edupageData not present, maybe expired credentials");
        return nullptr;
    }

    if(regexec(&regex, body, 2, pmatch, 0)) {
        free(body);
        error_callback(EDUPAGE_ERR_AUTH, "failed to execute regex");
        return nullptr;
    }

    len = pmatch[1].rm_eo - pmatch[1].rm_so;

    char* json = calloc(sizeof(char), len+1);
    strncpy(json, body + pmatch[1].rm_so * sizeof(*body), len);
    json[len] = '\0';

    free(body);
    regfree(&regex);

    json_error_t error;
    json_t *root = json_loads(json, 00, &error);

    free(json);

    if(!json_is_object(root)) {
        error_callback(EDUPAGE_ERR_INVALID_RESP, "invalid json, expected object");
        json_decref(root);
        return nullptr;
    }

    json_t *edupage = json_object_get(root, client->server);
    if (edupage == nullptr) {
        error_callback(EDUPAGE_ERR_INVALID_RESP, "invalid json, edupage singleton not found");
        json_decref(root);
        return nullptr;
    }

    json_t *ticket = json_object_get(edupage, "novyListok");
    if (ticket == nullptr) {
        error_callback(EDUPAGE_ERR_INVALID_RESP, "invalid json, ticket not found");
        json_decref(root);
        return nullptr;
    }

    strftime(date_buff, 20, "%Y-%m-%d", localtime(&t));
    json_t *day = json_object_get(ticket, date_buff);
    if (day == nullptr) {
        error_callback(EDUPAGE_ERR_INVALID_PARAMETER, "day not found");
        json_decref(root);
        return nullptr;
    }

    json_t *singleton = json_object_get(day, "2");
    if (singleton == nullptr) {
        error_callback(EDUPAGE_ERR_INVALID_RESP, "invalid json, day singleton not found");
        json_decref(root);
        return nullptr;
    }

    json_t *meal = json_object_get(singleton, "nazov");
    if (meal == nullptr) {
        error_callback(EDUPAGE_ERR_INVALID_PARAMETER, "meal not found");
        json_decref(root);
        return nullptr;
    }

    char const * const name = json_string_value(meal);

    json_decref(root);

    return name;
}