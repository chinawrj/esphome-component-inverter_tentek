# Main component CMakeLists.txt

idf_component_register(
    SRCS 
        "esp_idf_set_power_example_v2.c"
        "set_power_service.c"
    INCLUDE_DIRS "."
    REQUIRES 
        esp_wifi
        esp_netif
        esp_event
        esp_http_client
        nvs_flash
        mbedtls
        json
)
