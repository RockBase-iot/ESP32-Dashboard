# 隐私与安全说明

## 本地优先模型

- 设备直接从用户配置的公开端点拉取数据。
- 日历使用直接 ICS/iCal URL；不使用 OAuth、RockBase 账号或 RockBase 中转服务。
- 配置存储在 ESP32-S3 的 NVS 中。

## 敏感信息

- WiFi 密码不会通过 `/api/config` 返回。
- 日历 URL 和 API Key 只返回脱敏元数据。
- 投资组合持仓只返回是否已配置和数量，不返回明文。

## Web/AP 保护

- AP 恢复模式使用屏幕显示的临时 WPA2 密码。
- 能访问设备的客户端可以直接调用配置 API。
- 配置页面应仅在可信 AP 或私有局域网内使用。
- 不要将设备 HTTP 端口暴露到公网。

## 缓存行为

- Calendar、News、Finance、Economic 使用 current/previous 双版本缓存。
- 实时拉取失败时，页面可以显示 stale 缓存数据。
- 没有缓存时，页面显示明确的配置/空状态，而不是伪造实时数据。

## 使用建议

- Google Secret iCal URL 和投资组合持仓都应视为敏感信息。
- 如果设备丢失，建议重新生成日历 secret URL。
- 配置页面仅建议在可信局域网内访问，不要将设备 80 端口暴露到公网。
