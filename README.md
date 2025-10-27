# ESP-IDF Set Power Example

ESP-IDF based MIC POWER inverter power setting example project.

## Features

- ✅ **HTTP POST Requests** - Send power setting requests using ESP-IDF HTTP client
- ✅ **MD5 Signature Calculation** - Complete implementation of API signature algorithm
- ✅ **Scheduled Tasks** - Independent FreeRTOS task sends requests every minute
- ✅ **Error Handling** - Comprehensive timeout and error handling mechanism
- ✅ **WiFi Management** - Automatic WiFi connection and reconnection
- ✅ **Authentication Support** - JSESSIONID cookie authentication

## Project Structure

```
esp_idf_set_power_example/
├── CMakeLists.txt                    # Project configuration file
├── main/
│   ├── CMakeLists.txt               # Main component configuration
│   └── esp_idf_set_power_example.c  # Main program source code
└── sdkconfig                        # ESP-IDF configuration file
```

## Hardware Requirements

- ESP32 / ESP32-C3 / ESP32-C6 / ESP32-S3 Development Board
- USB Cable
- WiFi Network Connection

## Software Requirements

- ESP-IDF v5.0 or higher
- Python 3.8+
- Configured ESP-IDF development environment

## Configuration Steps

### 1. Modify WiFi Configuration

Edit WiFi configuration in `esp_idf_set_power_example.c`:

```c
#define WIFI_SSID      "your_wifi_ssid"      // Your WiFi name
#define WIFI_PASSWORD  "your_wifi_password"  // Your WiFi password
```

### 2. Modify Device Configuration

Modify device serial number and authentication information:

```c
#define DEVICE_SN      "TJ0323E30A0993"                      // Device serial number
#define JSESSIONID     "4E5B239CA671AE226063F4620697D07A"    // Session ID after login
```

### 3. Adjust Request Interval (Optional)

Default is one request every 60 seconds, you can modify:

```c
#define REQUEST_INTERVAL_MS  (60 * 1000)  // 60 seconds
```

## Build and Flash

### 1. Activate ESP-IDF Environment

```bash
cd ~/esp/esp-idf
. ./export.sh
```

### 2. Set Target Chip

```bash
# For ESP32
idf.py set-target esp32

# For ESP32-C6
idf.py set-target esp32c6

# For ESP32-S3
idf.py set-target esp32s3
```

### 3. Configure Project (Optional)

```bash
idf.py menuconfig
```

Important configuration items:
- `Component config → ESP HTTP Client` → Enable HTTP client
- `Component config → mbedTLS` → Ensure MD5 support is enabled
- `Component config → FreeRTOS` → Adjust task stack size (if needed)

### 4. Build Project

```bash
idf.py build
```

### 5. Flash to Device

```bash
# Use 1500000 baud rate for flashing (recommended)
idf.py -b 1500000 flash

# Or use default baud rate
idf.py flash
```

### 6. Monitor Serial Output

```bash
idf.py -b 115200 monitor

# Or flash and monitor in one go
idf.py -b 1500000 flash monitor
```

## Log Output Examples

### Normal Operation

```
I (1234) SET_POWER: MIC POWER Set Inverter Power Example
I (1245) SET_POWER: ESP-IDF Version: v5.1.2
I (1256) SET_POWER: Initializing WiFi...
I (1267) SET_POWER: wifi_init_sta finished.
I (3456) SET_POWER: Connected to AP SSID:your_wifi_ssid
I (3467) SET_POWER: Got IP:192.168.1.100
I (3478) SET_POWER: WiFi initialized successfully
I (3489) SET_POWER: Set power task created successfully
I (3500) SET_POWER: System initialized, monitoring will run every 60 seconds
I (3511) SET_POWER: Set power task started
I (3522) SET_POWER: WiFi connected, starting periodic requests
I (3533) SET_POWER: Preparing request: device=TJ0323E30A0993, power=100%, timestamp=1761566400000
I (3544) SET_POWER: Sign string: deviceSn=TJ0323E30A0993&outputPower=1001f80ca5871919371ea71716cae4841bd
I (3555) SET_POWER: Signature: 21cb9f6448bd1b65a78c05e2f2c6e0a6
I (3566) SET_POWER: Sending HTTP POST request...
I (3577) SET_POWER: HTTP_EVENT_ON_CONNECTED
I (3788) SET_POWER: HTTP_EVENT_ON_DATA, len=45
I (3799) SET_POWER: HTTP_EVENT_ON_FINISH
I (3810) SET_POWER: HTTP Status = 200, content_length = 45
I (3821) SET_POWER: Response: {"result":2,"msg":"Device offline!","obj":null}
I (3832) SET_POWER: ⚠️  Device offline
I (3843) SET_POWER: ✅ Request successful (power=100%)
I (3854) SET_POWER: Waiting 60 seconds until next request...
```

### Authentication Failure

```
E (3821) SET_POWER: ❌ Authentication failed: Please login
E (3832) SET_POWER: ❌ Request failed (1/5 consecutive failures)
```

