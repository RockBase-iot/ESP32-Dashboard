# NM Display 4.2" (ESP32-S3) — 低功耗硬件设计说明

> 适用板卡：NM Display 4.2 Inch（hw-rev2）  
> MCU：ESP32-S3（arduino-esp32 6.13.0 / IDF 5.x）  
> 本文档描述 rev2 相对 rev1 在**低功耗相关的硬件改动**，以及固件对应的实现模式，供其他项目 agent 参考。

---

## 一、硬件改动概述

### 1. 新增模块使能脚（active HIGH）

rev2 在每个主要外设模块上增加了独立的硬件使能引脚。  
平时由固件拉低关断，仅在需要使用该模块时拉高使能，用完后再拉低。

| 引脚 | 宏定义 | 连接模块 | 使能电平 |
|------|--------|----------|---------|
| IO47 | `PIN_LORA_EN` | LoRa 模块（HA-RA62 / SX1262）电源使能 | HIGH |
| IO44 | `PIN_CODEC_EN` | ES8311 音频 Codec 电源使能 | HIGH |
| IO43 | `PIN_ADC_EN` | 电池电压 ADC 采样电路使能 | HIGH |

> **rev1 对比**：rev1 无上述三个使能脚，LoRa / Codec / ADC 电路始终通电，产生持续静态电流。

### 2. 电池电压 ADC 采样电路

- IO43 拉高后，通过模拟开关/MOSFET 接通分压电阻网络。
- IO3 为 ADC 输入引脚（ESP32-S3 ADC1_CH2），读取电池分压后的电压值。
- 分压比默认配置为 2:1（`BATT_ADC_DIV = 2`，即 `Vbatt = ADC_mV × 2`），根据实际电路调整。

```c
#define PIN_BATT_ADC   3   // Battery voltage sense input (via resistor divider)
#define PIN_ADC_EN    43   // Battery ADC circuit enable (HIGH = on)
#define BATT_ADC_DIV   2   // Divider ratio: ADC_mV * BATT_ADC_DIV = battery mV
```

### 3. EPD CS 引脚变更

- rev1：`PIN_EPD_CS = IO3`（与电池 ADC 引脚冲突）
- rev2：`PIN_EPD_CS = IO46`（让出 IO3 给电池 ADC）

### 4. 去掉 WS2812 LED

- rev1 IO47 用于 WS2812 数据线，rev2 该 IO 改为 LoRa 使能脚。
- WS2812 硬件永久移除，节省 RMT 驱动功耗和 LED 本身静态电流。

---

## 二、已有使能脚（rev1 即有）

| 引脚 | 宏定义 | 连接模块 | 使能电平 |
|------|--------|----------|---------|
| IO41 | `PIN_PA_CTRL` | NS4150B 功放 | HIGH |
| IO40 | `PIN_TEMP_CTL` | AHT20 温湿度传感器电源 | HIGH |
| IO12 | `PIN_LORA_RST` | LoRa 复位（LOW = 复位/低功耗） | — |
| IO8  | `PIN_LORA_NSS` | LoRa SPI CS（LOW = 选中，HIGH = 空闲） | — |

---

## 三、固件低功耗实现模式

### 3.1 模块使能管理原则

每个测试函数在**进入时**拉高对应使能脚，**退出时**拉低，做到最短时间通电：

```cpp
// 示例：T4 ES8311 CODEC 测试
inline TestResult runTestT4(...) {
    pinMode(PIN_CODEC_EN, OUTPUT);
    digitalWrite(PIN_CODEC_EN, HIGH);  // 使能 codec
    delay(10);                          // 等待上电稳定
    // ... 测试逻辑 ...
    _es8311_enter_powerdown();
    digitalWrite(PIN_CODEC_EN, LOW);   // 关闭 codec
    return verdict ? TestResult::PASS : TestResult::FAIL;
}
```

同样模式用于：`PIN_LORA_EN`（T10 LoRa）、`PIN_ADC_EN`（T7 电池 ADC）、`PIN_CODEC_EN`（T5 DMIC）。

### 3.2 进入深度睡眠前的 GPIO 锁存

测试全部完成后，调用 `_enterDeepSleep()`：

**步骤一：将所有使能脚强制写入关闭状态**

