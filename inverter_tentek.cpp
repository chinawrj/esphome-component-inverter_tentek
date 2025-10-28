/**
 * @file inverter_tentek.cpp
 * @brief ESPHome Component Implementation for Tentek/MIC POWER Inverter Control
 * 
 * @author GitHub Copilot
 * @date 2025-10-27
 */

#include "inverter_tentek.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace inverter_tentek {

static const char *const TAG = "inverter_tentek";

void InverterTentekComponent::set_output_power(int power) {
  if (power < 0 || power > 100) {
    ESP_LOGW(TAG, "Invalid power value %d, must be 0-100", power);
    return;
  }

  if (power == output_power_) {
    ESP_LOGD(TAG, "Power already set to %d%%, ignoring duplicate request", power);
    return;
  }

  output_power_ = power;
  
  if (!service_initialized_) {
    ESP_LOGI(TAG, "Service not initialized yet, power will be set to %d%% after initialization", power);
    return;
  }

  ESP_LOGI(TAG, "Setting output power to %d%%...", power);
  
  // Send command through service (non-blocking)
  esp_err_t err = set_power_service_set_output(power, false);
  
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "✅ Power command queued successfully");
  } else {
    ESP_LOGE(TAG, "❌ Failed to queue power command: %s", esp_err_to_name(err));
  }
}

void InverterTentekComponent::setup() {
  ESP_LOGI(TAG, "🔧 Setting up Inverter Tentek Component...");
  
  // Validate configuration
  if (email_.empty() || password_.empty() || device_sn_.empty()) {
    ESP_LOGE(TAG, "❌ Invalid configuration: email, password, and device_sn are required");
    this->mark_failed();
    return;
  }
  
  ESP_LOGI(TAG, "Configuration:");
  ESP_LOGI(TAG, "  ├─ Email: %s", email_.c_str());
  ESP_LOGI(TAG, "  ├─ Device SN: %s", device_sn_.c_str());
  ESP_LOGI(TAG, "  ├─ Output Power: %d%%", output_power_);
  ESP_LOGI(TAG, "  ├─ Request Timeout: %u ms", request_timeout_ms_);
  ESP_LOGI(TAG, "  └─ Max Retry Count: %u", max_retry_count_);
  
  // Wait for WiFi to be connected (ESPHome handles WiFi)
  // The component's setup_priority is AFTER_WIFI, so WiFi should be ready
  
  // Initialize set_power_service
  ESP_LOGI(TAG, "Initializing set_power_service...");
  
  set_power_service_config_t service_config = {
      .email = email_.c_str(),
      .password = password_.c_str(),
      .device_sn = device_sn_.c_str(),
      .request_timeout_ms = request_timeout_ms_,
      .max_retry_count = max_retry_count_,
  };
  
  esp_err_t err = set_power_service_init(&service_config);
  
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "❌ Failed to initialize set_power_service: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }
  
  service_initialized_ = true;
  
  ESP_LOGI(TAG, "✅ set_power_service initialized successfully");
  ESP_LOGI(TAG, "   (Initial authentication will happen in background)");
  
  // Set initial power output (non-blocking, will be queued)
  if (output_power_ != 100) {
    ESP_LOGI(TAG, "Setting initial power to %d%%...", output_power_);
    set_power_service_set_output(output_power_, false);
  }
  
  ESP_LOGI(TAG, "✅ Inverter Tentek Component initialized successfully");
}

void InverterTentekComponent::loop() {
  if (!service_initialized_) {
    return;
  }
  
  // Periodic status logging (every 30 seconds)
  uint32_t current_time = millis();
  if (current_time - last_status_log_time_ > 30000) {
    last_status_log_time_ = current_time;
    
    set_power_service_status_t status;
    if (set_power_service_get_status(&status) == ESP_OK) {
      ESP_LOGI(TAG, "📊 Service Statistics:");
      ESP_LOGI(TAG, "   ├─ Authenticated: %s", status.is_authenticated ? "Yes" : "No");
      ESP_LOGI(TAG, "   ├─ Current Power Setting: %d%%", output_power_);
      ESP_LOGI(TAG, "   ├─ Total Requests: %lu", status.total_requests);
      ESP_LOGI(TAG, "   ├─ Successful: %lu", status.successful_requests);
      ESP_LOGI(TAG, "   ├─ Failed: %lu", status.failed_requests);
      ESP_LOGI(TAG, "   └─ Session Refreshes: %lu", status.session_refreshes);
    }
  }
}

void InverterTentekComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Inverter Tentek Component:");
  ESP_LOGCONFIG(TAG, "  Email: %s", email_.c_str());
  ESP_LOGCONFIG(TAG, "  Device SN: %s", device_sn_.c_str());
  ESP_LOGCONFIG(TAG, "  Output Power: %d%%", output_power_);
  ESP_LOGCONFIG(TAG, "  Request Timeout: %u ms", request_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Max Retry Count: %u", max_retry_count_);
  ESP_LOGCONFIG(TAG, "  Service Status: %s", service_initialized_ ? "Initialized" : "Not initialized");
  
  if (service_initialized_) {
    ESP_LOGCONFIG(TAG, "  Ready: %s", is_ready() ? "Yes" : "No");
  }
}

bool InverterTentekComponent::is_ready() const {
  if (!service_initialized_) {
    return false;
  }
  
  return set_power_service_is_ready();
}

bool InverterTentekComponent::get_status(set_power_service_status_t *status) const {
  if (!service_initialized_ || status == nullptr) {
    return false;
  }
  
  return set_power_service_get_status(status) == ESP_OK;
}

}  // namespace inverter_tentek
}  // namespace esphome
