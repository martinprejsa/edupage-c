#ifndef EDUPAGE_EDUPAGE_H
#define EDUPAGE_EDUPAGE_H

struct EdupageClient {
    char * server;
    char * sessid;
};

enum EdupageError {
    EDUPAGE_INVALID_PARAMETER,
    EDUPAGE_COMMS_ERROR,
    EDUPAGE_AUTH_ERROR,
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
 * @brief Retrieves edupage canteen.
 * 
 * @param client client handle
 * @param date format YYYYMMdd
 * @return struct EdupageCanteen 
 */
struct EdupageCanteen edupage_client_get_canteen(struct EdupageClient const client[static 1], char const * const date);

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
