#ifndef EDUPAGE_EDUPAGE_H
#define EDUPAGE_EDUPAGE_H

#include <time.h>
struct EdupageClient {
    char * server;
    char * sessid;
};

struct EdupageCanteen {
    char const * const data;
};

enum EdupageError {
    EDUPAGE_ERR_INVALID_PARAMETER,
    EDUPAGE_ERR_COMMS,
    EDUPAGE_ERR_AUTH,
    EDUPAGE_ERR_INVALID_RESP,
};

typedef void (*edupage_error_callback) (enum EdupageError error, char const * const msg);

/**
 * @brief Sets the error handling callback.
 */
void edupage_set_error_callback(edupage_error_callback c);

/**
 * @brief Creates EdupageClient and logs it in. 
 * Use @link edupage_set_error_callback @endlink to handle errors.
 * @param username edupage username
 * @param password edupage password
 * @param server edupage server name
 * @return struct EdupageClient* client handle
 */
struct EdupageClient * edupage_client_create(char const username[static 1], char const password[static 1], char const server[static 1]);

/**
 * @brief Destroys the client and cleans up.
 * 
 * @param client client handle
 */
void edupage_client_destroy(struct EdupageClient * client);

/**
 * @brief Retrieves edupage canteen meal for a specific date.
 * 
 * @param client client handle
 * @param date t date
 * @return struct EdupageCanteen 
 */
char const * const edupage_client_get_meal(struct EdupageClient const client[static 1], const time_t t);

/**
 * @brief Changes order status for a specific day.
 * Use @link edupage_set_error_callback @endlink to handle errors.
 * @param client client handle
 * @param status true for subscribe false for unsubscribe
 * @param date the specific date, format: YYYYMMdd
 * @return true if sucess
 * @return false on error
 */
bool edupage_client_canteen_change_order_status(struct EdupageClient const client[static 1], bool const status, char const * const date);

#endif // EDUPAGE_EDUPAGE_H
