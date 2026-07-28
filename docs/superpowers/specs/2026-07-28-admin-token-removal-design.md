# Admin Token Removal Design

Date: 2026-07-28
Target release: current maintenance release

## Objective

Remove the unused Admin Access and Admin Token feature from firmware, tests, CI, and user-facing documentation. Existing devices must also delete the legacy `AdminTokH` NVS value without changing any unrelated configuration.

Notes and Milestones CRUD is explicitly deferred to the next minor release. Its approved persistence direction is a dedicated, bounded NVS content store with overwrite-on-capacity behavior.

## Scope

This change includes:

- Remove the admin authentication implementation and its tests.
- Remove `adminTokenHash` from `AppConfig`.
- Remove the current admin token key/default definitions and load/save behavior.
- Remove admin token handling from the Web API configuration path.
- Remove Admin Access and Admin Token instructions from English and Chinese documentation.
- Remove the admin authentication test from CI and documented test commands.
- Delete the legacy `AdminTokH` key from deployed devices through an idempotent migration.

This change does not add replacement authentication, alter WiFi/AP behavior, or change any other stored setting.

## NVS Migration

The legacy key belongs to namespace `weather_epd` and is named `AdminTokH`.

`Settings` will gain a narrowly scoped key-removal operation. During `loadAppConfig()` startup, a read-write settings handle will attempt to erase the literal legacy key. The migration commits only when the key existed and was successfully erased. A missing key is treated as success and causes no flash write.

The legacy literal remains private to the migration implementation rather than remaining part of the active NVS schema. This makes the distinction between migration compatibility and supported configuration explicit.

The migration must not:

- erase the full `weather_epd` namespace;
- increment or reset dashboard page configuration versions;
- rewrite WiFi credentials, page state, source configuration, or calendar secrets;
- fail application startup when the key is absent.

## Source Changes

Delete:

- `src/app/security/admin_auth.cpp`
- `src/app/security/admin_auth.h`
- `test/test_admin_auth/test_main.cpp`

Update:

- `src/app/config/app_config.h`
- `src/app/config/app_config.cpp`
- `src/app/config/nvs_table.h`
- `src/app/config/settings.h`
- `src/app/config/settings.cpp`
- `src/app/web/web_server.cpp`
- `.github/workflows/build.yml`
- `README.md`
- `README_cn.md`
- English and Chinese user/recovery documentation containing Admin Token guidance

## Web Behavior

The existing configuration APIs remain directly usable from the device AP and trusted local network. No Admin Token field, session token, pairing prompt, or authorization header is required.

Documentation must state that anyone who can reach the device configuration page can change its settings, so the configuration interface should only be exposed on trusted networks.

## Error Handling and Diagnostics

Failure to remove the legacy key is logged as a warning but does not block boot. Successful removal is logged once because subsequent starts no longer find the key. Logs must not contain the former token hash or any secret value.

## Verification

Verification includes:

1. Unit tests for removing an existing key and treating a missing key as success.
2. Existing configuration and page-manager tests continue to compile.
3. Repository search finds no active `adminTokenHash`, `NVS_KEY_ADMIN_TOKEN_HASH`, `DEFAULT_ADMIN_TOKEN_HASH`, `admin_auth`, Admin Access, Admin Token, or `test_admin_auth` references, except the private migration literal and this design document.
4. `nm-display-420` firmware build succeeds.
5. LittleFS image and merged release image still build successfully.

## Deferred Notes and Milestones

The next minor release will implement manual web CRUD for Notes and Milestones. Both collections will use a dedicated NVS content namespace, bounded records, validation, and deterministic overwrite of the oldest eligible record after capacity is reached. That feature is not part of this maintenance change.
