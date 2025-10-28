# Main component CMakeLists.txt
# Supports both ESP-IDF standalone and ESPHome builds

# Check if we're building in ESPHome mode
if(DEFINED ESPHOME_BUILD)
    # ========================================
    # ESPHome Build Mode
    # ========================================
    # When used as ESPHome component, the registration is done in parent CMakeLists.txt
    # This file is not used in ESPHome mode
    
else()
    # ========================================
    # ESP-IDF Standalone Build Mode
    # ========================================
    # Full standalone project with example application
    
    idf_component_register(
        SRCS 
            "esp_idf_set_power_example_v2.c"
            "../set_power_service.c"
        INCLUDE_DIRS 
            "."
            ".."
        REQUIRES 
            esp_wifi
            esp_netif
            esp_event
            esp_http_client
            nvs_flash
            mbedtls
            json
    )
    
endif()
