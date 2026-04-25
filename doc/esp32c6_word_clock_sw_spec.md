# ESP32-C6 Word Clock – SW specifikace

## 0. Účel dokumentu
Tento dokument definuje návrh SW pro slovní hodiny založené na:
- **ESP32-C6**
- **WS2812B** adresovatelných LED
- pevně dané **8 × 20** matici znaků
- synchronizaci času přes **Wi-Fi + SNTP/NTP**
- volitelné aktualizaci FW přes **OTA**

Cíl dokumentu:
- dát jinému AI agentovi nebo vývojáři jednoznačné zadání
- rozdělit systém do malých, modulárních částí
- oddělit logiku času, mapování slov, LED rendering, konektivitu a správu konfigurace
- zajistit, aby šel SW později rozšířit bez rozbití jádra

---

## 1. Hlavní funkční požadavky

### 1.1 Povinné funkce
SW musí umět:
- zobrazovat aktuální čas na slovní matici
- automaticky synchronizovat čas přes Wi-Fi
- správně používat lokální čas včetně **letního/zimního času**
- po výpadku Wi-Fi pokračovat v běhu hodin z interního času MCU
- periodicky čas znovu synchronizovat
- řídit **160 LED** (8 × 20) přes WS2812B
- mít oddělenou logiku:
  - výpočtu času
  - výběru slov
  - převodu slov na LED indexy
  - fyzického vykreslení
- ukládat nastavení do persistentní paměti
- podporovat OTA update přes HTTPS

### 1.2 Doporučené funkce
SW by měl umět:
- první nastavení Wi-Fi bez rekompilace FW
- nastavit jas LED
- nastavit denní/noční režim jasu
- mít testovací režim pro ověření mechaniky
- mít diagnostický režim
- mít stavové logy přes UART

### 1.3 Nepovinné funkce pro budoucí rozšíření
Do první verze nejsou nutné, ale architektura je musí umožnit doplnit:
- ambientní senzor jasu
- webové administrační rozhraní
- ruční nastavení času bez Wi-Fi
- barevné režimy / efekty / animace
- alternativní faceplate nebo jinou slovní matici
- podpora jiného typu LED (např. SK6812)

---

## 2. Cílové chování hodin

### 2.1 Zobrazení času
Zobrazení je slovní, nikoli klasické HH:MM.

Použitá logika:
- `JE` / `JSOU` / `BUDE`
- hodina
- minutová část
- případně jednotka minut

### 2.2 Pravidla zobrazení

#### Přesná hodina
- `07:00 -> JE SEDM`
- `12:00 -> JE DVANÁCT`

#### Minuty 01–09
- `07:05 -> JE SEDM NULA PĚT`
- `07:08 -> JE SEDM NULA OSM`

#### Minuta 10
- `07:10 -> JE SEDM DESET`

#### Problematická část 11–19
Požadované pravidlo:
- `07:11 -> BUDE SEDM PATNÁCT`
- `07:12 -> BUDE SEDM PATNÁCT`
- `07:13 -> BUDE SEDM PATNÁCT`
- `07:14 -> BUDE SEDM PATNÁCT`
- `07:15 -> JE SEDM PATNÁCT`
- `07:16 -> BUDE SEDM DVACET`
- `07:17 -> BUDE SEDM DVACET`
- `07:18 -> BUDE SEDM DVACET`
- `07:19 -> BUDE SEDM DVACET`

#### Minuty 20–59
- `07:20 -> JE SEDM DVACET`
- `07:23 -> JE SEDM DVACET TŘI`
- `07:25 -> JE SEDM DVACET PĚT`
- `07:28 -> JE SEDM DVACET OSM`
- `07:30 -> JE SEDM TŘICET`
- `07:45 -> JE SEDM ČTYŘICET PĚT`
- `07:50 -> JE SEDM PADESÁT`
- `07:59 -> JE SEDM PADESÁT DEVĚT`

### 2.3 Gramatická pravidla prefixu
Pro hodiny používat:
- `JE` pro `1, 5, 6, 7, 8, 9, 10, 11, 12`
- `JSOU` pro `2, 3, 4`
- `BUDE` pouze v pásmech `11–14` a `16–19` minut

Poznámka:
`BUDE` zde neznamená přechod na další hodinu. Používá se pouze jako zkrácený čitelný kompromis pro problematický interval minut.

---

## 3. Matice znaků

