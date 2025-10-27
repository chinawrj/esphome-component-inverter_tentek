/**
 * @file set_power_service.c
 * @brief Set Power Service Implementation
 */

#include "set_power_service.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "mbedtls/md5.h"

static const char *TAG = "SET_POWER_SVC";

/* API Configuration */
#define API_URL        "http://server-tj.shuoxd.com:8080/v1/manage/setOnGridInverterParam"
#define LOGIN_URL      "http://server-tj.shuoxd.com:8080/v1/user/login"
#define SIGNATURE_KEY  "1f80ca5871919371ea71716cae4841bd"
#define USER_AGENT     "Mozilla/5.0 (iPhone; CPU iPhone OS 18_6_2 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148 Html5Plus/1.0 (Immersed/20) uni-app"

#define MAX_HTTP_OUTPUT_BUFFER 2048

/* Service state */
typedef struct {
    bool initialized;
    bool authenticated;
    char jsessionid[64];
    char email[128];
    char password[128];
    char device_sn[32];
    uint32_t request_timeout_ms;
    uint8_t max_retry_count;
    
    // Statistics
    uint32_t total_requests;
    uint32_t successful_requests;
    uint32_t failed_requests;
    uint32_t session_refreshes;
    
    // FreeRTOS resources
    QueueHandle_t cmd_queue;
    TaskHandle_t task_handle;
    SemaphoreHandle_t state_mutex;
} set_power_service_state_t;

static set_power_service_state_t s_service = {0};
static char g_jsessionid_from_cookie[64] = {0};

/* Forward declarations */
static void service_task(void *pvParameters);
static esp_err_t login_and_get_session(char *jsessionid_out);
static esp_err_t send_set_power_request(int output_power, const char *jsessionid);

/**
 * @brief URL encode a string
 */
static bool url_encode(char *dst, const char *src, size_t dst_size)
{
    const char *hex = "0123456789ABCDEF";
    size_t dst_pos = 0;
    
    while (*src && dst_pos < dst_size - 1) {
        if ((*src >= 'A' && *src <= 'Z') ||
            (*src >= 'a' && *src <= 'z') ||
            (*src >= '0' && *src <= '9') ||
            *src == '-' || *src == '_' || *src == '.' || *src == '~') {
            dst[dst_pos++] = *src;
        } else {
            if (dst_pos + 3 >= dst_size) {
                return false;
            }
            dst[dst_pos++] = '%';
            dst[dst_pos++] = hex[(*src >> 4) & 0x0F];
            dst[dst_pos++] = hex[*src & 0x0F];
        }
        src++;
    }
    
    dst[dst_pos] = '\0';
    return true;
}

/**
 * @brief Calculate MD5 signature
 */
static void calculate_signature(const char *device_sn, int output_power, char *signature)
{
    char encoded_sn[64];
    char sign_string[256];
    unsigned char md5_output[16];
    
    url_encode(encoded_sn, device_sn, sizeof(encoded_sn));
    
    snprintf(sign_string, sizeof(sign_string), 
             "deviceSn=%s&outputPower=%d%s",
             encoded_sn, output_power, SIGNATURE_KEY);
    
    mbedtls_md5_context ctx;
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (unsigned char *)sign_string, strlen(sign_string));
    mbedtls_md5_finish(&ctx, md5_output);
    mbedtls_md5_free(&ctx);
    
    for (int i = 0; i < 16; i++) {
        sprintf(&signature[i * 2], "%02x", md5_output[i]);
    }
    signature[32] = '\0';
}

