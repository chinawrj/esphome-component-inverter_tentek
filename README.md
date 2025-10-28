# ESPHome Inverter Tentek Component

ESPHome custom component for controlling Tentek solar inverters via HTTP API.

## Features

- ✅ **ESPHome Integration** - Native ESPHome component with YAML configuration
- ✅ **HTTP API Control** - Control inverter power output via Tentek cloud API
- ✅ **Authentication** - Secure login with email/password credentials
- ✅ **Power Control** - Set inverter output power (10% - 100% in 10% steps)
- ✅ **MD5 Signature** - Automatic API request signing
- ✅ **Error Handling** - Comprehensive retry mechanism and error logging
- ✅ **ESPHome Automation** - Integrate with ESPHome automations and triggers

## Supported Hardware

- ESP32 (all variants)
- ESP32-C3
- ESP32-C6
- ESP32-S3
- Any ESP32 board supported by ESPHome

## Installation

### Method 1: Local Component (Recommended for Development)

1. Clone this repository into your ESPHome `components` directory:

```bash
cd esphome_config_directory
mkdir -p components
cd components
git clone https://github.com/chinawrj/esphome-component-inverter_tentek.git inverter_tentek
```

2. Reference in your ESPHome YAML configuration:

```yaml
external_components:
  - source:
      type: local
      path: components
```

### Method 2: GitHub Repository (Recommended for Production)

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/chinawrj/esphome-component-inverter_tentek
      ref: main
    components: [inverter_tentek]
```

## Configuration

### Basic Example

```yaml
# Enable external components
external_components:
  - source:
      type: local
      path: components

# Configure Tentek inverter
inverter_tentek:
  id: my_inverter
  email: !secret tentek_email          # Your Tentek account email
  password: !secret tentek_password    # Your Tentek account password
  device_sn: !secret tentek_device_sn  # Your inverter device serial number
  output_power: 100                    # Initial power: 100%
  request_timeout: 10s                 # HTTP request timeout
  max_retry_count: 3                   # Max retry attempts on failure
```

### Secrets Configuration

Add to your `secrets.yaml`:

```yaml
# Tentek Inverter Credentials
tentek_email: "your_email@example.com"
tentek_password: "your_password"
tentek_device_sn: "YOUR_DEVICE_SERIAL_NUMBER"
```

### Advanced Configuration with Automation

```yaml
inverter_tentek:
  id: solar_inverter
  email: !secret tentek_email
  password: !secret tentek_password
  device_sn: !secret tentek_device_sn
  output_power: 100
  request_timeout: 10s
  max_retry_count: 3

# Global variable to track current power level
globals:
  - id: current_power_percent
    type: int
    restore_value: true
    initial_value: '100'

# Automation example: Adjust power based on time of day
time:
  - platform: sntp
    id: sntp_time
    on_time:
      # Full power during peak solar hours (10:00 - 16:00)
      - hours: 10
        minutes: 0
        seconds: 0
        then:
          - lambda: |-
              id(solar_inverter).set_output_power(100);
              id(current_power_percent) = 100;
      
      # Reduce power in evening (18:00)
      - hours: 18
        minutes: 0
        seconds: 0
        then:
          - lambda: |-
              id(solar_inverter).set_output_power(50);
              id(current_power_percent) = 50;
      
      # Minimum power at night (22:00)
      - hours: 22
        minutes: 0
        seconds: 0
        then:
          - lambda: |-
              id(solar_inverter).set_output_power(10);
              id(current_power_percent) = 10;
```

## Component API

### Configuration Options

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `id` | ID | Yes | - | Component identifier for referencing in automations |
| `email` | string | Yes | - | Tentek account email address |
| `password` | string | Yes | - | Tentek account password |
| `device_sn` | string | Yes | - | Inverter device serial number |
| `output_power` | int | No | 100 | Initial power level (10-100, step 10) |
| `request_timeout` | time | No | 10s | HTTP request timeout duration |
| `max_retry_count` | int | No | 3 | Maximum retry attempts on failure |

### Lambda Functions

#### `set_output_power(int power_percent)`

Set inverter output power level.

**Parameters:**
- `power_percent`: Power level percentage (10, 20, 30, ..., 100)

**Example:**
```yaml
lambda: |-
  id(solar_inverter).set_output_power(80);  // Set to 80%
```

**Constraints:**
- Only accepts multiples of 10 (10%, 20%, 30%, ..., 100%)
- Values outside this range will be clamped to valid range
- Power setting is persistent until manually changed

## Use Cases

### 1. Dynamic Power Control Based on Grid Monitoring

```yaml
# Monitor grid power and adjust solar inverter
on_inverter_output_power_adjustment:
  - then:
      - lambda: |-
          float power_gap = power_gap_watts;
          int current_power = id(current_power_percent);
          
          // Each 10% = 120W
          int power_step_change = (int)(abs(power_gap) / 120.0);
          
          if (power_gap > 0) {
            // Reduce solar generation
            int new_power = current_power - (power_step_change * 10);
            if (new_power < 10) new_power = 10;
            id(solar_inverter).set_output_power(new_power);
          } else if (power_gap < 0) {
            // Increase solar generation
            int new_power = current_power + (power_step_change * 10);
            if (new_power > 100) new_power = 100;
            id(solar_inverter).set_output_power(new_power);
          }