### Network Error

```
E (10000) SET_POWER: ❌ HTTP request failed: ESP_ERR_HTTP_TIMEOUT
E (10011) SET_POWER: ❌ Request failed (2/5 consecutive failures)
```

## Code Architecture

### Main Functions

| Function Name | Function Description |
|--------|---------|
| `app_main()` | Main entry function, system initialization |
| `wifi_init_sta()` | WiFi station mode initialization |
| `wifi_event_handler()` | WiFi event handler |
| `set_power_task()` | FreeRTOS task for sending periodic requests |
| `send_set_power_request()` | Send HTTP POST request |
| `calculate_signature()` | Calculate MD5 signature |
| `url_encode()` | URL encoding function |
| `http_event_handler()` | HTTP event handler |

### Key Configuration Macros

| Macro Definition | Default Value | Description |
|--------|--------|------|
| `WIFI_SSID` | "your_wifi_ssid" | WiFi network name |
| `WIFI_PASSWORD` | "your_wifi_password" | WiFi password |
| `WIFI_MAXIMUM_RETRY` | 5 | Maximum WiFi reconnection attempts |
| `REQUEST_INTERVAL_MS` | 60000 | Request interval (milliseconds) |
| `HTTP_TIMEOUT_MS` | 10000 | HTTP timeout (milliseconds) |
| `SET_POWER_TASK_STACK_SIZE` | 8192 | Task stack size (bytes) |
| `SET_POWER_TASK_PRIORITY` | 5 | Task priority |

## Error Handling Mechanism

### 1. HTTP Timeout Handling
- Timeout duration: 10 seconds
- Automatically returns error after timeout

### 2. Consecutive Failure Handling
- Records consecutive failure count
- After 5 consecutive failures, attempts WiFi reconnection
- Resets failure counter after successful reconnection

### 3. WiFi Disconnection Handling
- Automatic reconnection mechanism
- Maximum 5 retry attempts
- Continues normal operation after successful reconnection

### 4. JSON Response Parsing
- `result=0`: Setting successful
- `result=2`: Device offline (considered successful)
- `result=10000`: Authentication failure
- Other: Unknown error

## Extension Feature Suggestions

### 1. Dynamic Power Adjustment

```c
// Add to set_power_task()
int calculate_optimal_power(void) {
    // Calculate dynamically based on time, weather, load conditions
    struct tm timeinfo;
    time_t now = time(NULL);
    localtime_r(&now, &timeinfo);
    
    // Solar inverter: full power during daytime, stop at night (no solar energy)
    if (timeinfo.tm_hour >= 6 && timeinfo.tm_hour < 18) {
        return 100;  // 100% during daytime
    } else {
        return 0;    // 0% at night (no solar power available)
    }
}
```

### 2. HTTPS Support

Modify URL and configuration:

```c
#define API_URL "https://server-tj.shuoxd.com:8443/v1/manage/setOnGridInverterParam"

esp_http_client_config_t config = {
    .url = API_URL,
    .transport_type = HTTP_TRANSPORT_OVER_SSL,
    .cert_pem = server_cert_pem_start,  // Server certificate
    // ... other configurations
};
```

### 3. Login Feature

Add login API call to automatically get JSESSIONID:

```c
esp_err_t login_and_get_session(const char *email, const char *password, char *session_out);
```

### 4. OTA Firmware Update

Integrate ESP-IDF OTA functionality for remote updates.

## Troubleshooting

### Issue 1: Compile error `mbedtls/md5.h not found`

**Solution**:
```bash
idf.py menuconfig
# Navigate to Component config → mbedTLS → Ensure MD5 support is enabled
```

### Issue 2: WiFi connection failure

**Check items**:
1. WiFi SSID and password are correct
2. 2.4GHz WiFi support (ESP32 does not support 5GHz)
3. Router allows new device connections

### Issue 3: HTTP request timeout

**Possible causes**:
1. Server unreachable
2. Network firewall blocking
3. DNS resolution failure

**Solutions**:
- Increase timeout duration
- Use IP address instead of domain name
- Check network connection

### Issue 4: Authentication failure (result=10000)

**Reason**: JSESSIONID has expired

**Solutions**:
1. Use Python script to re-login and get new JSESSIONID
2. Implement automatic login feature

## Performance Optimization

### Memory Optimization
- Adjust `SET_POWER_TASK_STACK_SIZE` based on actual requirements
- Use `uxTaskGetStackHighWaterMark()` to monitor stack usage

### Power Consumption Optimization
- Use WiFi power saving mode
- Use Deep Sleep between request intervals
- Reduce log output level

## References

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [ESP HTTP Client](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html)
- [FreeRTOS](https://www.freertos.org/Documentation/RTOS_book.html)
- [ESP WiFi](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html)

## License

This example code follows the ESP-IDF license (Apache 2.0)

## Contributing

Issues and Pull Requests are welcome to improve this example!
