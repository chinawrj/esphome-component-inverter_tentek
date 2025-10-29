/**
 * @file inverter_tentek.h
 * @brief ESPHome Component for Tentek/MIC POWER Inverter Control
 * 
 * This component wraps the set_power_service for use in ESPHome.
 * It provides a high-level interface for controlling inverter power output
 * through the HTTP API with automatic session management.
 * 
 * Features:
 * - Automatic JSESSIONID management and re-authentication
 * - Background service task for non-blocking operation
 * - Configurable power output (0-100%)
 * - Statistics tracking and monitoring
 * - ESPHome automation actions
 * 
 * @note Requires WiFi to be connected before initialization
 * 
 * @author GitHub Copilot
 * @date 2025-10-27
 */

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/log.h"
#include <string>

// Include ESP-IDF set_power_service (located in main/)
extern "C" {
#include "set_power_service.h"
}

namespace esphome {
namespace inverter_tentek {

/**
 * @class InverterTentekComponent
 * @brief ESPHome component for Tentek/MIC POWER inverter control
 */
class InverterTentekComponent : public Component {
 public:
  /**
   * @brief Set user email for authentication
   * @param email User email address
   */
  void set_email(const std::string &email) { email_ = email; }

  /**
   * @brief Set user password for authentication
   * @param password User password
   */
  void set_password(const std::string &password) { password_ = password; }

  /**
   * @brief Set device serial number
   * @param device_sn Device serial number
   */
  void set_device_sn(const std::string &device_sn) { device_sn_ = device_sn; }

  /**
   * @brief Set output power percentage
   * @param power Output power percentage (0-100)
   */
  void set_output_power(int power);

  /**
   * @brief Get current output power setting
   * @return int Current power percentage (0-100)
   */
  int get_output_power() const { return output_power_; }

  /**
   * @brief Set HTTP request timeout
   * @param timeout_ms Timeout in milliseconds
   */
  void set_request_timeout(uint32_t timeout_ms) { request_timeout_ms_ = timeout_ms; }

  /**
   * @brief Set maximum retry count for failed requests
   * @param max_retry Maximum retry count
   */
  void set_max_retry_count(uint8_t max_retry) { max_retry_count_ = max_retry; }

  /**
   * @brief Component setup (called once during initialization)
   */
  void setup() override;

  /**
   * @brief Component loop (called periodically)
   */
  void loop() override;

  /**
   * @brief Get component setup priority
   * @return float Setup priority (lower value = later initialization)
   */
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  /**
   * @brief Dump component configuration to log
   */
  void dump_config() override;

  /**
   * @brief Check if service is ready
   * @return bool True if authenticated and ready
   */
  bool is_ready() const;

  /**
   * @brief Get service status information
   * @param status Pointer to status structure to fill
   * @return bool True on success
   */
  bool get_status(set_power_service_status_t *status) const;

 protected:
  std::string email_;              ///< User email for authentication
  std::string password_;           ///< User password for authentication
  std::string device_sn_;          ///< Device serial number
  int output_power_{-1};           ///< Current power output setting (-1=not set, 0-100% valid)
  uint32_t request_timeout_ms_{10000};  ///< HTTP request timeout
  uint8_t max_retry_count_{3};     ///< Maximum retry count
  
  bool service_initialized_{false};  ///< Service initialization status
  uint32_t last_status_log_time_{0}; ///< Last status log timestamp
};

/**
 * @class SetPowerAction
 * @brief ESPHome automation action for setting power output
 */
template<typename... Ts> class SetPowerAction : public Action<Ts...> {
 public:
  SetPowerAction(InverterTentekComponent *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(int, power)

  void play(Ts... x) override {
    int power = this->power_.value(x...);
    this->parent_->set_output_power(power);
  }

 protected:
  InverterTentekComponent *parent_;
};

}  // namespace inverter_tentek
}  // namespace esphome
