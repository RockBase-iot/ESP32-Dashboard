# NM-EPD-420 Dashboard User Guide

## First Setup

1. Flash the complete image with `pio run -e nm-display-420 -t upload_all`.
2. If WiFi is not configured, the device enters setup/AP recovery.
3. Connect to the displayed AP SSID. The temporary WPA2 key is shown on the e-paper screen.
4. Open `http://192.168.4.1`.
5. Set the WiFi SSID/password and device preferences.
6. Save Device and restart.

## Daily Use

- Page 0 is the legacy `PageWeather400x300::draw()` weather home.
- Managed pages are configured in the Web portal under Pages.
- BOOT short press moves to the next page while awake.
- USER short press moves to the previous page while awake.
- Focus Clock uses USER long press to start/stop the focus session.
- The lower-left footer shows the current device IP when WiFi is connected.

## Web Portal

- Open `http://<device-ip>` while the device is awake or during the configured `PortalSec` window.
- Configuration changes are available directly from the device AP or local network.
- Only expose the configuration page on a trusted network.
- Save Pages updates enabled pages, order, and rotation metadata.
- Save Source updates only that source group.
- Save Device updates WiFi, units, time/date formats, and schedule.

## Notes

- Weather uses Open-Meteo by default and does not require an API key.
- Calendar requires direct ICS/iCal links; OAuth and cloud relay are intentionally not used.
- Portfolio positions are sensitive and are not returned by GET APIs.
- Cached Calendar, News, Finance, and Economic data may be shown as stale when the latest fetch fails.
