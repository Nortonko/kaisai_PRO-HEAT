# Changelog

Všetky podstatné zmeny v tomto projekte sú zaznamenané v tomto súbore.

Formát vychádza z [Keep a Changelog](https://keepachangelog.com/sk/1.0.0/)
a projekt používa [sémantické verzovanie](https://semver.org/lang/sk/).

## [1.0.1] – 2026-08-04

### Pridané
- **Periodické znovu-odoslanie stavu do HA.** Komponent teraz raz za
  `republish_interval` (predvolene 30 s) pošle do Home Assistant celý posledný
  známy stav — climate, senzory, switche aj selecty — aj keď sa nič nezmenilo.
  Rieši to miznutie/zostarnutie entít v HA („naposledy videný pred hodinami",
  „nedostupné"), ktoré vznikalo pri publikovaní len-pri-zmene v kombinácii so
  sporadickým vysielaním stavu z AC. **Vďaka tomu už netreba `heartbeat` ani
  `timeout` filtre v YAML-e.** Interval sa dá nastaviť (`republish_interval: 0s`
  ho vypne).
- **Switch**: Displej (podsvietenie, `0x1E`), Pípanie (`0x25`) a Jemný vietor /
  Soft wind (`0x26`). Povelová strana zachytená duálnym odpočúvaním oboch smerov
  UART naraz (ESP32-S2 sniffer).

### Odstránené
- **Switch Mute** (`0x73`). Duálny záchyt ukázal, že Mute nie je nezávislý toggle
  — appka ho posiela automaticky naviazaný na kombináciu ventilátor/režim (napr.
  HEAT + fan AUTO → mute=1), takže ako user-ovládateľný switch nedáva zmysel.

### Zmenené
- **Oprava interpretácie polí `0x64` a `0x65`.** Pôvodne označené ako zdanlivý
  výkon (VA) a prúd (A); na základe merania (233 V, príkon 250–300 W) ide v
  skutočnosti o **otáčky ventilátora vonkajšej jednotky (RPM, `0x64`)** a
  **frekvenciu kompresora (Hz, `0x65`)**. Konfiguračné kľúče: `apparent_power` →
  `fan_rpm`, `current` → `compressor_freq`.

### Poznámky
- **Reálny elektrický príkon (W) protokol nehlási** žiadnym poľom.
- Protokol povelov: po povele (`0x21`/`0A 0A`) AC do ~150 ms odpovie ACK
  (`0x23`/`80 0A`); stavový rámec (`0C 0C`) modul potvrdzuje `80 0C`.

## [1.0.0] – 2026-07-28

Prvé samostatné vydanie. Externý ESPHome komponent `kaisai_ac` na plné lokálne
ovládanie klimatizácií **Kaisai PRO HEAT+ (KRW-12TLHI / KRWB-12TLHO)** cez
zreverzovaný UART protokol s hlavičkou `0xA5` pri 115200 baud.

Overené kompiláciou (ESPHome 2026.7.3, ESP8266) aj reálnou prevádzkou na jednotke.

### Pridané
- Externý komponent `kaisai_ac` (climate platforma + switch + select).
- **Climate entita**: režim (OFF / chladenie / kúrenie / sušenie / ventilátor /
  auto), cieľová teplota 16–31 °C, aktuálna teplota, swing (OFF / vert / horiz /
  both).
- **Select `fan`**: 8 stupňov ventilátora (AUTO, MUTE, LOW, LOW-MID, MID,
  MID-HIGH, HIGH, TURBO).
- **Select `vane_vertical` / `vane_horizontal`**: polohy lamiel vrátane kývania,
  prúdových režimov a odstupňovaných polôh.
- **Switch**: ECO, Sleep, Health, Anti-mildew.
- Obojsmerná komunikácia: parsovanie stavových rámcov (`0C 0C`) aj skladanie
  povelov (`0A 0A`) s CRC16-XMODEM.
- Voliteľné periodické poll rámce (`poll_interval`).
- Robustné rámcovanie s kontrolou CRC a resynchronizáciou; rozostupené
  odosielanie povelov cez internú frontu.
- Dokumentácia protokolu v `README.md` a ukážkový config `kaisai-ac-example.yaml`.

### Poznámky
- Protokol `A5`/115200 bol zreverzovaný samostatne pre túto jednotku z prevádzky
  originálneho WiFi modulu TCWBRCU1 (Tuya WBR3). **Nie je** kompatibilný so
  staršími TCL projektmi na protokole `BB`/9600 (tclac, AC-hack).
- Hardvér: vyžaduje modul s 5 V UART úrovňou (napr. SMLIGHT SLWF-01Pro v2.1).
  Priame 3,3 V z holého ESP klimatizácia neprijme.
- Polia Display/Beeper zatiaľ nie sú zmapované (v tejto jednotke sa nevyskytli).

[1.0.1]: https://github.com/Nortonko/kaisai_PRO-HEAT/releases/tag/v1.0.1
[1.0.0]: https://github.com/Nortonko/kaisai_PRO-HEAT/releases/tag/v1.0.0
