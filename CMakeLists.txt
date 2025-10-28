# Inverter Tentek Component - CMakeLists.txt
# 
# This file supports two build modes:
# 1. ESP-IDF standalone project (when IDF_PATH is set)
# 2. ESPHome component (when included by ESPHome build system)

cmake_minimum_required(VERSION 3.16)

# Check if we're building as an ESP-IDF project or ESPHome component
if(DEFINED ENV{IDF_PATH} AND NOT DEFINED ESPHOME_BUILD)
    # ========================================
    # ESP-IDF Standalone Build Mode
    # ========================================
    message(STATUS "Building as ESP-IDF standalone project")
    
    # Include ESP-IDF build system
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    
    # Define project
    project(esp_idf_set_power_example)
    
else()
    # ========================================
    # ESPHome Component Build Mode
    # ========================================
    message(STATUS "Building as ESPHome component")
    
    # Register component directly with proper dependencies
    idf_component_register(
        SRCS 
            "inverter_tentek.cpp"
            "md5_wrapper.cpp"
            "set_power_service.c"
        INCLUDE_DIRS 
            "."
        REQUIRES 
            esp_http_client
    )
    
endif()
