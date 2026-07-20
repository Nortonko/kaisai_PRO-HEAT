# Kaisai PRO HEAT+ — WiFi ovládanie cez ESPHome a Home Assistant

Externý komponent pre ESPHome, ktorý umožňuje ovládať klimatizácie/tepelné čerpadlá
**Kaisai PRO HEAT+** (a ďalšie jednotky na protokole TCL) priamo v **Home Assistant**,
lokálne a bez cloudu.

Táto vetva je **prispôsobená konkrétne pre model:**

| Časť | Typové označenie |
|------|------------------|
| Vnútorná jednotka | **KRW-12TLHI** |
| Vonkajšia jednotka | **KRWB-12TLHO** |

> Kaisai je pre tento rad vyrábaný na linkách TCL a používa rovnaký sériový (UART)
> protokol, preto funguje s komponentom `tclac`. Ide o **nástennú split jednotku**
> s káblom/konektorom pre WiFi modul — nie o monoblok vzduch-voda.

Projekt je fork pôvodného diela [I-am-nightingale/tclac](https://github.com/I-am-nightingale/tclac).
Poďakovanie autorovi je na konci tohto súboru.

---

## ⚠️ Dôležité upozornenia pred inštaláciou

1. **Hardvér.** Táto konfigurácia je pripravená pre WiFi modul **SMLIGHT SLWF-01Pro v2.1**
   (čip ESP8266 / ESP-12E). Na tejto doske je UART smerom ku klimatizácii vyvedený na
   **GPIO12 (TX)** a **GPIO14 (RX)**. Ak použiješ inú dosku (ESP-01S, ESP32-C3…),
   piny aj sekciu platformy si musíš upraviť.

2. **Rozostavenie pinov konektora.** SLWF-01Pro bol z výroby navrhnutý pre „Midea"
   rozostavenie pinov. Konektor Kaisai/TCL vyzerá podobne (tvar USB-A), ale **nemusí
   mať rovnaké poradie vodičov.** Pred prvým zasunutím do jednotky over multimetrom,
   ktorý pin nesie GND / +5 V / TX / RX. Pri nezhode hrozí poškodenie modulu alebo
   riadiacej dosky. V prípade pochybností prepoj vodiče napriamo na piny, nespoliehaj sa
   na to, že „keď to zapadne, musí to fungovať".

3. **Verzia ESPHome.** Komponent vyžaduje **Home Assistant a ESPHome minimálne 2026.4.0**.
   Testované na 2026.6.x / 2026.7.0.

4. **Dĺžka správ.** Komponent podporuje správy z jednotky s dĺžkou 61, 65 a 68 bajtov;
   plne overená je zatiaľ len 61-bajtová varianta. Ak jednotka nekomunikuje spoľahlivo,
   pozri voliteľné balíčky nižšie (`bad_connect.yaml`, `uart_speed.yaml`).

---

## 🛠️ Čo budeš potrebovať

- Klimatizáciu **Kaisai PRO HEAT+ (KRW-12TLHI / KRWB-12TLHO)** s portom pre WiFi modul
- WiFi modul **SMLIGHT SLWF-01Pro v2.1** (ESP8266/ESP-12E)
- **Home Assistant** s doplnkom **ESPHome Device Builder** (verzia ≥ 2026.4.0)

---

## 🧠 Inštalácia v Home Assistant

### 1. Nainštaluj ESPHome
V Home Assistant: **Nastavenia → Doplnky → Obchod s doplnkami → ESPHome Device Builder**.

### 2. Vytvor nové zariadenie
V ESPHome dashboarde: **New Device**. Pri prvom flashnutí pripoj modul cez USB/UART;
ďalšie aktualizácie už pôjdu bezdrôtovo (OTA).

### 3. Vlož konfiguráciu
Zvoľ jednu z dvoch:

- **Jednoduchá** → [`Sample_conf.yaml`](Sample_conf.yaml)
- **Podrobná (s komentármi ku každému poľu)** → [`TCL-Conditioner.yaml`](TCL-Conditioner.yaml)

Skopíruj obsah do svojho zariadenia v ESPHome a **uprav polia** (WiFi, názvy, kľúče).
Nápoveda ku každému poľu je priamo v komentároch YAML.

### 4. Flashni modul
Prvýkrát cez USB/UART, potom už OTA.

---

## 🔌 Platforma a zapojenie (SLWF-01Pro v2.1)

Sekcia platformy v YAML pre tento modul:

```yaml
esp8266:
  board: esp12e
```

UART piny (v sekcii `substitutions:`):

```yaml
uart_tx: GPIO12
uart_rx: GPIO14
```

Predvolené hodnoty `GPIO3 / GPIO1` z pôvodného projektu sú pre **ESP-01S** — pre
SLWF-01Pro ich **nepoužívaj**.

---

## 📦 Podgružované (remote) balíčky

Konfigurácia sa načítava modulárne z GitHubu cez sekciu `packages:`. Povinné je jadro,
ostatné moduly sú voliteľné:

```yaml
packages:
  remote_package:
    url: https://github.com/Nortonko/kaisai_PRO-HEAT.git
    ref: master
    files:
    # v - riadky s balíčkami zarovnaj presne pod túto značku, inak ESPHome hlási chyby
      - packages/core.yaml          # POVINNÉ jadro (wifi/api/ota/uart/climate…)
      # - packages/leds.yaml        # LED indikácia príjmu/vysielania (piny receive_led / transmit_led)
      # - packages/bad_connect.yaml # 3-násobný pokus o odoslanie príkazu pri slabom spojení
      # - packages/uart_speed.yaml  # prepínač rýchlosti UART v nastaveniach zariadenia
    refresh: 30s
```

> **Zarovnanie je povinné.** Všetky riadky `- packages/…` musia začínať na tej istej
> pozícii ako značka `# v`. Zlé odsadenie = záplava nejasných chýb z ESPHome.

`url:` je nastavené na **tento fork**, takže repozitár je samostatný. Ak by si chcel
ťažiť z opráv v pôvodnom projekte, môžeš `url:` dočasne prepnúť na
`https://github.com/I-am-nightingale/tclac.git`.

### Ručné pridelenie IP adresy (voliteľné)
Predvolene sa IP získa z DHCP. Ak chceš statickú, pridaj na koniec konfigurácie:

```yaml
wifi:
  manual_ip:
    static_ip: 192.168.1.4
    gateway: 192.168.1.1
    subnet: 255.255.255.0
```

---

## ✅ Overené jednotky (z pôvodného projektu)

Okrem Kaisai PRO HEAT+ boli komunitne overené aj ďalšie jednotky na rovnakom protokole
(s pájkovaním aj bez neho). Presne predpovedať kompatibilitu nie je možné — aj tá istá
modelová značka sa líši výbavou (chýbajúci WiFi modul, chýbajúci USB kábel, nezapájkovaný
UART konektor na doske):

- Axioma ASX09H1/ASB09H1
- Ballu BSAI-12HN1_15Y; Ballu Discovery DC BSVI-07/09/12HN8
- Daichi AIR20AVQ1/AIR20FV1, AIR25AVQS1R-1, AIR35AVQS1R-1, DA35EVQ1-1
- Dantex RK-12SATI/RK-12SATIE
- Ecostar Radium KVS-RAD09CH
- iFFALCON F1 18
- Royal Clima Gloria Inverter; Royal Clima Pandora RC-PDC28HN
- Tesla TT27TP61S-0932IAWUV
- TCL: ELI ONF 12, Liferise ONF 09, TAC-CT09INV/R, TAC-07/09/12CHSA (rôzne varianty),
  TAC-09HRID/E1, TAC-XAL24I, TPG31IHB a ďalšie

---

## 🙏 Poďakovanie

- Pôvodný komponent a všetka zásluha za protokol: **[I-am-nightingale/tclac](https://github.com/I-am-nightingale/tclac)**
- Článok k projektu (v ruštine): <https://dzen.ru/a/ZmdoyUNswXWnulhg>
- Poďakovať autorovi: [jeho Steam profil](https://steamcommunity.com/id/solovey-iron/)
- Alternatíva cez MQTT (iný projekt): <https://github.com/pavel211/TCL-TAC-07-WiFi>

Táto vetva len prekladá dokumentáciu do slovenčiny a prispôsobuje konfiguráciu modelu
Kaisai PRO HEAT+ (KRW-12TLHI / KRWB-12TLHO) na module SLWF-01Pro v2.1. Samotný kód
komponentu (`components/tclac`) je dielom pôvodného autora.