## 3.1 Logická matice
Tato matice je zdroj pravdy pro SW testy a mapování slov.

```text
JEXJSOUXBUDEJEDNADVĚ
SEDMČTYŘIXPĚTŠESTŘIX
OSMDEVĚTDESETDVANÁCT
JEDENÁCTXNULAXDESETX
PATNÁCTXDVACETŘICETX
ČTYŘICETPADESÁTJEDNA
DVĚXTŘIXČTYŘIPĚTŠEST
DEVĚTXXXSEDMXXOSMXXX
```

Rozměr:
- řádky: `8`
- sloupce: `20`
- LED celkem: `160`

### 3.2 Fyzická / mechanická matice faceplate
Toto je text pro skutečnou desku, kde byly výplňové `X` nahrazeny vizuálně neutrálnějšími písmeny.

```text
JEZJSOULBUDEJEDNADVĚ
SEDMČTYŘIFPĚTŠESTŘIL
OSMDEVĚTDESETDVANÁCT
JEDENÁCTLNULAXDESETX
PATNÁCTIDVACETŘICETL
ČTYŘICETPADESÁTJEDNA
DVĚLTŘIVČTYŘIPĚTŠEST
DEVĚTVILSEDMNIOSMFIL
```

### 3.3 Důležitý princip
SW **nesmí** odvozovat aktivní slova z fyzické matice parsováním textu.

SW musí používat pouze:
- explicitně definované tokeny slov
- explicitně definované pozice slov
- explicitní mapu `token -> seznam buněk`

Tím se zabrání tomu, aby dekorativní písmena na pozicích původních `X` ovlivnila logiku.

---

## 4. Slovník tokenů

## 4.1 Prefixy
- `PREFIX_JE`
- `PREFIX_JSOU`
- `PREFIX_BUDE`

## 4.2 Hodiny
- `HOUR_1_JEDNA`
- `HOUR_2_DVE`
- `HOUR_3_TRI`
- `HOUR_4_CTYRI`
- `HOUR_5_PET`
- `HOUR_6_SEST`
- `HOUR_7_SEDM`
- `HOUR_8_OSM`
- `HOUR_9_DEVET`
- `HOUR_10_DESET`
- `HOUR_11_JEDENACT`
- `HOUR_12_DVANACT`

## 4.3 Minutové desítky / speciální slova
- `MIN_0_NULA`
- `MIN_10_DESET`
- `MIN_15_PATNACT`
- `MIN_20_DVACET`
- `MIN_30_TRICET`
- `MIN_40_CTYRICET`
- `MIN_50_PADESAT`

## 4.4 Jednotky minut
- `UNIT_1_JEDNA`
- `UNIT_2_DVE`
- `UNIT_3_TRI`
- `UNIT_4_CTYRI`
- `UNIT_5_PET`
- `UNIT_6_SEST`
- `UNIT_7_SEDM`
- `UNIT_8_OSM`
- `UNIT_9_DEVET`

---

## 5. Přesné pozice slov v logické matici
Formát souřadnic:
- řádky indexovat od `0`
- sloupce indexovat od `0`
- rozsah včetně obou krajů

## 5.1 Prefixy
- `PREFIX_JE` -> row `0`, cols `0..1`
- `PREFIX_JSOU` -> row `0`, cols `3..6`
- `PREFIX_BUDE` -> row `0`, cols `8..11`

## 5.2 Hodiny
- `HOUR_1_JEDNA` -> row `0`, cols `12..16`
- `HOUR_2_DVE` -> row `0`, cols `17..19`
- `HOUR_7_SEDM` -> row `1`, cols `0..3`
- `HOUR_4_CTYRI` -> row `1`, cols `4..8`
- `HOUR_5_PET` -> row `1`, cols `10..12`
- `HOUR_6_SEST` -> row `1`, cols `13..16`
- `HOUR_3_TRI` -> row `1`, cols `16..18`  
  Pozn.: sdílí písmeno `T` s koncem slova `ŠEST`, což je povoleno.
- `HOUR_8_OSM` -> row `2`, cols `0..2`
- `HOUR_9_DEVET` -> row `2`, cols `3..7`
- `HOUR_10_DESET` -> row `2`, cols `8..12`
- `HOUR_12_DVANACT` -> row `2`, cols `13..19`
- `HOUR_11_JEDENACT` -> row `3`, cols `0..7`

