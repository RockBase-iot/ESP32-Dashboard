# 数据源说明

## 天气

- 默认：Open-Meteo。
- 用户配置：城市显示名、经纬度、单位。
- 必填：无需 API Key。

## 日历

- 默认：无。
- 启用日历类页面时必填：至少一个直接 ICS/iCal URL。
- 支持示例：Google Secret iCal、Outlook Published ICS、Apple Public Calendar，以及可转换为 HTTPS 的 `webcal://` 链接。
- 存储方式：作为本地 secret 保存；GET API 只返回脱敏元数据。

## 新闻

- 默认源：BBC News、Hacker News Front Page、NASA Breaking News。
- 用户配置：可选 RSS 2.0 或 Atom URL，用于替换默认源。
- 缓存行为：成功拉取的 feed 会缓存；失败时显示 stale 缓存或明确空状态。

## 金融行情

- 默认提供方：Stooq CSV。
- 默认代码：`AAPL.US,MSFT.US,BTCUSD`。
- 用户配置：可选 Stooq 股票/资产代码。
- 缓存行为：成功拉取的 CSV 会缓存；失败时可显示 stale 行情。
- 风险提示：延迟行情，不构成投资建议。

## 投资组合

- 默认：无。
- 启用 Portfolio 页面时必填：`AAPL:2:180:USD` 格式持仓。
- 敏感性：持仓只存储在本地，不通过 GET API 明文回显。

## 经济日历

- 默认：无。
- 启用 Economic Calendar 时必填：真实 RSS 或 ICS feed URL。
- `US`、`EU` 等地区标签本身不会自动映射为公开接口。
- 缓存行为：成功拉取的 feed 会缓存；失败时可显示 stale 数据。

## 世界时钟

- 默认：Shanghai、New York、London、Tokyo。
- 用户配置：最多四行 `Label | IANA_Timezone`。
- 空行不会显示。

## 专注时钟

- 默认：25 分钟专注、5 分钟休息、4 轮。
- 用户配置：标签、专注时长、休息时长、轮数。
- 不依赖外部数据源。

## 室内环境

- 默认：关闭。
- 仅在硬件传感器已安装并需要显示时启用。
