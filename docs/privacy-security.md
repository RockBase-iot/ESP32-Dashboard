# Privacy and Security

## Local-First Model

- The device fetches data directly from user-configured public endpoints.
- Calendar links are direct ICS/iCal URLs; no OAuth, RockBase account, or RockBase relay is required.
- Configuration is stored in NVS on the ESP32-S3.

## Secrets

- WiFi password is never returned by `/api/config`.
- Calendar URLs and API keys are returned only as masked metadata.
- Portfolio positions are returned only as configured/count metadata.

## Web/AP Protection

- AP recovery mode uses a temporary WPA2 key displayed on the e-paper screen.
- Configuration APIs are directly available to clients that can reach the device.
- Use the configuration interface only on a trusted AP or private LAN.
- Do not expose the device HTTP port to the public internet.

## Cache Behavior

- Calendar, News, Finance, and Economic payloads use current/previous cache versions.
- If live fetch fails, the UI may show stale cached data.
- If no cache exists, pages show a clear setup or empty state instead of fake live data.

## Operational Guidance

- Treat Google Secret iCal URLs and portfolio positions as sensitive.
- Rotate calendar secret URLs if the device is lost.
- Use a private LAN for setup and avoid exposing port 80 to the public internet.
