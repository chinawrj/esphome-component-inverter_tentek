# Inverter Tentek Component - ESPHome Integration Guide

## 概述 (Overview)

该组件现已支持双框架运行：
- **ESPHome**: 适用于Home Assistant集成和YAML配置的快速部署
- **ESP-IDF**: 适用于独立项目和高级自定义功能

The component now supports dual framework operation:
- **ESPHome**: For Home Assistant integration and YAML-based quick deployment
- **ESP-IDF**: For standalone projects and advanced customization

## 新增功能 (New Features)

### 1. ESPHome组件支持 (ESPHome Component Support)

现在可以通过简单的YAML配置来控制逆变器：

```yaml
inverter_tentek:
  id: my_inverter
  email: !secret inverter_email
  password: !secret inverter_password
  device_sn: !secret inverter_device_sn
  output_power: 100
```

### 2. 自动化操作 (Automation Actions)

支持ESPHome自动化系统：

```yaml
button:
  - platform: template
    name: "设置功率50%"
    on_press:
      - inverter_tentek.set_power:
          id: my_inverter
          output_power: 50
```

### 3. 双模式构建系统 (Dual-Mode Build System)

- CMakeLists.txt自动检测构建环境
- ESPHome模式下只编译必要的服务组件
- ESP-IDF模式下包含完整的示例应用

## 文件结构 (File Structure)

```
inverter_tentek/
├── __init__.py              # ESPHome组件注册 (Component registration)
├── inverter_tentek.h        # ESPHome C++封装头文件 (Wrapper header)
├── inverter_tentek.cpp      # ESPHome C++封装实现 (Wrapper implementation)
├── CMakeLists.txt          # 双模式构建配置 (Dual-mode build config)
├── README.md               # 原ESP-IDF文档 (Original ESP-IDF docs)
└── main/
    ├── CMakeLists.txt      # 主组件构建配置 (Main component config)
    ├── set_power_service.h # 核心服务头文件 (Core service header)
    ├── set_power_service.c # 核心服务实现 (Core service implementation)
    ├── esp_idf_set_power_example_v2.c  # ESP-IDF独立示例 (Standalone example)
    └── Kconfig.projbuild   # ESP-IDF配置菜单 (Configuration menu)
```

## ESPHome使用方法 (ESPHome Usage)

### 步骤1: 配置secrets.yaml

```yaml
# WiFi凭据
wifi_ssid: "你的WiFi名称"
wifi_password: "你的WiFi密码"

# 逆变器凭据
inverter_email: "your_email@example.com"
inverter_password: "your_password"
inverter_device_sn: "YOUR_DEVICE_SN"

# API加密密钥
api_encryption_key: "your_32_byte_base64_key"
ota_password: "your_ota_password"
```

### 步骤2: 创建ESPHome配置文件

参考 `inverter_controller_example.yaml`:

```yaml
esphome:
  name: inverter-controller
  
esp32:
  board: esp32-c6-devkitm-1
  variant: esp32c6
  framework:
    type: esp-idf

# 加载组件
external_components:
  - source:
      type: local
      path: components/inverter_tentek

# 配置逆变器
inverter_tentek:
  id: my_inverter
  email: !secret inverter_email
  password: !secret inverter_password
  device_sn: !secret inverter_device_sn
  output_power: 100
```

### 步骤3: 编译和上传

```bash
# 激活ESPHome环境
source ~/venv/esphome/bin/activate

# 验证配置
esphome config inverter_controller_example.yaml

# 编译并上传
esphome run inverter_controller_example.yaml

# 查看日志
esphome logs inverter_controller_example.yaml
```

## 配置选项 (Configuration Options)

### 必需参数 (Required)

| 参数 | 类型 | 说明 |
|------|------|------|
| `email` | string | 用户邮箱 (User email) |
| `password` | string | 用户密码 (User password) |
| `device_sn` | string | 设备序列号 (Device serial number) |

### 可选参数 (Optional)

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `output_power` | int | 100 | 初始功率输出 (0-100%) |
| `request_timeout` | time | 10s | HTTP请求超时 |
| `max_retry_count` | int | 3 | 最大重试次数 |

## 自动化示例 (Automation Examples)

### 1. 按钮控制 (Button Control)

```yaml
button:
  - platform: template
    name: "设置功率50%"
    on_press:
      - inverter_tentek.set_power:
          id: my_inverter
          output_power: 50
```

### 2. 滑块控制 (Slider Control)

