# Kaisai PRO HEAT+ — ESPHome komponent (protokol A5/115200)

Plne lokálne ovládanie klimatizácií **Kaisai PRO HEAT+** v **Home Assistant**
cez **ESPHome**, bez cloudu a bez originálneho Tuya modulu.

Projekt obsahuje externý ESPHome komponent `kaisai_ac`, ktorý komunikuje
so vzduchotechnickou jednotkou po UART **vlastným protokolom s hlavičkou `0xA5`
pri 115200 baud** (CRC16-XMODEM). Protokol bol zreverzovaný z reálnej prevádzky.

| Časť | Typové označenie |
|------|------------------|
| Vnútorná jednotka | **KRW-12TLHI** |
| Vonkajšia jednotka | **KRWB-12TLHO** |

> ⚠️ **Toto nie je variant projektu `tclac`.** Staršie TCL jednotky používajú
> protokol `BB` pri 9600 baud (projekty `tclac`, `AC-hack`). Táto jednotka
> (novšia generácia s modulom TCWBRCU1 / Tuya WBR3) hovorí protokolom `A5`
> pri 115200 a s tými projektmi **nie je kompatibilná**. Ak tvoja jednotka po
> zapojení posiela rámce začínajúce `A5` pri 115200, si na správnom mieste.

---

## Čo to vie

- **Klimatizácia (climate)**: zapnutie/vypnutie, režim (chladenie, kúrenie,
  sušenie, ventilátor, auto), cieľová teplota 16–31 °C, aktuálna teplota,
  swing (vertikálny / horizontálny / oba / vypnutý)
- **Ventilátor (select)**: 8 stupňov — AUTO, MUTE, LOW, LOW-MID, MID, MID-HIGH,
  HIGH, TURBO
- **Lamely (select)**: vertikálna aj horizontálna — kývanie, prúdové režimy,
  odstupňované polohy
- **Funkcie (switch)**: ECO, Sleep, Health, Anti-mildew, displej, pípanie, jemný vietor
- **Senzory**: otáčky ventilátora vonk. jednotky (RPM), frekvencia kompresora (Hz) a teploty výmenníkov (°C)

Čítanie stavu aj ovládanie sú overené na reálnej jednotke.

---

## ⚠️ Hardvér — pozor na napäťovú úroveň

Riadiaca doska klimatizácie má **5 V UART**. WiFi modul preto musí vysielať na
5 V úrovni — priame **3,3 V z holého ESP (napr. D1 mini) jednotka NEPRIJME** a
povely ignoruje (čítanie funguje aj tak, ale ovládanie nie).

Odporúčaný modul: **SMLIGHT SLWF-01Pro v2.1** — má prevodník úrovní na doske.
Alternatívne externý level shifter (BSS138 / 74AHCT125) na smere ESP TX → AC RX.

### Zapojenie (konektor CN16)

| CN16 | Modul |
|------|-------|
| +5V | +5V |
| GND | GND |
| TX (stavový vodič z AC) | RX modulu (GPIO14) |
| RX (povely do AC) | TX modulu (GPIO12) |

> Nezasúvaj SLWF-01Pro priamo do konektora AC — jeho konektor je zapojený pre
> „Midea" rozostavenie pinov, ktoré sa s Kaisai/TCL nemusí zhodovať. Prepoj
> vodiče ručne na CN16.

---

## Inštalácia

1. V Home Assistant maj **ESPHome Device Builder** (ESPHome ≥ 2026.4).
2. Skopíruj obsah `kaisai-ac-example.yaml` do nového zariadenia a uprav secrets.
3. Komponent sa načíta priamo z tohto repozitára cez `external_components`.
4. Prvé flashnutie cez USB, ďalej OTA.

### Minimálny config

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/Nortonko/kaisai_PRO-HEAT
      ref: v1.0.0            # pripni na tag pre stabilitu
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
    poll_interval: 2s        # 0 = neposielať polly
    republish_interval: 30s  # ako často znovu poslať celý stav do HA (0 = vypnúť)

switch:
  - platform: kaisai_ac
    kaisai_ac_id: kaisai
    function: eco            # eco | sleep | health | anti_mildew
    name: "ECO"

select:
  - platform: kaisai_ac
    kaisai_ac_id: kaisai
    target: fan              # fan | vane_vertical | vane_horizontal
    name: "Ventilátor"

sensor:
  - platform: kaisai_ac
    kaisai_ac_id: kaisai
    type: compressor_freq    # fan_rpm | compressor_freq | coil_temp_inner | coil_temp_outer
    name: "Frekvencia kompresora"
    unit_of_measurement: "Hz"
    device_class: frequency
