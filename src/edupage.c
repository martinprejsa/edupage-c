#include <edupage.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/header.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

size_t discard_writer(void *buffer, size_t size, size_t nmemb, void *userp) {
    return size*nmemb;
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

struct EdupageClient* edupage_client_create(char const username[static 1], char const password[static 1], char const server[static 1]) {
    if(strlen(username) >= 30) {
        return nullptr;
    }

    if(strlen(password) >= 30) {
        return nullptr;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    CURL* handle = curl_easy_init();

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

    curl_easy_cleanup(handle);

    if (phpsessid == nullptr) {
        free(phpsessid);
        return nullptr;
    }

    struct EdupageClient* client = malloc(sizeof(*client));
    client->server = calloc(sizeof(*client->server), strlen(server));
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