```yaml
number:
  - platform: template
    name: "逆变器功率输出"
    min_value: 0
    max_value: 100
    step: 5
    on_value:
      - inverter_tentek.set_power:
          id: my_inverter
          output_power: !lambda "return x;"
```

### 3. 定时控制 (Time-Based Control)

```yaml
time:
  - platform: sntp
    on_time:
      - hours: 9
        minutes: 0
        then:
          - inverter_tentek.set_power:
              id: my_inverter
              output_power: 50
      - hours: 17
        minutes: 0
        then:
          - inverter_tentek.set_power:
              id: my_inverter
              output_power: 100
```

### 4. 传感器触发 (Sensor Trigger)

```yaml
binary_sensor:
  - platform: gpio
    pin: GPIO0
    name: "紧急停止"
    on_press:
      - inverter_tentek.set_power:
          id: my_inverter
          output_power: 0
    on_release:
      - inverter_tentek.set_power:
          id: my_inverter
          output_power: 100
```

## ESP-IDF使用方法 (ESP-IDF Usage)

原有的ESP-IDF构建方式保持不变，详见 `README.md`。

The original ESP-IDF build method remains unchanged. See `README.md` for details.

### 快速开始 (Quick Start)

```bash
# 激活ESP-IDF环境
source ~/esp/esp-idf/export.sh

# 设置目标芯片
cd components/inverter_tentek
idf.py set-target esp32c6

# 配置项目
idf.py menuconfig

# 编译和烧录
idf.py build
idf.py -b 1500000 flash
idf.py -b 115200 monitor
```

## 核心架构 (Core Architecture)

```
┌─────────────────────────────────────┐
│   ESPHome YAML Configuration        │
│   (YAML配置)                        │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│   InverterTentekComponent           │
│   (ESPHome C++封装)                 │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│   set_power_service                 │
│   (核心C实现)                        │
│   - 消息队列接口                     │
│   - JSESSIONID管理                  │
│   - 自动重认证                       │
│   - 重试逻辑                         │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│   HTTP API (Tentek云服务器)         │
│   server-tj.shuoxd.com:8080        │
└─────────────────────────────────────┘
```

## 特性对比 (Feature Comparison)

| 特性 | ESPHome | ESP-IDF |
|------|---------|---------|
| 配置方式 | YAML声明式 | C代码 + Kconfig |
| Home Assistant集成 | 原生支持 | 需手动配置 |
| OTA更新 | 内置Web界面 | 需自行实现 |
| 学习曲线 | 平缓 | 陡峭 |
| 自定义能力 | 受限于YAML+lambda | 完全C++控制 |
| 开发速度 | 快速 | 较慢 |

## 故障排除 (Troubleshooting)

### ESPHome编译错误

**错误: 找不到组件**

确保组件路径正确：

```yaml
external_components:
  - source:
      type: local
      path: components/inverter_tentek  # 相对于配置文件目录
```

**错误: 缺少secrets**

创建 `secrets.yaml` 文件并包含所有必需凭据。

### 运行时问题

**服务未认证**

- 验证邮箱/密码凭据正确
- 检查设备序列号匹配逆变器
- 确保互联网连接正常
- 监控日志中的认证错误消息

**HTTP超时错误**

- 增加 `request_timeout` 值
- 检查网络稳定性
- 验证服务器可访问: `ping server-tj.shuoxd.com`

**会话过期 (result:10000)**

服务将自动重新认证。如果持续出现：
- 检查系统时间是否同步 (SNTP)
- 验证凭据未更改

## 安全建议 (Security Recommendations)

1. **凭据管理**: 所有敏感信息存储在 `secrets.yaml`，不要提交到git
2. **强密码**: 使用强密码保护逆变器账户
3. **VPN**: 考虑使用WireGuard VPN进行安全远程访问
4. **OTA保护**: 启用ESPHome OTA密码保护
5. **API加密**: 使用HTTPS加密Home Assistant API

## 更新日志 (Changelog)

### v1.0.0 (2025-10-27)
- ✅ 添加ESPHome组件支持
- ✅ 实现双框架构建系统
- ✅ 创建YAML配置接口
- ✅ 添加自动化操作支持
- ✅ 保持ESP-IDF兼容性

## 许可证 (License)

本组件按原样提供，用于教育和个人用途。

This component is provided as-is for educational and personal use.

## 贡献 (Contributing)

欢迎提交问题和拉取请求！

Issues and Pull Requests are welcome!
