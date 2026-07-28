# Recovery and Reflash

## Complete Flash

Use the complete target whenever web assets changed or the portal reports missing assets:

```bash
pio run -e nm-display-420 -t upload_all --upload-port COM12
```

`upload_all` uploads firmware and the LittleFS image that contains `/index.html.gz`.

## Release Single-Binary Image

For end users or release distribution, generate one merged binary that includes
the bootloader, partition table, OTA data initializer, app firmware, and
LittleFS image:

```bash
pio run -e nm-display-420 -t release_bin
```

The output file is flashed at address `0x0`:

```text
release/esp32-dashboard-nm-epd-420-v0.1.1.bin
```

Change the root `VERSION` file before building a new public release.

## Web Assets Missing

Symptom:

```text
Web assets not uploaded. Run: pio run -e nm-display-420 -t upload_all
```

Fix:

```bash
pio run -e nm-display-420 -t upload_all --upload-port <PORT>
```

## Manual BOOT Mode

1. Hold BOOT.
2. Tap RESET or power-cycle while holding BOOT.
3. Release BOOT after the serial port appears.
4. Run `upload_all` with the detected COM port.

## Factory-Like Recovery

- Reflash with `upload_all`.
- Open AP recovery mode.
- Set WiFi again if NVS was erased.
- Reconfigure data sources that are stored as local secrets.

## Build Gates Before Release

```bash
pio test -e nm-display-420 -f test_legacy_config_migration --without-uploading --without-testing
pio test -e nm-display-420 -f test_source_runtime_cache --without-uploading --without-testing
pio run -e nm-display-420
pio run -e nm-display-420 -t buildfs
pio run -e nm-display-420 -t release_bin
```