## 5.3 Minuty
- `MIN_0_NULA` -> row `3`, cols `9..12`
- `MIN_10_DESET` -> row `3`, cols `14..18`
- `MIN_15_PATNACT` -> row `4`, cols `0..6`
- `MIN_20_DVACET` -> row `4`, cols `8..13`
- `MIN_30_TRICET` -> row `4`, cols `13..18`  
  Pozn.: sdílí písmeno `T` s koncem slova `DVACET`, což je povoleno.
- `MIN_40_CTYRICET` -> row `5`, cols `0..7`
- `MIN_50_PADESAT` -> row `5`, cols `8..14`

## 5.4 Jednotky minut
- `UNIT_1_JEDNA` -> row `5`, cols `15..19`
- `UNIT_2_DVE` -> row `6`, cols `0..2`
- `UNIT_3_TRI` -> row `6`, cols `4..6`
- `UNIT_4_CTYRI` -> row `6`, cols `8..12`
- `UNIT_5_PET` -> row `6`, cols `13..15`
- `UNIT_6_SEST` -> row `6`, cols `16..19`
- `UNIT_9_DEVET` -> row `7`, cols `0..4`
- `UNIT_7_SEDM` -> row `7`, cols `8..11`
- `UNIT_8_OSM` -> row `7`, cols `14..16`

---

## 6. Renderovací pravidla

## 6.1 Výstup renderovací logiky
Jádro renderingu nesmí vracet „string“, ale množinu tokenů:

```text
07:23 -> { PREFIX_JE, HOUR_7_SEDM, MIN_20_DVACET, UNIT_3_TRI }
```

## 6.2 Převod času na tokeny
Implementovat čistou funkci:

```cpp
std::vector<TokenId> render_time_tokens(uint8_t hour24, uint8_t minute);
```

Pravidla:
- `hour24` převést na rozsah `1..12`
- `00` minut = pouze prefix + hodina
- `01..09` = prefix + hodina + `MIN_0_NULA` + jednotka
- `10` = prefix + hodina + `MIN_10_DESET`
- `11..14` = `PREFIX_BUDE` + hodina + `MIN_15_PATNACT`
- `15` = prefix + hodina + `MIN_15_PATNACT`
- `16..19` = `PREFIX_BUDE` + hodina + `MIN_20_DVACET`
- `20` = prefix + hodina + `MIN_20_DVACET`
- `21..29` = prefix + hodina + `MIN_20_DVACET` + jednotka
- `30` = prefix + hodina + `MIN_30_TRICET`
- `31..39` = prefix + hodina + `MIN_30_TRICET` + jednotka
- `40` = prefix + hodina + `MIN_40_CTYRICET`
- `41..49` = prefix + hodina + `MIN_40_CTYRICET` + jednotka
- `50` = prefix + hodina + `MIN_50_PADESAT`
- `51..59` = prefix + hodina + `MIN_50_PADESAT` + jednotka

## 6.3 Prefixová logika
Funkce:

```cpp
TokenId choose_prefix(uint8_t hour12, uint8_t minute);
```

Pravidla:
- pokud `minute` je v `11..14` nebo `16..19`, vrátit `PREFIX_BUDE`
- jinak:
  - `hour12` v `{2,3,4}` -> `PREFIX_JSOU`
  - jinak -> `PREFIX_JE`

---

## 7. Mapování buněk na LED indexy

## 7.1 Princip
Implementace musí oddělit:
- **logické souřadnice matice** `(row, col)`
- **fyzický index LED** v řetězu WS2812B

To umožní:
- změnit vedení LED bez změny logiky hodin
- změnit orientaci matice
- testovat logiku bez HW

## 7.2 Doporučený fyzický wiring
Doporučený default:
- matice `8 × 20`
- zapojení do „hada“ (serpentine)
- řádek 0 zleva doprava
- řádek 1 zprava doleva
- řádek 2 zleva doprava
- atd.

Funkce:

```cpp
uint16_t matrix_to_led_index(uint8_t row, uint8_t col);
```

Výpočet pro serpentine layout:
- sudý řádek: `index = row * COLS + col`
- lichý řádek: `index = row * COLS + (COLS - 1 - col)`

## 7.3 Zdroj pravdy
V produkčním kódu musí existovat jediný zdroj pravdy:
- buď generická funkce pro serpentine layout
- nebo explicitní statická tabulka `8 × 20`