```

### 2. Time-Based Power Scheduling

```yaml
# Schedule power levels throughout the day
time:
  - platform: sntp
    on_time:
      # Morning: Start with moderate power
      - hours: 6
        minutes: 0
        then:
          - lambda: id(solar_inverter).set_output_power(50);
      
      # Midday: Maximum power
      - hours: 10
        minutes: 0
        then:
          - lambda: id(solar_inverter).set_output_power(100);
      
      # Evening: Reduce power
      - hours: 18
        minutes: 0
        then:
          - lambda: id(solar_inverter).set_output_power(30);
      
      # Night: Minimum power (solar panels inactive)
      - hours: 22
        minutes: 0
        then:
          - lambda: id(solar_inverter).set_output_power(10);
```

### 3. Power Control with Rate Limiting

```yaml
globals:
  - id: last_adjustment_time
    type: unsigned long
    initial_value: '0'

# Rate-limited power adjustment (minimum 10 seconds between changes)
lambda: |-
  const uint32_t now = millis();
  const uint32_t RATE_LIMIT_MS = 10 * 1000;  // 10 seconds
  
  if (now - id(last_adjustment_time) >= RATE_LIMIT_MS) {
    id(solar_inverter).set_output_power(new_power_level);
    id(last_adjustment_time) = now;
    ESP_LOGI("power_control", "Power adjusted to %d%%", new_power_level);
  } else {
    ESP_LOGD("power_control", "Rate limit active, skipping adjustment");
  }
```

## Technical Details

### Power Level Constraints

- **Valid Range**: 10% - 100%
- **Step Size**: 10%
- **Valid Values**: 10, 20, 30, 40, 50, 60, 70, 80, 90, 100
- **No 0% Support**: Tentek inverters do not support 0% power setting

### API Communication

- **Protocol**: HTTP (not HTTPS)
- **Authentication**: MD5 signature-based with automatic login
- **Endpoint**: Tentek cloud API server
- **Session Management**: Automatic JSESSIONID handling
- **Retry Logic**: Configurable retry attempts with exponential backoff

### Performance Characteristics

- **Request Duration**: ~500ms - 2s (network dependent)
- **Memory Usage**: ~4KB heap per component instance
- **WiFi Dependency**: Requires active WiFi connection

## Troubleshooting

### Issue 1: Authentication Failed

**Symptoms**: Log shows "Authentication failed" or "Login error"

**Solutions**:
1. Verify email and password are correct in `secrets.yaml`
2. Check Tentek account is active and accessible
3. Ensure device serial number matches your inverter

### Issue 2: Device Offline

**Symptoms**: API returns "Device offline"

**Solutions**:
1. Check inverter is powered on and connected to internet
2. Verify device serial number is correct
3. Check inverter cloud connection status in Tentek app

### Issue 3: Request Timeout

**Symptoms**: HTTP request timeout errors

**Solutions**:
1. Increase `request_timeout` value (e.g., 15s or 20s)
2. Check WiFi signal strength on ESP32
3. Verify network allows outbound HTTP connections

### Issue 4: Invalid Power Level

**Symptoms**: Power setting has no effect

**Solutions**:
1. Ensure power value is multiple of 10 (10, 20, ..., 100)
2. Check inverter supports the requested power level
3. Review logs for error messages

## Development and Contributing

### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/chinawrj/esphome-component-inverter_tentek.git
cd esphome-component-inverter_tentek
```

2. Test with ESPHome:
```bash
esphome config your_device.yaml
esphome compile your_device.yaml
```

### Project Structure

```
inverter_tentek/
├── __init__.py           # ESPHome component registration
├── inverter_tentek.h     # C++ header file
├── inverter_tentek.cpp   # C++ implementation
├── README.md             # This file
└── CMakeLists.txt        # ESP-IDF build configuration
```

### Contributing Guidelines

1. Fork the repository
2. Create a feature branch
3. Make your changes with clear commit messages in English
4. Test thoroughly with actual hardware
5. Submit a pull request

## License

This component follows the Apache 2.0 license.

## Support

- **Issues**: [GitHub Issues](https://github.com/chinawrj/esphome-component-inverter_tentek/issues)
- **ESPHome**: [ESPHome Documentation](https://esphome.io/)

## Related Projects

- [esphome-component-powersync](https://github.com/chinawrj/esphome-component-powersync) - Distributed power monitoring system
- [esphome-component-dlt645](https://github.com/chinawrj/esphome-component-dlt645) - DL/T 645 smart meter protocol

## Changelog

### Version 1.0.0 (2025-10-29)
- ✅ Initial release
- ✅ Basic power control functionality
- ✅ ESPHome integration
- ✅ Authentication and session management
- ✅ Error handling and retry mechanism
