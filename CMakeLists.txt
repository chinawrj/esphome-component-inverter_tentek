# ESP-IDF Set Power Example - CMakeLists.txt
# 
# 这个文件配置ESP-IDF项目的构建系统
# 适用于ESP-IDF v5.0及更高版本

cmake_minimum_required(VERSION 3.16)

# 包含ESP-IDF构建系统
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 定义项目
project(esp_idf_set_power_example)
