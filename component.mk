# ESPHome component makefile for inverter_tentek

# Add main directory to include paths
COMPONENT_ADD_INCLUDEDIRS := . main
COMPONENT_SRCDIRS := . main

# Add C source files from main/
COMPONENT_OBJS := main/set_power_service.o

# Link with mbedtls for MD5
COMPONENT_REQUIRES := mbedtls esp_http_client
