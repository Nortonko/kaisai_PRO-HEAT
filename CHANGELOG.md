# Changelog

Všetky podstatné zmeny v tejto vetve (fork projektu
[I-am-nightingale/tclac](https://github.com/I-am-nightingale/tclac))
sú zaznamenané v tomto súbore.

Formát vychádza z [Keep a Changelog](https://keepachangelog.com/sk/1.0.0/)
a projekt používa [sémantické verzovanie](https://semver.org/lang/sk/).

## [1.0.1] – 2026-07-20

Slovenský preklad samotného komponentu, kozmetické úpravy `core.yaml`
a potlačenie kompilačných varovaní.

### Pridané
- `default: break;` do `switch` blokov `mode`, `fan_mode` a `preset`
  v `components/tclac/tclac.cpp`.

### Zmenené
- Kompletný slovenský preklad komponentu `components/tclac` — komentáre v
  `tclac.cpp`, `tclac.h`, `automation.h` a tri validačné hlášky v `climate.py`.
  Autorský podpis prepísaný do latinky, atribúcie zachované.
- `core.yaml`: rozbaľovacie zoznamy (kývanie/fixácia lamiel) a názvy prepínačov
  preložené do slovenčiny; preložené aj všetky komentáre.
- `core.yaml`: názov climate entity zmenený z `"${device_name} Climate"` na
  `"Klimatizácia"` — odstránená duplicita v názve entity v Home Assistant.
- `core.yaml`: `external_components` presmerované na tento fork, `refresh`
  znížený na `1d`.
- `ref` zafixovaný z `master` na tag `v1.0.1` (v `core.yaml` aj v konfigu
  zariadenia) — build sa už neriadi pohyblivým upstreamom.

### Opravené
- Potlačené `-Wswitch` varovania pri kompilácii (*enumeration value not handled
  in switch*) pridaním `default: break;`. Správanie sa nemení — dané režimy
  aj tak nie sú v `supported_modes`, takže sa v UI nedajú vybrať.

### Odstránené
- `packages/screen.yaml` (OLED SSD1306) — pre SLWF-01Pro v2.1 nepoužiteľné;
  odstránené aj zmienky v dokumentácii.

### Poznámky
- Po vydaní vytvor git tag `v1.0.1`; `core.yaml` aj konfig zariadenia naň už
  odkazujú cez `ref`.

## [1.0.0] – 2026-07-20

Prvé vydanie prispôsobené pre **Kaisai PRO HEAT+ (KRW-12TLHI / KRWB-12TLHO)**
na WiFi module **SMLIGHT SLWF-01Pro v2.1**.

### Pridané
- Slovenský
