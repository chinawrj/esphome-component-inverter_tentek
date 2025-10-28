/**
 * @file md5_wrapper.cpp
 * @brief MD5 calculation wrapper implementation
 * 
 * This file implements C-compatible MD5 functions using ESPHome's MD5 component.
 * It bridges the gap between C code (set_power_service.c) and C++ ESPHome API.
 */

#include "md5_wrapper.h"
#include "esphome/components/md5/md5.h"

using namespace esphome::md5;

extern "C" {

int md5_calculate(const uint8_t *input, size_t ilen, uint8_t output[16]) {
    if (input == nullptr || output == nullptr) {
        return -1;  // Invalid parameters
    }
    
    // Create MD5 digest object
    MD5Digest md5;
    
    // Initialize the digest
    md5.init();
    
    // Add input data
    md5.add(input, ilen);
    
    // Calculate the hash
    md5.calculate();
    
    // Get the result as bytes
    md5.get_bytes(output);
    
    return 0;  // Success
}

}  // extern "C"
