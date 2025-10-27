# ESP-IDF Set Power Example

基于ESP-IDF的MIC POWER逆变器功率设置示例项目。

## 功能特性

- ✅ **HTTP POST请求** - 使用ESP-IDF HTTP客户端发送设置功率请求
- ✅ **MD5签名计算** - 完整实现API签名算法
- ✅ **定时任务** - 独立FreeRTOS任务每分钟发送一次请求
- ✅ **错误处理** - 完善的超时和错误处理机制
- ✅ **WiFi管理** - 自动WiFi连接和重连
- ✅ **认证支持** - JSESSIONID cookie认证

## 项目结构

```
esp_idf_set_power_example/
├── CMakeLists.txt                    # 项目配置文件
├── main/
│   ├── CMakeLists.txt               # 主组件配置
│   └── esp_idf_set_power_example.c  # 主程序源码
└── sdkconfig                        # ESP-IDF配置文件
```

## 硬件要求

- ESP32 / ESP32-C3 / ESP32-C6 / ESP32-S3 开发板
- USB连接线
- WiFi网络连接

## 软件要求

- ESP-IDF v5.0 或更高版本
- Python 3.8+
- 已配置的ESP-IDF开发环境

## 配置步骤

### 1. 修改WiFi配置

编辑 `esp_idf_set_power_example.c` 中的WiFi配置：

```c
#define WIFI_SSID      "your_wifi_ssid"      // 你的WiFi名称
#define WIFI_PASSWORD  "your_wifi_password"  // 你的WiFi密码
```

### 2. 修改设备配置

修改设备序列号和认证信息：

```c
#define DEVICE_SN      "TJ0323E30A0993"                      // 设备序列号
#define JSESSIONID     "4E5B239CA671AE226063F4620697D07A"    // 登录后的Session ID
```

### 3. 调整请求间隔（可选）

默认每60秒发送一次请求，可以修改：

```c
#define REQUEST_INTERVAL_MS  (60 * 1000)  // 60秒
```

## 构建和烧录

### 1. 激活ESP-IDF环境

```bash
cd ~/esp/esp-idf
. ./export.sh
```

### 2. 设置目标芯片

```bash
# 对于ESP32
idf.py set-target esp32

# 对于ESP32-C6
idf.py set-target esp32c6

# 对于ESP32-S3
idf.py set-target esp32s3
```

### 3. 配置项目（可选）

```bash
idf.py menuconfig
```

重要配置项：
- `Component config → ESP HTTP Client` → 启用HTTP客户端
- `Component config → mbedTLS` → 确保MD5支持已启用
- `Component config → FreeRTOS` → 调整任务栈大小（如需要）

### 4. 构建项目

```bash
idf.py build
```

### 5. 烧录到设备

```bash
# 使用1500000波特率烧录（推荐）
idf.py -b 1500000 flash

# 或使用默认波特率
idf.py flash
```

### 6. 监控串口输出

```bash
idf.py -b 115200 monitor

# 或者一次性完成烧录和监控
idf.py -b 1500000 flash monitor
```

## 日志输出示例

