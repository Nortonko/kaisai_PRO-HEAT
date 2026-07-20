# Changelog

Všetky podstatné zmeny v tejto vetve (fork projektu
[I-am-nightingale/tclac](https://github.com/I-am-nightingale/tclac))
sú zaznamenané v tomto súbore.

Formát vychádza z [Keep a Changelog](https://keepachangelog.com/sk/1.0.0/)
a projekt používa [sémantické verzovanie](https://semver.org/lang/sk/).

## [1.0.0] – 2026-07-20

Prvé vydanie prispôsobené pre **Kaisai PRO HEAT+ (KRW-12TLHI / KRWB-12TLHO)**
na WiFi module **SMLIGHT SLWF-01Pro v2.1**.

### Pridané
- Slovenský preklad `README.md` vrátane hlavičky s typovým označením jednotky.
- Sekcia s upozorneniami pred inštaláciou (hardvér, rozostavenie pinov konektora,
  minimálna verzia ESPHome, dĺžka správ).
- Popis platformy a zapojenia pre SLWF-01Pro v2.1 (ESP-12E, GPIO12/GPIO14).
- Tento súbor `CHANGELOG.md`.

### Zmenené
- `Sample_conf.yaml` aj `TCL-Conditioner.yaml` preložené do slovenčiny (komentáre).
- Platforma prepnutá na `esp8266: board: esp12e` (predtým ESP-01S / nodemcu-32s).
- UART piny zmenené na `uart_tx: GPIO12` a `uart_rx: GPIO14` podľa hardvéru
  SLWF-01Pro v2.1 (predtým GPIO3/GPIO1, čo platí len pre ESP-01S).
- `packages: url:` presmerované na tento fork
  (`https://github.com/Nortonko/kaisai_PRO-HEAT.git`) namiesto pôvodného repozitára.
- Predvolené `device_name` / `humanly_name` nastavené na Kaisai PRO HEAT+,
  vrátane poznámky o 15-znakovom limite pre `device_name`.

### Bezpečnosť
- Citlivé polia `recovery_pass`, `ota_pass` a `api_key` presunuté na `!secret`
  namiesto natvrdo zapísaných hodnôt v ukážkových súboroch.

### Poznámky
- Kód samotného komponentu (`components/tclac`) nebol menený — protokol a všetka
  zásluha zostávajú dielom pôvodného autora.
- Kompatibilita bola overená proti ESPHome 2026.6.x / 2026.7.0; komponent vyžaduje
  minimálne 2026.4.0.
- Fyzické rozostavenie pinov konektora Kaisai/TCL nie je overené meraním — pred
  pripojením modulu skontroluj multimetrom GND / +5 V / TX / RX.

[1.0.0]: https://github.com/Nortonko/kaisai_PRO-HEAT/releases/tag/v1.0.0
