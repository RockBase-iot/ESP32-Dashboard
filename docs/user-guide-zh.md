# NM-EPD-420 Dashboard 用户手册

## 首次配置

1. 使用 `pio run -e nm-display-420 -t upload_all` 完整烧录固件和网页资源。
2. 如果尚未配置 WiFi，设备会进入配置/AP 恢复模式。
3. 连接屏幕上显示的 AP 热点，临时 WPA2 密码也会显示在墨水屏上。
4. 打开 `http://192.168.4.1`。
5. 设置 WiFi 名称/密码和设备参数。
6. 保存 Device 页面并重启。

## 日常使用

- 第 0 页是兼容保留的 `PageWeather400x300::draw()` 天气首页。
- 其他托管页面在网页端 Pages 页面中开启、排序和配置轮转。
- 设备唤醒状态下，短按 BOOT 切换到下一页。
- 设备唤醒状态下，短按 USER 切换到上一页。
- Focus Clock 页面中，长按 USER 用于开始/结束专注时钟。
- 页脚左下角会显示当前设备 IP 地址。

## Web 配置页面

- 在设备唤醒或 `PortalSec` 配置窗口内访问 `http://<设备IP>`。
- 可直接通过设备 AP 或局域网进行保存、同步和重启操作。
- 配置页面应仅在可信局域网中使用。
- Save Pages 用于保存页面启用、排序和轮转配置。
- Save Source 只保存对应数据源。
- Save Device 保存 WiFi、单位、时间/日期格式和刷新计划。

## 说明

- 天气默认使用 Open-Meteo，不需要 API Key。
- 日历使用直接 ICS/iCal 链接；当前设计不使用 OAuth，也不通过 RockBase 云中转。
- 投资组合持仓属于敏感数据，不会通过 GET API 明文返回。
- Calendar、News、Finance、Economic 在拉取失败时会优先显示缓存，并标记为 stale。