### 正常运行

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
I (3821) SET_POWER: Response: {"result":2,"msg":"设备离线!","obj":null}
I (3832) SET_POWER: ⚠️  Device offline
I (3843) SET_POWER: ✅ Request successful (power=100%)
I (3854) SET_POWER: Waiting 60 seconds until next request...
```

### 认证失败

```
E (3821) SET_POWER: ❌ Authentication failed: Please login
E (3832) SET_POWER: ❌ Request failed (1/5 consecutive failures)
```

### 网络错误

```
E (10000) SET_POWER: ❌ HTTP request failed: ESP_ERR_HTTP_TIMEOUT
E (10011) SET_POWER: ❌ Request failed (2/5 consecutive failures)
```

## 代码架构

### 主要函数

| 函数名 | 功能描述 |
|--------|---------|
| `app_main()` | 主入口函数，初始化系统 |
| `wifi_init_sta()` | WiFi站点模式初始化 |
| `wifi_event_handler()` | WiFi事件处理器 |
| `set_power_task()` | 定时发送请求的FreeRTOS任务 |
| `send_set_power_request()` | 发送HTTP POST请求 |
| `calculate_signature()` | 计算MD5签名 |
| `url_encode()` | URL编码函数 |
| `http_event_handler()` | HTTP事件处理器 |

### 关键配置宏

| 宏定义 | 默认值 | 说明 |
|--------|--------|------|
| `WIFI_SSID` | "your_wifi_ssid" | WiFi网络名称 |
| `WIFI_PASSWORD` | "your_wifi_password" | WiFi密码 |
| `WIFI_MAXIMUM_RETRY` | 5 | WiFi重连最大次数 |
| `REQUEST_INTERVAL_MS` | 60000 | 请求间隔（毫秒） |
| `HTTP_TIMEOUT_MS` | 10000 | HTTP超时时间（毫秒） |
| `SET_POWER_TASK_STACK_SIZE` | 8192 | 任务栈大小（字节） |
| `SET_POWER_TASK_PRIORITY` | 5 | 任务优先级 |

## 错误处理机制

### 1. HTTP超时处理
- 超时时间：10秒
- 超时后自动返回错误

### 2. 连续失败处理
- 记录连续失败次数
- 达到5次连续失败后尝试重连WiFi
- 重连后重置失败计数器

### 3. WiFi断连处理
- 自动重连机制
- 最多重试5次
- 重连成功后继续正常运行

### 4. JSON响应解析
- `result=0`: 设置成功
- `result=2`: 设备离线（视为成功）
- `result=10000`: 认证失败
- 其他：未知错误

## 扩展功能建议

### 1. 动态功率调整

```c
// 在 set_power_task() 中添加
int calculate_optimal_power(void) {
    // 根据时间、天气、负载等条件动态计算
    struct tm timeinfo;
    time_t now = time(NULL);
    localtime_r(&now, &timeinfo);
    
    // 白天高功率，夜间低功率
    if (timeinfo.tm_hour >= 6 && timeinfo.tm_hour < 18) {
        return 100;  // 100%
    } else {
        return 50;   // 50%
    }
}
```

### 2. HTTPS支持

修改URL和配置：

```c
#define API_URL "https://server-tj.shuoxd.com:8443/v1/manage/setOnGridInverterParam"

esp_http_client_config_t config = {
    .url = API_URL,
    .transport_type = HTTP_TRANSPORT_OVER_SSL,
    .cert_pem = server_cert_pem_start,  // 服务器证书
    // ... 其他配置
};
```

### 3. 登录功能

添加登录API调用以自动获取JSESSIONID：

```c
esp_err_t login_and_get_session(const char *email, const char *password, char *session_out);
```

### 4. OTA固件更新

集成ESP-IDF OTA功能实现远程更新。

## 故障排除

### 问题1: 编译错误 `mbedtls/md5.h not found`

**解决方案**：
```bash
idf.py menuconfig
# 进入 Component config → mbedTLS → 确保 MD5 支持已启用
```

### 问题2: WiFi连接失败

**检查项**：
1. WiFi SSID和密码是否正确
2. 2.4GHz WiFi支持（ESP32不支持5GHz）
3. 路由器是否允许新设备接入

### 问题3: HTTP请求超时

**可能原因**：
1. 服务器不可达
2. 网络防火墙阻止
3. DNS解析失败

**解决方案**：
- 增加超时时间
- 使用IP地址替代域名
- 检查网络连接

### 问题4: 认证失败 (result=10000)

**原因**: JSESSIONID已过期

**解决方案**：
1. 使用Python脚本重新登录获取新的JSESSIONID
2. 实现自动登录功能

## 性能优化

### 内存优化
- 调整 `SET_POWER_TASK_STACK_SIZE` 根据实际需求
- 使用 `uxTaskGetStackHighWaterMark()` 监控栈使用

### 功耗优化
- 使用WiFi省电模式
- 在请求间隔使用Deep Sleep
- 减少日志输出级别

## 参考资料

- [ESP-IDF编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/)
- [ESP HTTP Client](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/protocols/esp_http_client.html)
- [FreeRTOS](https://www.freertos.org/Documentation/RTOS_book.html)
- [ESP WiFi](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-guides/wifi.html)

## 许可证

本示例代码遵循ESP-IDF许可证（Apache 2.0）

## 贡献

欢迎提交Issue和Pull Request改进本示例！