```

Kompletný príklad so všetkými entitami je v [`kaisai-ac-example.yaml`](kaisai-ac-example.yaml).

### Čerstvosť dát v Home Assistant

Komponent publikuje zmeny okamžite, ale hodnoty, ktoré sa dlho nemenia
(setpoint, režim, teploty pri ustálenom behu), by inak v HA „zostarli" —
klimatizácia totiž posiela stav sporadicky, v zhlukoch. Preto komponent raz za
`republish_interval` (predvolene 30 s) znovu odošle **celý posledný známy stav**
(climate, senzory, switche, selecty). Vďaka tomu entity nikdy nezostarnú ani
nespadnú na „nedostupné" a **nie sú potrebné žiadne `heartbeat` ani `timeout`
filtre v YAML-e**. Interval sa dá zmeniť alebo vypnúť (`republish_interval: 0s`).

---

## Protokol (pre zvedavých a ďalší vývoj)

Rámec: `A5 01 01 <cmd> <seq> 00 00 <len> <crcHi> <crcLo> <marker> <payload>`

- `cmd`: `0x21` = dáta/povel, `0x23` = poll/riadenie
- `seq`: sekvenčné číslo — pri `0x21` na bajte **[4]**, pri `0x23` na bajte **[5]**
- `len`: celková dĺžka rámca
- `crc`: CRC16-XMODEM (poly `0x1021`, init `0`, bez reflexie), big-endian,
  cez celý rámec **okrem** dvoch CRC bajtov (`[0:8]` + `[10:len]`)
- `marker`: `0C 0C` = stav z AC (čítanie), `0A 0A` = povel do AC (zápis)

Po každom povele (`cmd 0x21`, marker `0A 0A`) AC do ~100–230 ms odpovie
potvrdzovacím rámcom (`cmd 0x23`, marker `80 0A`) a vzápätí bežným stavovým
rámcom. Komponent na ACK nečaká ani ho nevyhodnocuje — skutočný stav si prečíta
z nasledujúceho stavového rámca, takže prípadný stratený povel sa opraví pri
ďalšom čítaní. (Zistené duálnym odpočúvaním oboch smerov naraz.)

Payload je zoznam polí tvaru `00 <ID> <hodnota…>`:

| Pole | Anchor | Význam |
|------|--------|--------|
| power | `00 01 P 00 02` | 0 = vyp, 1 = zap |
| setpoint | `00 02 00 00 HI LO` | teplota × 100 |
| aktuálna teplota | `00 03 00 00 HI LO` | teplota × 100 |
| ventilátor | `00 05 F 00 0C\|72` | 0=AUTO … 7=TURBO |
| vert. swing | `00 0C V 00 0D` | 0/1 |
| horiz. swing | `00 0D H 00 0E` | 0/1 |
| horiz. poloha | `00 0E X 00 11` | kód 1–13 |
| vert. poloha | `00 11 X 00 12` | kód 1–13 |
| režim | `00 12 M 00 DF` | 0=AUTO,1=COOL,2=DRY,3=FAN,4=HEAT |
| ECO | `00 DF E 00 C9` | 0/1 |
| Sleep | `00 22 S 00 25` | 0/1 |
| Health | `00 15 H 00 A4` | 0/1 |
| Anti-mildew | `00 27 M 00 2D` | 0/1 |
| displej (podsvietenie) | `00 1E D` | 0/1 |
| pípanie (beep) | `00 25 B 00 27` | 0/1 |
| jemný vietor (soft wind) | `00 26 S` | 0/1 |
| otáčky ventilátora vonk. jednotky | `00 64` + 4B, potom `00 65` | RPM (0 = ventilátor stojí) |
| frekvencia kompresora | `00 65` + 4B, potom `00 13` | Hz (0 = kompresor stojí) |
| tepl. vnút. výmenníka | `00 5C 00 00 HI LO` | °C ×100 |
| tepl. vonk. výmenníka | `00 60 00 00 HI LO` | °C ×100 |

Poll rámec (udržuje AC v obraze): `A5 01 01 23 00 <seq> 00 0C <crc> 80 0C`.

**Poznámka k diagnostike vonkajšej jednotky:** polia `0x64` a `0x65` hlásia **otáčky ventilátora vonkajšej jednotky (RPM)** a **frekvenciu kompresora (Hz)** — nie elektrický príkon. **Reálny príkon (W) protokol nehlási vôbec**; na meranie spotreby treba externý merač. (Skoršie verzie tieto polia mylne označovali ako zdanlivý výkon a prúd; interpretácia bola opravená na základe merania — pri 233 V a 250–300 W by hodnoty ako VA/prúd dávali nereálny účinník ~0,36.)

Podrobnejšia dokumentácia komponentu je v [`components/kaisai_ac/README.md`](components/kaisai_ac/README.md).

---

## Kompatibilita

- ESPHome / Home Assistant **2026.4.0** alebo novšie (overené na 2026.7.3).
- Určené pre Kaisai PRO HEAT+ (KRW-12TLHI / KRWB-12TLHO). Iné jednotky s rovnakým
  `A5`/115200 protokolom môžu fungovať tiež, ale nie sú overené.

## Prispievanie

Ak máš inú jednotku na tomto protokole a nájdeš ďalšie polia (napr. Display,
Beeper, ďalšie diagnostické polia), pull requesty a záchyty rámcov sú vítané.

## Licencia

[MIT](LICENSE). Protokol bol zreverzovaný samostatne; samotný kód je nový.
