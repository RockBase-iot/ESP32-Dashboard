# 恢复与重新烧录流程

## 完整烧录

网页资源发生变化，或网页提示缺少资源时，使用完整烧录：

```bash
pio run -e nm-display-420 -t upload_all --upload-port COM12
```

`upload_all` 会同时上传应用固件和 LittleFS 镜像，LittleFS 中包含 `/index.html.gz` 网页配置界面。

## 发布用单文件固件

面向用户发布时，可以生成一个从 `0x0` 地址写入的合并固件，内含 bootloader、分区表、OTA data 初始化块、应用固件和 LittleFS 镜像：

```bash
pio run -e nm-display-420 -t release_bin
```

输出文件位于 `release/` 目录，命名格式为：

```text
esp32-dashboard-设备名-版本号.bin
```

例如：

```text
release/esp32-dashboard-nm-epd-420-v0.1.1.bin
```

发布新版本前，请先更新根目录的 `VERSION` 文件。

## Web assets not uploaded

现象：

```text
Web assets not uploaded. Run: pio run -e nm-display-420 -t upload_all
```

处理：

```bash
pio run -e nm-display-420 -t upload_all --upload-port <PORT>
```

## 手动进入 BOOT 模式

1. 按住 BOOT。
2. 按 RESET，或在按住 BOOT 的同时重新上电。
3. 串口出现后松开 BOOT。
4. 使用检测到的 COM 口执行 `upload_all` 或烧录发布用单文件固件。

## 类出厂恢复

- 使用 `upload_all` 重新烧录，或使用 release 目录中的单文件固件从 `0x0` 地址写入。
- 进入 AP 恢复模式。
- 如果 NVS 已擦除，需要重新设置 WiFi。
- 重新配置保存在本地 secret 中的数据源。

## 发布前构建门禁

```bash
pio test -e nm-display-420 -f test_legacy_config_migration --without-uploading --without-testing
pio test -e nm-display-420 -f test_source_runtime_cache --without-uploading --without-testing
pio run -e nm-display-420
pio run -e nm-display-420 -t buildfs
pio run -e nm-display-420 -t release_bin
```
