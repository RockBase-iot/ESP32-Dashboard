# ESP32 Dashboard

基于 ESP32 的墨水屏 Dashboard。当前版本已完整实现天气站功能，使用完全免费的 [Open-Meteo](https://open-meteo.com/) API（**无需注册、无需 API Key**），显示当前天气、5 天预报、逐小时气温/降水概率折线图、空气质量及室内传感器数据。

这只是一个开始。后续计划陆续加入加密货币行情、股市数据、本地 IoT 设备监控等更多功能——真正发挥出墨水屏上 **"Dashboard Any"** 的潜力。

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/PlatformIO-espressif32%406.13.0-orange.svg)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue.svg)](https://www.arduino.cc/)

[English](./README.md) | 简体中文

---

## 效果预览

| 开机画面 | 唤醒画面 | 天气主界面 |
|:-:|:-:|:-:|
| ![poweron](image/poweron.png) | ![wakingup](image/wakingup.png) | ![weather](image/WeatherPgae.png) |

| AP 配置模式 | Web 配置页面（概览） | Web 配置页面（设置） |
|:-:|:-:|:-:|
| ![apmode](image/AP%20mode.png) | ![web1](image/web1.png) | ![web3](image/web3.png) |

---

## 功能特性

- **Open-Meteo** 天气 + 空气质量接口，完全免费，无需 API Key
- 当前天气：气温、体感温度、风速、湿度、气压、能见度、紫外线指数
- 5 天逐日预报，附 WMO 天气图标
- 逐小时气温折线 + 降水概率柱状图（未来 12 小时）
- 板载 AHT20 / BME280 传感器提供室内温湿度
- 美国 AQI 指数 + PM2.5 浓度
- 深度睡眠节能，刷新间隔可配置（默认 30 分钟）
- 就寝/起床时间窗口，夜间不刷新
- 浏览器 Web 配置页面：WiFi、位置、单位制、时区、刷新间隔（无需 App）
- AP 配置模式（长按 Boot 键）：ESP32 作为热点，首次配网专用
- 上电配置窗口（`PortalSec` 可配置，默认 30 秒）：每次连上 WiFi 后，Web 配置页面可在设备 IP 上访问；设为 `0` 可关闭以最大化省电
- SNTP 时间同步，可配置 UTC 偏移
- 三色（红/黑/白）墨水屏配色支持
- 多语言 UI：`zh_CN`、`en_US`
- 可配置单位制：°C / °F、km/h / m/s / mph / kn、hPa / inHg / mmHg、km / mi、mm / in

---

## 支持的板子

| 环境名称 | MCU | 显示屏 | 分辨率 | 颜色 | 传感器 | 备注 |
|---|---|---|---|---|---|---|
| `nm-display-420` | ESP32-S3 | 4.2″ EPD (GDEY042Z98) | 400 × 300 | 红/黑/白 | AHT20 | |
| `dfrobot_firebeetle2_esp32e` / `firebeetle32` | ESP32 | 7.5″ EPD (GDEY075T7) | 800 × 480 | 黑/白 | BME280 | 计划支持，很快上线，尚未实测 |

---

## 引脚定义

### NM Display 420（ESP32-S3）

| 信号 | GPIO | 备注 |
|---|---|---|
| EPD CS | 3 | |
| EPD DC | 4 | |
| EPD RST | 5 | |
| EPD BUSY | 6 | |
| EPD SCK | 2 | |
| EPD MOSI | 1 | |
| EPD MISO | 10 | 未使用（显示屏只写） |
| EPD PWR | 21 | 直接接 3.3 V，无需控制 |
| AHT20 SDA | 39 | |
| AHT20 SCL | 38 | |
| AHT20 CTL（电源） | 40 | 访问传感器前拉高 |
| 电池 ADC | A0 | 100 kΩ + 100 kΩ 分压 |
| Boot/唤醒按键 | IO0 | 外部上拉，通过 EXT0 唤醒深度睡眠 |
| AP 配置按键 | IO45 | 外部上拉 |

> 该板上还集成了 ES8311 音频编解码器，与 AHT20 共用 I²C 总线。固件在启动时立即将其置入挂起状态，节省约 3 mA。

---

## 快速开始

### 1. 安装 PlatformIO

安装 [PlatformIO IDE](https://platformio.org/install/ide?install=vscode)（VS Code 插件）或命令行工具。

### 2. 克隆项目

```bash
git clone https://github.com/your-repo/ESP32-Dashboard.git
cd ESP32-Dashboard
```

用 VS Code 打开文件夹，PlatformIO 会自动解析所有依赖。

### 3. 编译 & 烧录

根据硬件选择对应的编译环境：

```bash
# NM Display 420（ESP32-S3，4.2" 三色墨水屏）
pio run -e nm-display-420 -t upload
```

LittleFS 文件系统镜像（Web 页面 HTML）由 `extra_script_fs.py` 自动打包并烧录。

### 4. 首次配置（AP 模式）

1. 设备运行时**长按 Boot 键（IO0）≥ 2 秒**进入 **AP 配置模式**。
2. 屏幕显示热点名称（`esp_dashboard_XXXXXX`）和配置 URL `192.168.4.1`。
3. 用手机或电脑连接该热点，打开浏览器访问 `http://192.168.4.1`。
4. 填写 WiFi 名称/密码、经纬度、城市名称、UTC 偏移、单位制，点击**保存**。
5. 设备重启，自动连接家庭 WiFi，拉取天气数据并刷新屏幕。

### 5. 后续访问

每次唤醒后，设备会保持 WiFi 连接一段可配置的时间（`PortalSec`，默认 30 秒，`0` 表示关闭）。窗口期间设备 IP 显示在屏幕左下角，从同一局域网任意浏览器访问 `http://<设备IP>` 即可进入配置页面。在配置页保存后会提示立即重启，确认后新配置即刻生效；否则将在下次唤醒时生效。

---

## 按键说明

| 操作 | 效果 |
|---|---|
| 短按 Boot（IO0） | 下一页（设备唤醒期间） |
| 长按 Boot（IO0）≥ 2 秒 | AP 配置模式（360 秒后自动重启） |

---

## 配置项说明

所有配置持久化存储于 NVS Flash，通过 Web 页面修改：

| 配置项 | 默认值 | 说明 |
|---|---|---|
| WiFi SSID | — | 2.4 GHz 网络名称 |
| WiFi 密码 | — | 网络密码 |
| 纬度 | `30.6667` | 天气查询位置 |
| 经度 | `104.0667` | 天气查询位置 |
| 城市名称 | `Chengdu, Sichuan, China` | 仅用于屏幕显示 |
| UTC 偏移 | `8` | 与 UTC 的小时差，如 `8` 表示 UTC+8 |
| 睡眠间隔 | `30` 分钟 | 两次刷新之间的深睡时长 |
| 配置窗口 | `30` 秒 | 每次唤醒后 Web 配置页面保持可访问的秒数（`0`–`600`，`0` = 关闭） |
| 就寝时间 | `0` 时 | 停止刷新的整点小时（24 小时制） |
| 起床时间 | `6` 时 | 恢复刷新的整点小时 |
| 温度单位 | `C` | `C` / `F` |
| 风速单位 | `kmh` | `kmh` / `ms` / `mph` / `kn` |
| 气压单位 | `hPa` | `hPa` / `inHg` / `mmHg` |
| 距离单位 | `km` | `km` / `mi` |
| 降水单位 | `mm` | `mm` / `in` |
| 语言 | `zh_CN` | `zh_CN` / `en_US` |

---

## 第三方依赖

### 所有环境通用

| 库 | 版本 | 用途 |
|---|---|---|
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | 7.4.3 | JSON 解析（Open-Meteo API 响应） |
| [GxEPD2](https://github.com/ZinggJM/GxEPD2) | 1.6.8 | 墨水屏驱动 |
| [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) | latest | Web 服务器的异步 TCP 底层 |
| [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) | latest | Web 配置页面后端 |

### 仅 `nm-display-420`

| 库 | 版本 | 用途 |
|---|---|---|
| [Adafruit AHTX0](https://github.com/adafruit/Adafruit_AHTX0) | 2.0.5 | AHT20 温湿度传感器 |
| [Adafruit BusIO](https://github.com/adafruit/Adafruit_BusIO) | 1.17.4 | I²C / SPI 抽象层 |
| [Adafruit Unified Sensor](https://github.com/adafruit/Adafruit_Sensor) | 1.1.15 | 传感器统一抽象 |

---

## 项目结构

```
src/
├── main.cpp                  # 入口 — 调用 DashboardApp::run()
├── app/
│   ├── dashboardApp.cpp/h    # 唤醒周期总控制器
│   ├── config/               # NVS 配置读写、配置项定义
│   ├── weather/              # Open-Meteo HTTP 请求 + JSON 解析
│   ├── wifi/                 # WiFi 连接 + SNTP 时间同步
│   ├── web/                  # ESPAsyncWebServer 配置门户（REST API）
│   └── locale/               # zh_CN / en_US 语言字符串
├── bsp/
│   ├── IBoard.h              # 硬件抽象接口
│   ├── nm_display_420/       # ESP32-S3 + 4.2" 三色墨水屏 BSP
│   ├── firebeetle2_esp32e/   # FireBeetle 2 ESP32-E + BME280 BSP
│   └── firebeetle32/         # FireBeetle ESP32 + BME280 BSP
├── drivers/
│   ├── sensor/               # ISensor 接口 + AHT20 驱动
│   └── audio/                # ES8311 编解码器挂起辅助函数
├── ui/
│   ├── layouts/
│   │   ├── epd_400x300/      # 4.2" 三色布局
│   │   └── epd_800x480/      # 7.5" 黑白布局
│   └── pages/                # PageWeatherBase、PageLoading、PageError
└── assets/
    ├── fonts/                # FreeSans 位图字体（4 pt – 48 pt）
    └── icons/                # 天气图标（16×16 至 96×96）
```

---

## 唤醒周期流程

```
上电 / 定时器唤醒
        │
        ▼
  _detectWakeup()
  ┌──────────────────────────────────┐
  │ 长按（≥2 秒） → AP 配置模式     │
  │ 短按          → 下一页          │
  │ 定时器 / 冷启动 → 正常流程      │
  └──────────────────────────────────┘
        │
        ▼
  _initHardware()      墨水屏初始化 + 传感器初始化
        │
        ├─── AP 模式 ──► SoftAP + Web 门户 → 360 秒后重启
        │
        ▼
  _showLoadingPage()   （冷启动 / 按键唤醒时显示）
        │
        ▼
  _connectAndSync()    WiFi 连接 + SNTP 时间同步
        │ 失败 → 最多重试 3 次，超过则显示错误页 → 深度睡眠
        ▼
  _fetchData()         Open-Meteo 天气 + 空气质量 HTTP 请求
        │ 失败 → 最多重试 3 次，超过则显示错误页 → 深度睡眠
        ▼
  _renderWeather()     墨水屏分页渲染循环
        │
        ▼  （PortalSec > 0：Web 门户 + 按键窗口 N 秒）
  WiFi 断开 → 墨水屏休眠 → 深度睡眠（N 分钟）
```

---

## 移植到新板子

1. **创建** `src/bsp/<board_name>/config.h` — 定义 `DISP_WIDTH`、`DISP_HEIGHT` 以及所有 `PIN_*` 常量。

2. **创建** `src/bsp/<board_name>/Board.cpp` — 实现 `IBoard` 纯虚函数：

   | 方法 | 要求 |
   |---|---|
   | `init()` | 串口、电源轨、外设初始化 |
   | `epd()` | 返回封装 GxEPD2 的 `IEpdDriver&` |
   | `gfx()` | 返回 GxEPD2 的 `Adafruit_GFX&` |
   | `colorAccent()` / `hasAccentColor()` | 三色屏返回红色/true；黑白屏返回黑色/false |
   | `getTempSensor()` | 返回 `ISensor*`，无传感器时返回 `nullptr` |
   | `readBatteryMv()` | ADC 读值转换为毫伏 |
   | `deepSleep(us)` | 配置唤醒源后调用 `esp_deep_sleep_start()` |
   | `bootButtonPin()` / `apButtonPin()` | 返回对应 GPIO 编号 |

3. **添加 UI 布局**（若分辨率与现有布局不同）：在 `src/ui/layouts/epd_NNNxNNN/` 中继承 `PageWeatherBase`，实现 `_drawCurrentConditions()`、`_drawForecast()`、`_drawStatusBar()`。

4. **在 `platformio.ini` 中添加环境配置**：

   ```ini
   [env:my_board]
   board = <pio_board_id>
   build_src_filter =
       +<*>
       -<bsp/>        +<bsp/my_board/>
       -<ui/layouts/> +<ui/layouts/epd_NNNxNNN/>
   build_flags =
       ${env.build_flags}
       -DUI_LAYOUT_EPD_NNNxNNN
   lib_deps =
       ${env.lib_deps}
       <额外传感器库>
   ```

---

## 致谢

- [Open-Meteo](https://open-meteo.com/) — 免费开源天气 API
- [GxEPD2](https://github.com/ZinggJM/GxEPD2) — 墨水屏驱动库
- [ArduinoJson](https://arduinojson.org/) — Arduino / ESP32 JSON 库
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) — 异步 HTTP 服务器
- [Adafruit](https://github.com/adafruit) — 传感器及 GFX 库