Doporučení:
- použít explicitní tabulku `uint16_t led_index_map[8][20]`
- i když bude zpočátku vygenerovaná ze serpentine pravidla

Tím se usnadní pozdější mechanické změny.

---

## 8. Doporučená SW architektura

## 8.1 Architektonický styl
Použít **modulární event-driven architekturu**.

Vrstevnice systému:
1. **HAL / drivers**
2. **services**
3. **domain logic**
4. **application orchestration**

## 8.2 Moduly

### `app_main`
Zodpovědnost:
- start aplikace
- inicializace NVS
- inicializace event loop
- inicializace služeb
- spuštění hlavního orchestration modulu

### `clock_engine`
Zodpovědnost:
- převod času na sadu tokenů
- žádná znalost LED ani Wi-Fi
- čistě deterministická logika

### `word_layout`
Zodpovědnost:
- definice tokenů
- definice souřadnic slov
- převod tokenů na seznam buněk

### `led_renderer`
Zodpovědnost:
- práce s LED bufferem
- převod aktivních tokenů na aktivní LED
- aplikace jasu
- volání driveru WS2812B

### `time_service`
Zodpovědnost:
- získání systémového času
- převod na lokální čas
- informace o stavu synchronizace
- periodický resync

### `wifi_service`
Zodpovědnost:
- připojení do Wi-Fi
- reconnect logika
- publikace eventů o stavu připojení

### `provisioning_service`
Zodpovědnost:
- první konfigurace Wi-Fi
- reset provisioning dat
- lokální provisioning workflow

### `ota_service`
Zodpovědnost:
- kontrola dostupnosti nové verze
- stažení a aplikace FW přes HTTPS
- rollback-safe update flow

### `settings_store`
Zodpovědnost:
- uložení konfigurace do NVS
- načítání konfigurace po bootu

### `diagnostics_service`
Zodpovědnost:
- UART logy
- self-test
- LED test patterns
- build info / FW version

### `app_controller`
Zodpovědnost:
- drží runtime stav systému
- reaguje na eventy (Wi-Fi up/down, time sync, provisioning, OTA)
- rozhoduje, co se zobrazuje na panelu

---

## 9. Doporučená adresářová struktura

```text
main/
  app_main.cpp

components/
  app_controller/
    app_controller.cpp
    app_controller.hpp

  clock_engine/
    clock_engine.cpp
    clock_engine.hpp
    clock_rules.cpp
    clock_rules.hpp

  word_layout/
    word_layout.cpp
    word_layout.hpp
    word_tokens.hpp
    led_index_map.hpp

  led_renderer/
    led_renderer.cpp
    led_renderer.hpp
    led_effects.cpp
    led_effects.hpp

  time_service/
    time_service.cpp
    time_service.hpp

  wifi_service/
    wifi_service.cpp
    wifi_service.hpp

  provisioning_service/
    provisioning_service.cpp
    provisioning_service.hpp

  ota_service/
    ota_service.cpp
    ota_service.hpp

  settings_store/
    settings_store.cpp
    settings_store.hpp
    settings_model.hpp

  diagnostics_service/
    diagnostics_service.cpp
    diagnostics_service.hpp

  common/
    app_events.hpp
    app_types.hpp
    error_codes.hpp
```

---

## 10. Runtime stavový model

## 10.1 Stavy zařízení

### `BOOT`
- inicializace HW a služeb

### `NEEDS_PROVISIONING`
- zařízení nemá uloženou Wi-Fi konfiguraci
- spustí provisioning

### `CONNECTING_WIFI`
- pokus o připojení k AP

### `ONLINE_SYNCING_TIME`
- Wi-Fi připojena
- čeká se na první SNTP sync

### `RUNNING`
- běžný provoz hodin
- čas je dostupný
- Wi-Fi může být online i offline

### `OTA_IN_PROGRESS`
- provádí se aktualizace FW

### `ERROR`
- kritický stav
- zařízení má stále ukazovat rozumný fallback

## 10.2 Přechody
- `BOOT -> NEEDS_PROVISIONING` pokud nejsou credentials
- `BOOT -> CONNECTING_WIFI` pokud credentials existují
- `CONNECTING_WIFI -> ONLINE_SYNCING_TIME` po připojení
- `ONLINE_SYNCING_TIME -> RUNNING` po prvním time sync
- `RUNNING -> OTA_IN_PROGRESS` při startu OTA
- `OTA_IN_PROGRESS -> BOOT` po restartu
- `CONNECTING_WIFI -> RUNNING` je povoleno i bez Wi-Fi, pokud už byl čas dříve úspěšně synchronizován a interní čas běží dál

