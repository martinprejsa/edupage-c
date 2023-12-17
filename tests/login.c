#include <edupage.h>

int main(void) {
    struct EdupageClient *client = edupage_client_create("", "", "");
    edupage_client_destroy(client);
    return 0;
}