/**
 * @brief HTTP event handler
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    static int output_len;
    
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
            
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
            
        case HTTP_EVENT_ON_HEADER:
            // Capture Set-Cookie header for JSESSIONID
            if (strcasecmp(evt->header_key, "Set-Cookie") == 0) {
                ESP_LOGD(TAG, "Found Set-Cookie: %s", evt->header_value);
                
                char *jsessionid_start = strstr(evt->header_value, "JSESSIONID=");
                if (jsessionid_start != NULL) {
                    jsessionid_start += strlen("JSESSIONID=");
                    char *jsessionid_end = strchr(jsessionid_start, ';');
                    
                    int len;
                    if (jsessionid_end != NULL) {
                        len = jsessionid_end - jsessionid_start;
                    } else {
                        len = strlen(jsessionid_start);
                    }
                    
                    if (len > 0 && len < sizeof(g_jsessionid_from_cookie)) {
                        strncpy(g_jsessionid_from_cookie, jsessionid_start, len);
                        g_jsessionid_from_cookie[len] = '\0';
                        ESP_LOGI(TAG, "✅ Captured JSESSIONID: %s", g_jsessionid_from_cookie);
                    }
                }
            }
            break;
            
        case HTTP_EVENT_ON_DATA:
            if (output_len == 0 && evt->user_data) {
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            
            if (!esp_http_client_is_chunked_response(evt->client)) {
                if (evt->user_data) {
                    int copy_len = evt->data_len;
                    if (output_len + copy_len >= MAX_HTTP_OUTPUT_BUFFER) {
                        copy_len = MAX_HTTP_OUTPUT_BUFFER - output_len - 1;
                    }
                    if (copy_len > 0) {
                        memcpy((char *)evt->user_data + output_len, evt->data, copy_len);
                        output_len += copy_len;
                    }
                }
            }
            break;
            
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            output_len = 0;
            break;
            
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            output_len = 0;
            break;
            
        default:
            break;
    }
    
    return ESP_OK;
}

/**
 * @brief Login and get JSESSIONID
 */