---

## 11. Časová synchronizace

## 11.1 Zdroj času
Použít:
- Wi-Fi
- SNTP/NTP
- lokální timezone přes POSIX `TZ` string

## 11.2 Požadované chování
- po prvním připojení k Wi-Fi provést initial sync
- po úspěšném sync označit systém jako `time_valid`
- následně provádět periodický resync
- při ztrátě Wi-Fi dál zobrazovat čas z interních timerů
- po rebootu bez napájení se čas považuje za neplatný, dokud neproběhne sync

## 11.3 Timezone
Použít český lokální čas včetně DST.
Doporučený `TZ` string pro ČR:

```text
CET-1CEST,M3.5.0/2,M10.5.0/3
```

Tento string musí být nastaven při startu přes:
- `setenv("TZ", ..., 1)`
- `tzset()`

## 11.4 Validita času
Funkce:

```cpp
bool time_service_is_valid();
```

Čas je validní, pokud:
- proběhl alespoň jeden úspěšný sync
- nebo zařízení neběželo přes power cycle a předtím už čas validní byl

## 11.5 Fallback zobrazení při nevalidním čase
Doporučení pro první verzi:
- celý panel zhasnout
- nebo zobrazit jednoduchý test pattern
- nebo periodicky blikat prefix `JE`

Důležité:
- tento fallback musí být jednoduchý a jednoznačně odlišitelný od platného času

---

## 12. Wi-Fi provisioning

## 12.1 Doporučený přístup
Pro první verzi doporučuji:
- **SoftAP provisioning**
- jednoduchá lokální konfigurační stránka
- po uložení credentials restart nebo reconnect flow

Důvod:
- jednodušší UX než hardcoded credentials
- menší složitost než BLE provisioning
- bez nutnosti recompliace FW při změně sítě

## 12.2 Požadavky
SW musí umět:
- detekovat, že Wi-Fi credentials nejsou uložené
- spustit provisioning režim
- uložit SSID + heslo do NVS
- provisioning po úspěšném uložení ukončit
- mít možnost credentials smazat

## 12.3 Reset provisioning dat
Požadovat jednu z cest:
- podržení tlačítka při bootu
- servisní funkce přes UART
- později HTTP endpoint

Minimální requirement:
- musí existovat způsob vrátit zařízení do provisioning režimu bez reflashe FW

---

## 13. OTA aktualizace

## 13.1 Cíl
SW musí podporovat OTA update bez fyzického přístupu k UART kabelu.

## 13.2 Požadované vlastnosti
- OTA přes HTTPS
- dual-slot partition layout
- validace FW před aktivací
- rollback-safe chování
- jasná identifikace build verze

## 13.3 Spouštění OTA
První verze může podporovat jednu z variant:
- ručně přes lokální HTTP endpoint
- ručně přes UART příkaz
- automatická kontrola nové verze po bootu

Doporučení pro první verzi:
- **ručně spuštěná OTA kontrola**
- automatické OTA až později

## 13.4 Partition table
Požadovat minimálně:
- `nvs`
- `otadata`
- `phy_init`
- `factory` nebo rovnou `ota_0`
- `ota_0`
- `ota_1`

Doporučení:
- pokud není speciální důvod pro `factory`, používat jen OTA slots a standardní OTA flow

---

## 14. LED rendering a vizuální chování

## 14.1 Barevnost
První verze:
- pouze jedna barva
- doporučeně teplá nebo neutrální bílá simulovaná přes RGB

## 14.2 Jas
Požadovat:
- globální brightness `0..255`
- oddělený day/night brightness profil

Např.:
- denní jas: `96`
- noční jas: `24`

## 14.3 Obnovování displeje
- display nemusí být přepočítáván každou smyčku
- stačí při změně minuty
- nebo při změně jasu / režimu / testu

## 14.4 Přechody
První verze:
- jednoduché přímé přepnutí
- žádné efekty nejsou nutné

Volitelné později:
- jemný fade out/in

---

## 15. Persistentní konfigurace

## 15.1 Co ukládat do NVS
- Wi-Fi credentials
- timezone string
- jas denní
- jas noční
- automatický přechod day/night enabled
- čas začátku nočního režimu
- čas konce nočního režimu
- OTA URL / release channel
- hostname zařízení
- build metadata cache

