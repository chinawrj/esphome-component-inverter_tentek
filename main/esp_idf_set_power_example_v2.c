/**
 * @file esp_idf_set_power_example_v2.c
 * @brief ESP-IDF example using set_power_service
 * 
 * This example demonstrates how to use the set_power_service
 * with a message queue interface for clean separation of concerns.
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "set_power_service.h"

/* WiFi Configuration */
#define WIFI_MAXIMUM_RETRY  5

static const char *TAG = "APP_MAIN";

/* FreeRTOS event group for WiFi events */
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

/**
 * @brief WiFi event handler
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "Connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief Initialize WiFi station
 */
static esp_err_t wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s", CONFIG_WIFI_SSID);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID:%s", CONFIG_WIFI_SSID);
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        return ESP_FAIL;
    }
}

/**
 * @brief Application main task - Periodically sends set power commands
 * 
 * This task demonstrates how to use the set_power_service from application code.
 * It simply sends periodic commands through the service's message queue.
 */
static void app_periodic_task(void *pvParameters)
{
    int output_power = 100;  // Default power setting: 100%
    int request_count = 0;
    
    ESP_LOGI(TAG, "Periodic task started");
    
    // Wait for service to be ready
    while (!set_power_service_is_ready()) {
        ESP_LOGI(TAG, "Waiting for set_power_service to be ready...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "✅ Service is ready, starting periodic requests");
    
    while (1) {
        request_count++;
        ESP_LOGI(TAG, "📤 Sending request #%d: set power to %d%%", request_count, output_power);
        
        // Option 1: Non-blocking send (fire and forget)
        // esp_err_t err = set_power_service_set_output(output_power, false);
        
        // Option 2: Blocking send (wait for completion)
        esp_err_t err = set_power_service_set_output(output_power, true);
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "✅ Request #%d completed successfully", request_count);
        } else {
            ESP_LOGE(TAG, "❌ Request #%d failed: %s", request_count, esp_err_to_name(err));
        }
        
        // Print service statistics every 5 requests
        if (request_count % 5 == 0) {
            set_power_service_status_t status;
            if (set_power_service_get_status(&status) == ESP_OK) {
                ESP_LOGI(TAG, "📊 Service Statistics:");
                ESP_LOGI(TAG, "   Authenticated: %s", status.is_authenticated ? "Yes" : "No");
                ESP_LOGI(TAG, "   Total Requests: %lu", status.total_requests);
                ESP_LOGI(TAG, "   Successful: %lu", status.successful_requests);
                ESP_LOGI(TAG, "   Failed: %lu", status.failed_requests);
                ESP_LOGI(TAG, "   Session Refreshes: %lu", status.session_refreshes);
            }
        }
        
        // Optional: Adjust power dynamically based on conditions
        // output_power = calculate_optimal_power();
        
        // Wait for next interval
        ESP_LOGI(TAG, "⏳ Waiting %d seconds until next request...", CONFIG_REQUEST_INTERVAL_SEC);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_REQUEST_INTERVAL_SEC * 1000));
    }
    
    vTaskDelete(NULL);
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "MIC POWER Set Inverter Power Example (Service Based)");
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "=================================================");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize WiFi
    ESP_LOGI(TAG, "Initializing WiFi...");
    ret = wifi_init_sta();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed");
        return;
    }
    
    ESP_LOGI(TAG, "✅ WiFi initialized successfully");
    
    // Initialize set_power_service
    ESP_LOGI(TAG, "Initializing set_power_service...");
    
    set_power_service_config_t service_config = {
        .email = CONFIG_USER_EMAIL,
        .password = CONFIG_USER_PASSWORD,
        .device_sn = CONFIG_DEVICE_SN,
        .request_timeout_ms = CONFIG_REQUEST_TIMEOUT_MS,
        .max_retry_count = CONFIG_MAX_RETRY_COUNT,
    };
    
    ret = set_power_service_init(&service_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to initialize set_power_service: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "✅ set_power_service initialized successfully");
    
    // Create application periodic task
    BaseType_t task_created = xTaskCreate(
        app_periodic_task,
        "app_periodic",
        4096,
        NULL,
        5,
        NULL
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create periodic task");
        set_power_service_deinit();
        return;
    }
    
    ESP_LOGI(TAG, "✅ Application task created successfully");
    ESP_LOGI(TAG, "System initialized, monitoring will run every %d seconds", 
             CONFIG_REQUEST_INTERVAL_SEC);
}