```cpp
pinMode(PIN_PA_CTRL,  OUTPUT); digitalWrite(PIN_PA_CTRL,  LOW);  // 功放 off
pinMode(PIN_LORA_EN,  OUTPUT); digitalWrite(PIN_LORA_EN,  LOW);  // LoRa off
pinMode(PIN_CODEC_EN, OUTPUT); digitalWrite(PIN_CODEC_EN, LOW);  // Codec off
pinMode(PIN_ADC_EN,   OUTPUT); digitalWrite(PIN_ADC_EN,   LOW);  // ADC off
pinMode(PIN_TEMP_CTL, OUTPUT); digitalWrite(PIN_TEMP_CTL, LOW);  // AHT20 off
pinMode(PIN_LORA_RST, OUTPUT); digitalWrite(PIN_LORA_RST, LOW);  // LoRa 保持复位
pinMode(PIN_LORA_NSS, OUTPUT); digitalWrite(PIN_LORA_NSS, HIGH); // SPI CS 空闲高
delay(10); // 等待 GPIO 输出稳定
```

**步骤二：锁存引脚电平（`gpio_hold_en`）**

```cpp
#include <driver/gpio.h>

gpio_hold_en((gpio_num_t)PIN_PA_CTRL);
gpio_hold_en((gpio_num_t)PIN_LORA_EN);
gpio_hold_en((gpio_num_t)PIN_CODEC_EN);
gpio_hold_en((gpio_num_t)PIN_ADC_EN);
gpio_hold_en((gpio_num_t)PIN_TEMP_CTL);
gpio_hold_en((gpio_num_t)PIN_LORA_RST);
gpio_hold_en((gpio_num_t)PIN_LORA_NSS);
gpio_deep_sleep_hold_en();  // 跨深睡眠保持锁存（ESP32-S3 必须）
```

> `gpio_hold_en()` 捕获当前输出电平并在 CPU 关闭后继续驱动该电平。  
> `gpio_deep_sleep_hold_en()` 使该锁存在深睡眠期间 IO 电源域断电时依然有效（ESP32-S3 特有，ESP32-original 行为略有不同）。

**步骤三：EPD 休眠（保持画面，关闭控制器）**

```cpp
// GxEPD2 提供的 hibernate()：向 EPD 控制器发送深度睡眠命令
// 面板保留最后刷新的画面，控制器进入极低功耗状态
_display.hibernate();
```

**步骤四：ESP32-S3 进入深度睡眠**

```cpp
#include <esp_sleep.h>

// IO0（BOOT 键）是 RTC GPIO，可作为 EXT0 唤醒源
esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // LOW 电平唤醒
Serial.flush();   // 确保串口日志全部输出后再睡眠
esp_deep_sleep_start();
```

---

## 四、锁存引脚状态汇总

| 引脚 | 深睡眠期间电平 | 效果 |
|------|--------------|------|
| IO41 PIN_PA_CTRL  | **LOW**  | 功放断电 |
| IO47 PIN_LORA_EN  | **LOW**  | LoRa 模块断电 |
| IO44 PIN_CODEC_EN | **LOW**  | ES8311 断电 |
| IO43 PIN_ADC_EN   | **LOW**  | ADC 电路断电 |
| IO40 PIN_TEMP_CTL | **LOW**  | AHT20 断电 |
| IO12 PIN_LORA_RST | **LOW**  | LoRa 保持复位态 |
| IO8  PIN_LORA_NSS | **HIGH** | SPI CS 空闲，不触发外设 |

---

## 五、唤醒行为

- 唤醒源：按下 BOOT 键（IO0 低电平，RTC GPIO）。
- 唤醒后 ESP32-S3 从 `setup()` 重新开始执行，即重新运行完整测试序列。
- **注意**：唤醒后需在 `init` 阶段调用 `gpio_hold_dis()` 解除各引脚的 hold，再重新初始化为工作状态，否则 `digitalWrite` 无效。

```cpp
// 唤醒后 init 阶段释放 hold（示例）
gpio_hold_dis((gpio_num_t)PIN_PA_CTRL);
gpio_hold_dis((gpio_num_t)PIN_LORA_EN);
// ... 其余引脚 ...
```

---

## 六、关键头文件 / 依赖

```cpp
#include <esp_sleep.h>    // esp_deep_sleep_start, esp_sleep_enable_ext0_wakeup
#include <driver/gpio.h>  // gpio_hold_en, gpio_hold_dis, gpio_deep_sleep_hold_en
```

platformio.ini 无需额外库，上述头文件由 `espressif32` framework 内置提供。

---

## 七、参考实现

- 本项目：`src/test_runner.cpp` → `_enterDeepSleep()`
- 同系列项目参考：`ESP32-Dashboard/src/bsp/nm_display_420/Board.cpp` → `deepSleep()` 方法（相同板卡，相同 GPIO hold 模式）
