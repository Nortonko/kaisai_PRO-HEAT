# Komponent `kaisai_ac` — Kaisai PRO HEAT+ (protokol A5/115200)

Externý ESPHome komponent na plné lokálne ovládanie klimatizácií **Kaisai PRO HEAT+**
(KRW-12TLHI / KRWB-12TLHO) a príbuzných TCL jednotiek, ktoré po UART komunikujú
**vlastným protokolom s hlavičkou `0xA5` pri 115200 baud** a kontrolným súčtom
CRC16-XMODEM.

> ⚠️ Toto **nie je** ten istý protokol ako projekt `tclac` (ten je `BB`/9600).
> Ide o novšiu vetvu, ktorá bola zreverzovaná z reálnej prevádzky originálneho
> WiFi modulu **TCWBRCU1** (Tuya WBR3). Ak tvoja jednotka po zapojení posiela
> rámce začínajúce `A5` pri 115200, si na správnom mieste.

## Čo komponent poskytuje

- **climate entita**: režim (OFF / chladenie / kúrenie / sušenie / ventilátor / auto),
  cieľová teplota (16–31 °C), aktuálna teplota, ventilátor (8 stupňov:
  AUTO/MUTE/LOW/LOW-MID/MID/MID-HIGH/HIGH/TURBO), swing (OFF/vert/horiz/both)
- **switch**: ECO, Sleep, Health, Anti-mildew
- **select**: poloha vertikálnej a horizontálnej lamely (kývanie, prúdové režimy,
  odstupňované polohy)

## Hardvér — dôležité

Klimatizácia má **5 V UART**. WiFi modul preto musí vysielať na 5 V úrovni —
priame 3,3 V z holého ESP (napr. D1 mini) jednotka **nepočuje** a povely ignoruje.
Odporúčaný modul je **SMLIGHT SLWF-01Pro v2.1**, ktorý má prevodník úrovní na doske.
Prípadne externý level shifter (BSS138 / 74AHCT125) na smere ESP TX → AC RX.

Zapojenie a kompletný device config sú v `kaisai-ac-example.yaml`.

## Použitie

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/Nortonko/kaisai_PRO-HEAT
      ref: main            # radšej pripni na tag, napr. v2.0.0
    components: [kaisai_ac]

uart:
  id: ac_uart
  rx_pin: GPIO14
  tx_pin: GPIO12
  baud_rate: 115200
  rx_buffer_size: 512

climate:
  - platform: kaisai_ac
    id: kaisai
    name: "Kaisai PRO HEAT+"
    uart_id: ac_uart
    poll_interval: 2s      # 0 = neposielať polly (AC streamuje aj sám)

switch:
  - platform: kaisai_ac
    kaisai_ac_id: kaisai
    function: eco          # eco | sleep | health | anti_mildew
    name: "ECO"

select:
  - platform: kaisai_ac
    kaisai_ac_id: kaisai
    axis: vertical         # vertical | horizontal
    name: "Vertikálna lamela"
```

## Protokol (na dokumentáciu / ďalší vývoj)

Rámec: `A5 01 01 <cmd> <seq> 00 00 <len> <crcHi> <crcLo> <marker> <payload>`
- `cmd`: `0x21` = dáta/povel, `0x23` = poll/riadenie
- `seq`: sekvenčné číslo — pri `0x21` na bajte **[4]**, pri `0x23` na bajte **[5]** (bajt [4]=0)
- `len`: celková dĺžka rámca v bajtoch
- `crc`: CRC16-XMODEM (poly `0x1021`, init `0`, bez reflexie), big-endian,
  počítaný cez celý rámec **okrem** dvoch CRC bajtov (t.j. `[0:8]` + `[10:len]`)
- `marker`: `0C 0C` = stav z AC (čítanie), `0A 0A` = povel do AC (zápis)

Payload je zoznam polí tvaru `00 <ID> <hodnota…>`. Dekódované polia:

| Pole | ID (anchor) | Význam |
|------|-------------|--------|
| power | `00 01 P 00 02` | 0 = vyp, 1 = zap |
| setpoint | `00 02 00 00 HI LO` | teplota × 100 |
| aktuálna tepl. | `00 03 00 00 HI LO` | teplota × 100 |
| ventilátor | `00 05 F 00 0C\|72` | 0=AUTO…7=TURBO |
| vert. swing | `00 0C V 00 0D` | 0/1 |
| horiz. swing | `00 0D H 00 0E` | 0/1 |
| horiz. poloha | `00 0E X 00 11` | kód 1–13 |
| vert. poloha | `00 11 X 00 12` | kód 1–13 |
| režim | `00 12 M 00 DF` | 0=AUTO,1=COOL,2=DRY,3=FAN,4=HEAT |
| ECO | `00 DF E 00 C9` | 0/1 |
| Sleep | `00 22 S 00 25` | 0/1 |
| Health | `00 15 H 00 A4` | 0/1 |
| Anti-mildew | `00 27 M 00 2D` | 0/1 |

Poll (udržuje AC „v obraze"): `A5 01 01 23 00 <seq> 00 0C <crc> 80 0C`.

## Poznámky a stav

- Čítanie stavu je overené a stabilné. Ovládanie funguje na 5 V module.
- ESP8266 pri 115200 môže mať softvérový UART na hrane — SLWF-01Pro (a jeho
  piny GPIO12/14) sa osvedčil.
- Polia Display/Beeper z A5 protokolu zatiaľ nie sú zmapované (v tejto jednotke
  sa nevyskytli), preto ich komponent nevystavuje.

## Poďakovanie

Protokol A5/115200 bol zreverzovaný samostatne pre túto jednotku. Inšpirácia
štruktúrou pochádza z rodiny TCL projektov (`I-am-nightingale/tclac`,
`adaasch/AC-hack`), tie však používajú starší `BB`/9600 protokol a s touto
jednotkou nie sú kompatibilné.
