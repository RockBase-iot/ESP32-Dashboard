# Data Sources

## Weather

- Default: Open-Meteo.
- User input: city label, latitude, longitude, and units.
- Required: no API key.

## Calendar

- Default: none.
- Required when calendar pages are enabled: at least one direct ICS/iCal URL.
- Supported examples: Google Secret iCal, Outlook published ICS, Apple public calendar, and `webcal://` links normalized to HTTPS.
- Stored as local secrets; GET APIs return only masked metadata.

## News

- Default feeds: BBC News, Hacker News front page, NASA Breaking News.
- User input: optional replacement RSS 2.0 or Atom feed URLs.
- Cache behavior: latest successful feed payload is cached; failures show cached stale data or an empty state.

## Finance

- Default provider: Stooq CSV.
- Default symbols: `AAPL.US,MSFT.US,BTCUSD`.
- User input: optional replacement Stooq symbols.
- Cache behavior: last successful quote CSV is cached and may be shown as stale.
- Disclaimer: delayed quotes, not investment advice.

## Portfolio

- Default: none.
- Required when Portfolio is enabled: positions in `AAPL:2:180:USD` format.
- Sensitive: positions are stored locally and are not echoed by GET APIs.

## Economic Calendar

- Default: none.
- Required when Economic Calendar is enabled: a real RSS or ICS feed URL.
- Region labels such as `US` or `EU` are not live data sources by themselves.
- Cache behavior: last successful feed is cached and may be shown as stale.

## World Clock

- Default: Shanghai, New York, London, Tokyo.
- User input: up to four rows of `Label | IANA_Timezone`.
- Empty rows are skipped.

## Focus Clock

- Default: 25-minute focus, 5-minute break, 4 sessions.
- User input: label and focus/break/session lengths.
- No external data source.

## Indoor Climate

- Default: disabled.
- User input: enable only when the hardware sensor is installed and expected.
