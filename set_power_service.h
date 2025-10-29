/**
 * @file set_power_service.h
 * @brief Set Power Service - Message Queue Based Service for MIC POWER Inverter Control
 * 
 * This service provides a message queue interface for controlling the MIC POWER inverter.
 * It automatically manages JSESSIONID lifecycle and handles re-authentication when needed.
 * 
 * Features:
 * - Automatic JSESSIONID management
 * - Session expiry detection and recovery
 * - Message queue interface for command submission
 * - Thread-safe operation
 * - Configurable retry policies
 * 
 * @note This service requires WiFi to be connected before initialization
 */

#ifndef SET_POWER_SERVICE_H
#define SET_POWER_SERVICE_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Service Configuration */
#define SET_POWER_SERVICE_QUEUE_SIZE        10      // Maximum pending commands
#define SET_POWER_SERVICE_TASK_STACK_SIZE   8192    // Task stack size
#define SET_POWER_SERVICE_TASK_PRIORITY     5       // Task priority

/**
 * @brief Command types for set power service
 */
typedef enum {
    SET_POWER_CMD_SET_OUTPUT,       /*!< Set output power percentage */
    SET_POWER_CMD_FORCE_RELOGIN,    /*!< Force re-authentication */
    SET_POWER_CMD_GET_STATUS,       /*!< Get service status */
} set_power_cmd_type_t;

/**
 * @brief Command message structure
 */
typedef struct {
    set_power_cmd_type_t cmd_type;  /*!< Command type */
    int output_power;                /*!< Output power percentage (0-100) for SET_OUTPUT_POWER */
    void *response_sem;              /*!< Optional: Semaphore to signal completion */
    esp_err_t *result;               /*!< Optional: Pointer to store result */
} set_power_cmd_t;

/**
 * @brief Service status information
 */
typedef struct {
    bool is_authenticated;           /*!< Whether service has valid JSESSIONID */
    uint32_t total_requests;         /*!< Total number of requests sent */
    uint32_t successful_requests;    /*!< Number of successful requests */
    uint32_t failed_requests;        /*!< Number of failed requests */
    uint32_t skipped_requests;       /*!< Number of requests skipped (deduplication) */
    uint32_t session_refreshes;      /*!< Number of times JSESSIONID was refreshed */
    char jsessionid[64];             /*!< Current JSESSIONID (read-only) */
} set_power_service_status_t;

/**
 * @brief Service configuration
 */
typedef struct {
    const char *email;               /*!< User email for authentication */
    const char *password;            /*!< User password for authentication */
    const char *device_sn;           /*!< Device serial number */
    uint32_t request_timeout_ms;     /*!< HTTP request timeout in milliseconds */
    uint8_t max_retry_count;         /*!< Maximum retry count for failed requests */
} set_power_service_config_t;

/**
 * @brief Default service configuration
 */
#define SET_POWER_SERVICE_CONFIG_DEFAULT() {        \
    .email = NULL,                                   \
    .password = NULL,                                \
    .device_sn = NULL,                               \
    .request_timeout_ms = 10000,                     \
    .max_retry_count = 3,                            \
}

/**
 * @brief Initialize set power service
 * 
 * This function initializes the service, performs initial authentication,
 * and starts the background service task.
 * 
 * @param config Service configuration
 * @return 
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid configuration
 *      - ESP_ERR_NO_MEM: Memory allocation failed
 *      - ESP_FAIL: Authentication or initialization failed
 * 
 * @note WiFi must be connected before calling this function
 */
esp_err_t set_power_service_init(const set_power_service_config_t *config);

/**
 * @brief Deinitialize set power service
 * 
 * Stops the service task and frees all resources.
 * 
 * @return ESP_OK on success
 */
esp_err_t set_power_service_deinit(void);

/**
 * @brief Send a command to the service (non-blocking)
 * 
 * This function queues a command for processing by the service task.
 * It returns immediately without waiting for completion.
 * 
 * @param cmd Command to send
 * @param timeout_ms Maximum time to wait for queue space (use portMAX_DELAY to wait indefinitely)
 * @return 
 *      - ESP_OK: Command queued successfully
 *      - ESP_ERR_TIMEOUT: Queue full, timeout occurred
 *      - ESP_ERR_INVALID_STATE: Service not initialized
 * 
 * @note The command is processed asynchronously. Use set_power_service_send_sync() for synchronous operation.
 */
esp_err_t set_power_service_send(const set_power_cmd_t *cmd, uint32_t timeout_ms);

/**
 * @brief Send a command to the service and wait for completion (blocking)
 * 
 * This function queues a command and blocks until the command is processed.
 * 
 * @param cmd Command to send
 * @param timeout_ms Maximum time to wait for command completion
 * @return 
 *      - ESP_OK: Command completed successfully
 *      - ESP_ERR_TIMEOUT: Command processing timeout
 *      - ESP_ERR_INVALID_STATE: Service not initialized
 *      - Other: Error code from command execution
 */
esp_err_t set_power_service_send_sync(set_power_cmd_t *cmd, uint32_t timeout_ms);

/**
 * @brief Set output power (convenience function)
 * 
 * @param output_power Output power percentage (0-100)
 * @param wait_completion If true, wait for command completion; if false, return immediately
 * @return 
 *      - ESP_OK: Success (or command queued if wait_completion=false)
 *      - ESP_ERR_INVALID_ARG: Invalid power value
 *      - Other: Error from service
 */
esp_err_t set_power_service_set_output(int output_power, bool wait_completion);

/**
 * @brief Force service to re-authenticate
 * 
 * This can be used to refresh JSESSIONID proactively.
 * 
 * @return ESP_OK on success
 */
esp_err_t set_power_service_force_relogin(void);

/**
 * @brief Get service status
 * 
 * @param status Pointer to status structure to fill
 * @return ESP_OK on success
 */
esp_err_t set_power_service_get_status(set_power_service_status_t *status);

/**
 * @brief Check if service is ready to accept commands
 * 
 * @return true if service is initialized and authenticated
 */
bool set_power_service_is_ready(void);

/**
 * @brief Get last successfully set power value
 * 
 * Returns the power value that was last successfully confirmed via HTTP response.
 * This is different from the requested power - it only reflects what was actually set.
 * 
 * @return Last successful power (10-100%), or -1 if no successful request yet
 */
int set_power_service_get_last_successful_power(void);

#ifdef __cplusplus
}
#endif

#endif // SET_POWER_SERVICE_H