static esp_err_t login_and_get_session(char *jsessionid_out)
{
    esp_err_t err = ESP_FAIL;
    char password_hash[33];
    char signature[33];
    char post_data[512];
    char response_buffer[MAX_HTTP_OUTPUT_BUFFER];
    
    ESP_LOGI(TAG, "🔐 Logging in with email: %s", s_service.email);
    
    // Calculate MD5 hash of password
    unsigned char md5_output[16];
    mbedtls_md5_context ctx;
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (unsigned char *)s_service.password, strlen(s_service.password));
    mbedtls_md5_finish(&ctx, md5_output);
    mbedtls_md5_free(&ctx);
    
    for (int i = 0; i < 16; i++) {
        sprintf(&password_hash[i * 2], "%02x", md5_output[i]);
    }
    password_hash[32] = '\0';
    
    // Calculate signature for login
    char encoded_email[128];
    url_encode(encoded_email, s_service.email, sizeof(encoded_email));
    
    char sign_string[512];
    snprintf(sign_string, sizeof(sign_string),
             "appVersion=20250822.1&email=%s&password=%s&phoneModel=huawei%%20mate&phoneOs=1%s",
             encoded_email, password_hash, SIGNATURE_KEY);
    
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (unsigned char *)sign_string, strlen(sign_string));
    mbedtls_md5_finish(&ctx, md5_output);
    mbedtls_md5_free(&ctx);
    
    for (int i = 0; i < 16; i++) {
        sprintf(&signature[i * 2], "%02x", md5_output[i]);
    }
    signature[32] = '\0';
    
    // Build POST data
    snprintf(post_data, sizeof(post_data),
             "email=%s&password=%s&appVersion=20250822.1&phoneOs=1&phoneModel=huawei%%20mate&sign=%s",
             s_service.email, password_hash, signature);
    
    // Configure HTTP client with optimized settings
    esp_http_client_config_t config = {
        .url = LOGIN_URL,
        .event_handler = http_event_handler,
        .user_data = response_buffer,
        .timeout_ms = s_service.request_timeout_ms * 2,  // Double timeout for login
        .buffer_size = MAX_HTTP_OUTPUT_BUFFER,
        .keep_alive_enable = true,
        .keep_alive_idle = 5,
        .keep_alive_interval = 5,
        .keep_alive_count = 3,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_header(client, "User-Agent", USER_AGENT);
    esp_http_client_set_header(client, "Accept", "*/*");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    ESP_LOGI(TAG, "Sending login request...");
    
    memset(g_jsessionid_from_cookie, 0, sizeof(g_jsessionid_from_cookie));
    
    err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        
        if (status_code == 200) {
            if (strstr(response_buffer, "\"result\":0") != NULL) {
                if (strlen(g_jsessionid_from_cookie) > 0) {
                    strncpy(jsessionid_out, g_jsessionid_from_cookie, 63);
                    jsessionid_out[63] = '\0';
                    ESP_LOGI(TAG, "✅ Login successful! JSESSIONID: %s", jsessionid_out);
                    
                    // Update service state
                    xSemaphoreTake(s_service.state_mutex, portMAX_DELAY);
                    s_service.authenticated = true;
                    strncpy(s_service.jsessionid, jsessionid_out, sizeof(s_service.jsessionid) - 1);
                    s_service.session_refreshes++;
                    xSemaphoreGive(s_service.state_mutex);
                    
                    err = ESP_OK;
                } else {
                    ESP_LOGE(TAG, "❌ JSESSIONID not captured");
                    err = ESP_FAIL;
                }
            } else {
                ESP_LOGE(TAG, "❌ Login failed: %s", response_buffer);
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "❌ Login HTTP error: status code %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "❌ Login HTTP request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    
    return err;
}

/**
 * @brief Send set power request
 */
static esp_err_t send_set_power_request(int output_power, const char *jsessionid)
{
    esp_err_t err = ESP_FAIL;
    char signature[33];
    char post_data[256];
    char cookie_header[128];
    char time_header[32];
    char response_buffer[MAX_HTTP_OUTPUT_BUFFER];
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t timestamp_ms = (int64_t)tv.tv_sec * 1000LL + (int64_t)tv.tv_usec / 1000LL;
    
    calculate_signature(s_service.device_sn, output_power, signature);
    
    snprintf(post_data, sizeof(post_data), 
             "deviceSn=%s&outputPower=%d",
             s_service.device_sn, output_power);
    
    snprintf(time_header, sizeof(time_header), "%lld", timestamp_ms);
    snprintf(cookie_header, sizeof(cookie_header), "JSESSIONID=%s", jsessionid);
    
    // Configure HTTP client with optimized settings
    esp_http_client_config_t config = {
        .url = API_URL,
        .event_handler = http_event_handler,
        .user_data = response_buffer,
        .timeout_ms = s_service.request_timeout_ms * 2,  // Increase timeout
        .buffer_size = MAX_HTTP_OUTPUT_BUFFER,
        .keep_alive_enable = true,
        .keep_alive_idle = 5,
        .keep_alive_interval = 5,
        .keep_alive_count = 3,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_header(client, "User-Agent", USER_AGENT);
    esp_http_client_set_header(client, "Accept", "*/*");
    esp_http_client_set_header(client, "Accept-Language", "zh");
    esp_http_client_set_header(client, "Accept-Encoding", "gzip, deflate");
    esp_http_client_set_header(client, "Connection", "keep-alive");
    esp_http_client_set_header(client, "time", time_header);
    esp_http_client_set_header(client, "sign", signature);
    esp_http_client_set_header(client, "Cookie", cookie_header);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        
        if (status_code == 200) {
            if (strstr(response_buffer, "\"result\":0") != NULL) {
                ESP_LOGI(TAG, "✅ Success: Power set to %d%%", output_power);
                err = ESP_OK;
            } else if (strstr(response_buffer, "\"result\":2") != NULL) {
                ESP_LOGW(TAG, "⚠️  Device offline");
                err = ESP_OK;
            } else if (strstr(response_buffer, "\"result\":10000") != NULL) {
                ESP_LOGE(TAG, "❌ Session expired (result:10000)");
                err = ESP_ERR_INVALID_STATE;
            } else {
                ESP_LOGE(TAG, "❌ Unknown response: %s", response_buffer);
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "❌ HTTP error: status code %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "❌ HTTP request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    
    // Update statistics
    xSemaphoreTake(s_service.state_mutex, portMAX_DELAY);
    s_service.total_requests++;
    if (err == ESP_OK) {
        s_service.successful_requests++;
    } else {
        s_service.failed_requests++;
    }
    xSemaphoreGive(s_service.state_mutex);
    
    return err;
}

/**
 * @brief Service task main loop
 */
static void service_task(void *pvParameters)
{
    set_power_cmd_t cmd;
    esp_err_t result;
    
    ESP_LOGI(TAG, "Service task started");
    
    // Perform initial authentication on first run
    ESP_LOGI(TAG, "Performing initial authentication...");
    char initial_session[64];
    esp_err_t initial_auth_result = login_and_get_session(initial_session);
    if (initial_auth_result == ESP_OK) {
        ESP_LOGI(TAG, "✅ Initial authentication successful");
    } else {
        ESP_LOGE(TAG, "❌ Initial authentication failed, will retry on first command");
    }
    
    while (1) {
        // Wait for command from queue
        if (xQueueReceive(s_service.cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            
            result = ESP_FAIL;
            
            switch (cmd.cmd_type) {
                case SET_POWER_CMD_SET_OUTPUT:
                    ESP_LOGI(TAG, "Processing SET_OUTPUT command: power=%d%%", cmd.output_power);
                    
                    // Check authentication
                    xSemaphoreTake(s_service.state_mutex, portMAX_DELAY);
                    bool is_auth = s_service.authenticated;
                    char session[64];
                    strncpy(session, s_service.jsessionid, sizeof(session) - 1);
                    xSemaphoreGive(s_service.state_mutex);
                    
                    if (!is_auth) {
                        ESP_LOGW(TAG, "Not authenticated, attempting login...");
                        result = login_and_get_session(session);
                        if (result != ESP_OK) {
                            ESP_LOGE(TAG, "❌ Login failed");
                            break;
                        }
                    }
                    
                    // Send request with retry logic
                    uint8_t retry_count = 0;
                    while (retry_count <= s_service.max_retry_count) {
                        result = send_set_power_request(cmd.output_power, session);
                        
                        // Success or device offline - both are acceptable
                        if (result == ESP_OK) {
                            break;
                        }
                        
                        // Handle session expiry
                        if (result == ESP_ERR_INVALID_STATE) {
                            ESP_LOGW(TAG, "🔄 Session expired, re-logging in...");
                            
                            xSemaphoreTake(s_service.state_mutex, portMAX_DELAY);
                            s_service.authenticated = false;
                            xSemaphoreGive(s_service.state_mutex);
                            
                            result = login_and_get_session(session);
                            if (result != ESP_OK) {
                                ESP_LOGE(TAG, "❌ Re-login failed");
                                break;
                            }
                            ESP_LOGI(TAG, "✅ Re-login successful, retrying request...");
                            continue;  // Retry with new session
                        }
                        
                        // Handle timeout/network errors with retry
                        if (result == ESP_ERR_HTTP_EAGAIN || result == ESP_FAIL) {
                            retry_count++;
                            if (retry_count <= s_service.max_retry_count) {
                                ESP_LOGW(TAG, "⚠️  Request failed, retry %d/%d after 2s...", 
                                        retry_count, s_service.max_retry_count);
                                vTaskDelay(pdMS_TO_TICKS(2000));  // Wait before retry
                            } else {
                                ESP_LOGE(TAG, "❌ Request failed after %d retries", s_service.max_retry_count);
                            }
                        } else {
                            break;  // Other errors, don't retry
                        }
                    }
                    break;
                    
                case SET_POWER_CMD_FORCE_RELOGIN:
                    ESP_LOGI(TAG, "Processing FORCE_RELOGIN command");
                    
                    xSemaphoreTake(s_service.state_mutex, portMAX_DELAY);
                    s_service.authenticated = false;
                    xSemaphoreGive(s_service.state_mutex);
                    
                    result = login_and_get_session(session);
                    break;
                    
                case SET_POWER_CMD_GET_STATUS:
                    ESP_LOGI(TAG, "Processing GET_STATUS command");
                    result = ESP_OK;
                    break;
                    
                default:
                    ESP_LOGE(TAG, "Unknown command type: %d", cmd.cmd_type);
                    result = ESP_ERR_INVALID_ARG;
                    break;
            }
            
            // Signal completion if requested
            if (cmd.response_sem != NULL) {
                if (cmd.result != NULL) {
                    *cmd.result = result;
                }
                xSemaphoreGive((SemaphoreHandle_t)cmd.response_sem);
            }
        }
    }
    
    vTaskDelete(NULL);
}

/* Public API Implementation */

esp_err_t set_power_service_init(const set_power_service_config_t *config)
{
    if (config == NULL || config->email == NULL || config->password == NULL || config->device_sn == NULL) {
        ESP_LOGE(TAG, "Invalid configuration");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (s_service.initialized) {
        ESP_LOGW(TAG, "Service already initialized");
        return ESP_OK;
    }
    
    memset(&s_service, 0, sizeof(s_service));
    
    // Copy configuration
    strncpy(s_service.email, config->email, sizeof(s_service.email) - 1);
    strncpy(s_service.password, config->password, sizeof(s_service.password) - 1);
    strncpy(s_service.device_sn, config->device_sn, sizeof(s_service.device_sn) - 1);
    s_service.request_timeout_ms = config->request_timeout_ms;
    s_service.max_retry_count = config->max_retry_count;
    
    // Create mutex
    s_service.state_mutex = xSemaphoreCreateMutex();
    if (s_service.state_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Create command queue
    s_service.cmd_queue = xQueueCreate(SET_POWER_SERVICE_QUEUE_SIZE, sizeof(set_power_cmd_t));
    if (s_service.cmd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create command queue");
        vSemaphoreDelete(s_service.state_mutex);
        return ESP_ERR_NO_MEM;
    }
    
    // Initial authentication will be performed by service task
    // to avoid stack overflow in app_main context
    s_service.authenticated = false;
    
    // Create service task
    BaseType_t ret = xTaskCreate(
        service_task,
        "set_power_svc",
        SET_POWER_SERVICE_TASK_STACK_SIZE,
        NULL,
        SET_POWER_SERVICE_TASK_PRIORITY,
        &s_service.task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create service task");
        vQueueDelete(s_service.cmd_queue);
        vSemaphoreDelete(s_service.state_mutex);
        return ESP_ERR_NO_MEM;
    }
    
    s_service.initialized = true;
    
    ESP_LOGI(TAG, "✅ Service initialized successfully (authentication will happen in background)");
    
    return ESP_OK;
}

esp_err_t set_power_service_deinit(void)
{
    if (!s_service.initialized) {
        return ESP_OK;
    }
    
    if (s_service.task_handle != NULL) {
        vTaskDelete(s_service.task_handle);
        s_service.task_handle = NULL;
    }
    
    if (s_service.cmd_queue != NULL) {
        vQueueDelete(s_service.cmd_queue);
        s_service.cmd_queue = NULL;
    }
    
    if (s_service.state_mutex != NULL) {
        vSemaphoreDelete(s_service.state_mutex);
        s_service.state_mutex = NULL;
    }
    
    s_service.initialized = false;
    
    ESP_LOGI(TAG, "Service deinitialized");
    
    return ESP_OK;
}

esp_err_t set_power_service_send(const set_power_cmd_t *cmd, uint32_t timeout_ms)
{
    if (!s_service.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (cmd == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    TickType_t ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    if (xQueueSend(s_service.cmd_queue, cmd, ticks) != pdTRUE) {
        ESP_LOGW(TAG, "Command queue full, timeout occurred");
        return ESP_ERR_TIMEOUT;
    }
    
    return ESP_OK;
}

esp_err_t set_power_service_send_sync(set_power_cmd_t *cmd, uint32_t timeout_ms)
{
    if (!s_service.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (cmd == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    if (sem == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    esp_err_t result = ESP_FAIL;
    cmd->response_sem = sem;
    cmd->result = &result;
    
    esp_err_t err = set_power_service_send(cmd, timeout_ms);
    if (err != ESP_OK) {
        vSemaphoreDelete(sem);
        return err;
    }
    
    TickType_t ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    if (xSemaphoreTake(sem, ticks) != pdTRUE) {
        vSemaphoreDelete(sem);
        return ESP_ERR_TIMEOUT;
    }
    
    vSemaphoreDelete(sem);
    
    return result;
}

esp_err_t set_power_service_set_output(int output_power, bool wait_completion)
{
    if (output_power < 0 || output_power > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    
    set_power_cmd_t cmd = {
        .cmd_type = SET_POWER_CMD_SET_OUTPUT,
        .output_power = output_power,
        .response_sem = NULL,
        .result = NULL,
    };
    
    if (wait_completion) {
        return set_power_service_send_sync(&cmd, 30000);
    } else {
        return set_power_service_send(&cmd, 1000);
    }
}

esp_err_t set_power_service_force_relogin(void)
{
    set_power_cmd_t cmd = {
        .cmd_type = SET_POWER_CMD_FORCE_RELOGIN,
        .response_sem = NULL,
        .result = NULL,
    };
    
    return set_power_service_send_sync(&cmd, 30000);
}

esp_err_t set_power_service_get_status(set_power_service_status_t *status)
{
    if (!s_service.initialized || status == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xSemaphoreTake(s_service.state_mutex, portMAX_DELAY);
    
    status->is_authenticated = s_service.authenticated;
    status->total_requests = s_service.total_requests;
    status->successful_requests = s_service.successful_requests;
    status->failed_requests = s_service.failed_requests;
    status->session_refreshes = s_service.session_refreshes;
    strncpy(status->jsessionid, s_service.jsessionid, sizeof(status->jsessionid) - 1);
    
    xSemaphoreGive(s_service.state_mutex);
    
    return ESP_OK;
}

bool set_power_service_is_ready(void)
{
    if (!s_service.initialized) {
        return false;
    }
    
    xSemaphoreTake(s_service.state_mutex, portMAX_DELAY);
    bool is_auth = s_service.authenticated;
    xSemaphoreGive(s_service.state_mutex);
    
    return is_auth;
}
