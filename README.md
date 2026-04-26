# qclocks

České slovní hodiny na ESP32-C6 s maticí 160 WS2812B LED (8 × 20).

## Hardware

| Komponenta | Detail |
|---|---|
| MCU | ESP32-C6 |
| LED | 160 × WS2812B, serpentinové zapojení 8 × 20 |
| Flash | 2 MB |
| Připojení | WiFi 2.4 GHz, USB-JTAG (konzole + flash) |

## Architektura

Aplikace je rozdělena do samostatných komponent:

- **clock_engine** – převod `(hodina, minuta)` na sadu tokenů (česká slovní reprezentace času)
- **word_layout** – mapování tokenů na indexy LED v matici
- **led_renderer** – ovládání LED pásku (stub; RMT driver v Phase 4)
- **wifi_service** – připojení k WiFi, TX výkon 8 dBm, modem sleep
- **time_service** – synchronizace času přes SNTP (pool.ntp.org + Cloudflare), časová zóna z NVS
- **provisioning_service** – detekce WiFi credentials v NVS
- **ota_service** – HTTPS OTA update z GitHub Releases, porovnání verzí před stažením
- **settings_store** – nastavení v NVS (jas den/noc, noční režim, časová zóna, OTA URL)
- **diagnostics_service** – boot info (heap, uptime, reset reason) + interaktivní konzole
- **app_controller** – stavový automat zařízení, orchestrace všech služeb

## Stavový automat

```
BOOT → NEEDS_PROVISIONING (chybí WiFi credentials)
     → CONNECTING_WIFI → ONLINE_SYNCING_TIME → RUNNING
                                              → OTA_IN_PROGRESS → restart
```

## OTA aktualizace

Firmware se aktualizuje automaticky při každém zapnutí:

1. Device se připojí k WiFi a synchronizuje čas
2. Po 30 sekundách stáhne hlavičku nového firmware z GitHub Releases
3. Porovná verzi s běžící verzí (`esp_app_desc_t.version` = git tag)
4. Pokud je nová verze → stáhne celý firmware, flashuje, restartuje
5. Pokud stejná verze → nic nestahuje, pokračuje normálně

OTA URL se nastavuje do NVS přes `tools/provision.ps1`.

Manuální spuštění OTA je také možné přes konzoli příkazem `ota`.

## Konzole (USB-JTAG)

Dostupná přes `idf.py monitor`. Podporované příkazy:

| Příkaz | Akce |
|---|---|
| `info` | Zobrazí IDF verzi, uptime, free heap, reset reason |
| `ota` | Spustí OTA aktualizaci okamžitě |
| `reset_wifi` | Vymaže WiFi credentials z NVS a restartuje |

## Partition table (2 MB flash)

| Partition | Offset | Velikost |
|---|---|---|
| nvs | 0x009000 | 24 KB |
| otadata | 0x00F000 | 8 KB |
| phy_init | 0x011000 | 4 KB |
| ota_0 | 0x020000 | 960 KB |
| ota_1 | 0x110000 | 960 KB |

## První instalace

```powershell
# 1. Build a flash (nutný fyzický přístup k USB)
. C:\esp\v6.0\esp-idf\export.ps1
idf.py build flash

# 2. Zápis WiFi credentials a OTA URL do NVS
.\tools\provision.ps1 `
  -SSID "NazevSite" `
  -Password "heslo" `
  -OtaUrl "https://github.com/Nowass/qclocks/releases/latest/download/qclocks.bin"
```

Po prvním zapnutí se device připojí k WiFi a při dalších bootech se aktualizuje automaticky.

## CI/CD

GitHub Actions workflow `.github/workflows/release.yml` se spustí při tagu `v*.*.*`:
- Buildí firmware pomocí `espressif/esp-idf-ci-action@v1` (ESP-IDF v6.0, target esp32c6)
- Uploaduje `qclocks.bin` do GitHub Release
- Verze firmware = git tag (embedded do binárky přes `PROJECT_VER` v CMake)

```powershell
git tag v1.0.5
git push origin v1.0.5
```

## Vývoj

```powershell
# Build
. C:\esp\v6.0\esp-idf\export.ps1
idf.py build

# Flash + monitor
idf.py flash monitor

# Unit testy (nativní host, MinGW)
cd tests
cmake -G Ninja -DCMAKE_CXX_COMPILER=D:/Qt/Tools/mingw1310_64/bin/g++.exe ..
ninja
.\qclocks_tests.exe
```
