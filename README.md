# ESP32-Dashboard
A ESP32-based desktop information station which integrated with clock, weather, stocks and cryptocurrency price.

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ESP32-orange.svg)](https://www.espressif.com/)
[![Display](https://img.shields.io/badge/Display-TFT%2FeInk-green.svg)]()

English | [简体中文](./README_cn.md)

---

## ✨ Features

- 🕐 **Precision Clock** - NTP auto-sync with multi-timezone support
- 🌤️ **Real-time Weather** - Current conditions, forecasts, and weather alerts
- 📈 **Stock Tracking** - Real-time prices, change percentages, and trend charts
- 💰 **Crypto Monitor** - Bitcoin, Ethereum, and other major cryptocurrency prices
- 🖥️ **Multi-Display** - Supports 2.8"/4.0" TFT color screens, 4.2" tri-color e-ink
- 📱 **Smart Config** - Mobile/web-based configuration, no hard-coding required
- 🔆 **Auto Brightness** - Automatic screen brightness adjustment based on ambient light
- 🔋 **Ultra Low Power** - Deep sleep mode for e-ink displays, standby for months
- 🎨 **Theme Switching** - Multiple UI themes for different scenarios

---

## 📸 Preview

## 🛠️ Hardware Support

### Display Options

| Size | Type | Driver Chip | Resolution | Status |
|------|------|-------------|------------|--------|
| 2.8" | TFT Color | ST7789 | 240×320 | [ ]Supported |
| 4.0" | TFT Color | ST7796 | 320×480 | [ ]Supported |
| 4.2" | BWR E-ink | GDE042A2 | 400×300 | [ ]Supported |

### Optional Components

- **Ambient Light Sensor** - BH1750 / LTR-553ALS for auto brightness
- **Temp/Humidity Sensor** - DHT22 / SHT30 for indoor monitoring
- **RTC Module** - DS3231 for offline timekeeping
- **Speaker** - Voice announcements for weather and hourly chimes

## 🚀 Quick Start

---

## ⚙️ Configuration

---

## 🎨 Interface Showcase

### Main Screen
- Large time display
- Date/day of week
- Current weather icon and temperature

### Weather Screen
- Current weather details (temp, humidity, pressure, wind)
- 3-day forecast
- Air quality index

### Stock Screen
- Watchlist with real-time prices
- Price changes and percentages
- Mini candlestick charts

### Crypto Screen
- Major cryptocurrency prices
- 24-hour change percentages
- Market cap rankings

---

### TODO List

- [ ] Support more display sizes (5.65" e-ink, 7.5" e-ink)
- [ ] Add touchscreen support
- [ ] Home Assistant integration
- [ ] Add more data sources (news, calendar)
- [ ] Mobile app for remote configuration

---

## 🙏 Acknowledgments

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) - Excellent TFT display library
- [GxEPD2](https://github.com/ZinggJM/GxEPD2) - E-paper display driver library
- [LVGL](https://lvgl.io/) - Embedded graphics library
- [ArduinoJson](https://arduinojson.org/) - JSON parsing library
- [WiFiManager](https://github.com/tzapu/WiFiManager) - WiFi configuration library

---

## 📞 Contact Us

- GitHub Issues: [Submit an issue](https://github.com/RockBase-iot/ESP32-Dashboard/issues)
- Email: support@rockbaseiot.com

---

<p align="center">
  Made with ❤️ by RockBase IoT Team
</p>