## 15.2 Datový model
Doporučená struktura:

```cpp
struct AppSettings {
    uint8_t brightness_day;
    uint8_t brightness_night;
    bool night_mode_enabled;
    uint8_t night_start_hour;
    uint8_t night_start_minute;
    uint8_t night_end_hour;
    uint8_t night_end_minute;
    char timezone[64];
    char ota_url[192];
    char hostname[64];
};
```

## 15.3 Defaults
Při prvním bootu použít bezpečné defaulty:
- timezone = `CET-1CEST,M3.5.0/2,M10.5.0/3`
- brightness_day = `96`
- brightness_night = `24`
- night_mode_enabled = `true`
- noční okno např. `22:00–06:00`

---

## 16. Diagnostika a servisní režimy

## 16.1 Povinné test režimy
SW musí umět tyto režimy:

### `LED_ALL_ON`
- rozsvítí všech 160 LED

### `LED_ROW_SCAN`
- rozsvítí vždy jeden řádek

### `LED_COL_SCAN`
- rozsvítí vždy jeden sloupec

### `WORD_SCAN`
- postupně rozsvěcí všechna definovaná slova

### `MATRIX_INDEX_SCAN`
- postupně rozsvěcí jednotlivé LED indexy
- užitečné pro hledání chyb v wiring mapě

## 16.2 Diagnostické informace přes UART
Povinně logovat:
- FW version
- build timestamp
- uptime
- Wi-Fi status
- IP address
- last successful time sync
- current timezone
- current brightness mode
- reset reason

---

## 17. Bezpečnostní a provozní požadavky

## 17.1 Minimální bezpečnostní požadavky
Pro produkční variantu doporučit:
- OTA pouze přes HTTPS
- validace server certifikátu
- oddělení dev a prod buildů

## 17.2 Doporučené rozšíření pro produkci
Volitelně připravit architekturu i pro:
- secure boot
- flash encryption
- OTA rollback
- anti-rollback policy

## 17.3 Chování při chybě
Při chybách:
- nesmí dojít k restart loop bez logu
- renderer musí mít bezpečný fallback
- ztráta Wi-Fi nesmí zastavit běh hodin
- chyba OTA nesmí znefunkčnit aktuální funkční firmware

---

## 18. Testovací strategie

## 18.1 Povinné unit testy

### `clock_engine` testy
Ověřit:
- všech 720 kombinací `12 × 60`
- správný prefix
- správnou hodinovou tokenizaci
- správnou minutovou tokenizaci
- problematický interval `11–19`

### `word_layout` testy
Ověřit:
- každý token má přesně definovanou pozici
- žádný token neleze mimo matici
- žádný token nemá prázdnou délku

### `reading_order` testy
Ověřit, že pro každý čas je výsledná čtecí posloupnost:
- prefix
- hodina
- minutové slovo
- případně jednotka minut

Test nesmí selhat ani kvůli překryvným slovům `ŠEST/TŘI` nebo `DVACET/TŘICET`, protože tyto překryvy jsou validní, pokud se nikdy neaktivují současně.

### `mapping` testy
Ověřit:
- každá aktivovaná buňka se převede na validní LED index
- žádné dva různé body nesmí mapovat na stejnou LED, pokud to není záměrně explicitně povolené

## 18.2 Povinné integrační testy
- boot bez Wi-Fi konfigurace
- provisioning
- připojení k Wi-Fi
- první SNTP sync
- ztráta Wi-Fi
- obnovení Wi-Fi
- OTA happy path
- OTA failure path

## 18.3 HW test checklist
- všechny LED svítí
- žádná LED není přehozená
- žádný řádek není zrcadlený proti očekávání
- mechanický přesvit je akceptovatelný
- slova se čtou správně ve vizuálním pořadí

---

## 19. Požadavky na implementační kvalitu

## 19.1 Kódové zásady
- žádná velká „god class“
- každá komponenta má jedinou hlavní odpovědnost
- žádná časová logika uvnitř LED driveru
- žádná znalost Wi-Fi uvnitř clock engine
- žádné hardcoded LED indexy rozeseté po kódu

## 19.2 Konfigurovatelnost
Musí jít snadno změnit:
- GPIO pro LED
- počet řádků/sloupců
- LED mapu
- timezone string
- SNTP server
- OTA URL

