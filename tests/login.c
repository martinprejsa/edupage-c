#include <assert.h>
#include <edupage.h>
#include <stdio.h>
#include <stdlib.h>

void handle_err(enum EdupageError err, char const * const msg) {
    printf("[ERROR]: %s\n", msg);
    exit(1);
}

int main(void) {
    char* username = getenv("username");
    assert (username != nullptr);

    char* password = getenv("password");
    assert (password != nullptr);

    char* server = getenv("server");
    assert (server != nullptr);

    edupage_set_error_callback(&handle_err); 
    
    struct EdupageClient *client = edupage_client_create(username, password, server);
    
   // assert(client != nullptr);

    edupage_client_destroy(client);

    return 0;
}