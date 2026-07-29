# Changelog

Všetky podstatné zmeny v tomto projekte sú zaznamenané v tomto súbore.

Formát vychádza z [Keep a Changelog](https://keepachangelog.com/sk/1.0.0/)
a projekt používa [sémantické verzovanie](https://semver.org/lang/sk/).

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

[1.0.0]: https://github.com/Nortonko/kaisai_PRO-HEAT/releases/tag/v1.0.0