## 19.3 Build metadata
Build musí obsahovat:
- version string
- git commit hash
- build datetime
- build type (`dev` / `prod`)

---

## 20. Doporučené API kontrakty mezi moduly

## 20.1 `clock_engine`
```cpp
std::vector<TokenId> render_time_tokens(uint8_t hour24, uint8_t minute);
```

## 20.2 `word_layout`
```cpp
std::span<const CellCoord> token_to_cells(TokenId token);
```

## 20.3 `led_renderer`
```cpp
void led_renderer_clear();
void led_renderer_set_tokens(std::span<const TokenId> tokens);
void led_renderer_show();
void led_renderer_set_brightness(uint8_t value);
```

## 20.4 `time_service`
```cpp
bool time_service_init();
bool time_service_is_valid();
bool time_service_get_local_tm(struct tm* out_tm);
time_t time_service_get_epoch();
```

## 20.5 `wifi_service`
```cpp
bool wifi_service_init();
bool wifi_service_connect();
bool wifi_service_is_connected();
```

## 20.6 `ota_service`
```cpp
bool ota_service_check_for_update();
bool ota_service_start_update();
```

## 20.7 `settings_store`
```cpp
bool settings_load(AppSettings* out);
bool settings_save(const AppSettings* in);
bool settings_reset();
```

---

## 21. Doporučený boot flow

```text
1. init NVS
2. load settings
3. init logging
4. init LED renderer
5. init button / diagnostics hooks
6. check provisioning data
7. pokud nejsou credentials -> provisioning mode
8. pokud credentials jsou -> connect Wi-Fi
9. init SNTP + timezone
10. wait for first valid time (s timeoutem)
11. enter normal clock rendering loop
12. schedule periodic resync
13. expose manual OTA trigger
```

---

## 22. Normální render loop
Doporučení:
- perioda smyčky `200–500 ms`
- skutečný redraw jen při změně vstupního stavu

Redraw při:
- změně minuty
- změně jasu
- změně test režimu
- změně stavu validity času

Pseudo-flow:

```text
read local time
if time invalid -> fallback display
else
  tokens = render_time_tokens(hour, minute)
  led_renderer_set_tokens(tokens)
  led_renderer_show()
```

---

## 23. Co má být hotové v první implementaci

### Must-have
- řízení 160 WS2812B
- render času dle přesných pravidel
- 720 unit testů
- Wi-Fi připojení
- SNTP sync
- timezone + DST
- NVS settings
- OTA přes HTTPS
- diagnostické LED testy

### Should-have
- SoftAP provisioning
- day/night brightness
- build metadata

### Nice-to-have
- web UI
- fade přechody
- ambient sensor

---

## 24. Co nesmí implementace dělat
- nesmí míchat logiku času s hardware logikou LED
- nesmí vyhodnocovat slova parsováním mechanické matice
- nesmí mít prefixovou logiku rozesetou po více modulech
- nesmí být závislá na permanentním Wi-Fi spojení pro samotný chod hodin
- nesmí vyžadovat reflash FW jen kvůli změně Wi-Fi sítě

---

## 25. Doporučení pro předání dalšímu AI agentovi
Při implementaci požadovat tyto výstupy:
- samostatný modul `clock_engine` s unit testy
- samostatný modul `word_layout` s unit testy
- samostatný modul `led_renderer`
- stub/mock pro `time_service`
- stub/mock pro `wifi_service`
- integrační test pro 720 časů
- explicitní soubor s mapou tokenů a souřadnic
- explicitní soubor s LED index mapou

Dále požadovat, aby agent nejprve implementoval a otestoval:
1. tokenizační logiku času
2. mapování tokenů na buňky
3. mapování buněk na LED indexy
4. až potom HW a Wi-Fi vrstvy

---

## 26. Krátké závěrečné rozhodnutí
Pro první verzi doporučuji implementaci postavit takto:
- **ESP-IDF**
- **espressif/led_strip** pro WS2812B
- **Wi-Fi + SNTP** pro čas
- **POSIX TZ** pro letní/zimní čas
- **NVS** pro konfiguraci
- **HTTPS OTA** s dual-slot partition layoutem
- čisté oddělení na moduly `clock_engine`, `word_layout`, `led_renderer`, `time_service`, `wifi_service`, `ota_service`, `settings_store`

Tahle varianta je dost malá pro první implementaci, ale současně dost modulární pro další rozšíření.
