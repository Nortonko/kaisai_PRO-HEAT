#pragma once
// ============================================================
// Externý ESPHome komponent pre klimatizácie Kaisai PRO HEAT+
// (KRW-12TLHI / KRWB-12TLHO) a príbuzné TCL jednotky, ktoré
// komunikujú protokolom s hlavičkou 0xA5 pri 115200 baud.
//
// POZOR: toto NIE JE ten istý protokol ako projekt tclac
// (ten je BB/9600). Táto jednotka používa novší A5/115200
// protokol s CRC16-XMODEM, zreverzovaný z reálnej prevádzky.
//
// Hardvér: WiFi modul s 5V UART prevodníkom úrovní
// (napr. SMLIGHT SLWF-01Pro v2.1). Priame 3.3V ESP TX
// klimatizácia NEPRIJME — treba 5V úroveň.
// ============================================================

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/select/select.h"
#include <vector>
#include <string>

namespace esphome {
namespace kaisai_ac {

// Binárne funkcie klimatizácie (prepínače)
enum KaisaiFunction : uint8_t {
  FUNC_ECO = 0,
  FUNC_SLEEP = 1,
  FUNC_HEALTH = 2,
  FUNC_MILDEW = 3,
};

// Ciele pre select entity
enum KaisaiSelectTarget : uint8_t {
  SEL_VANE_V = 0,  // vertikálna lamela (pole 0x11)
  SEL_VANE_H = 1,  // horizontálna lamela (pole 0x0E)
  SEL_FAN = 2,     // rýchlosť ventilátora (pole 0x05)
};

class KaisaiAC;

// ---- Prepínač (ECO / Sleep / Health / Anti-mildew) ----
class KaisaiSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(KaisaiAC *parent) { this->parent_ = parent; }
  void set_function(KaisaiFunction f) { this->func_ = f; }
  void setup() override;

 protected:
  void write_state(bool state) override;
  KaisaiAC *parent_{nullptr};
  KaisaiFunction func_{FUNC_ECO};
};

// ---- Select (poloha lamely alebo rýchlosť ventilátora) ----
class KaisaiSelect : public select::Select, public Component {
 public:
  void set_parent(KaisaiAC *parent) { this->parent_ = parent; }
  void set_target(uint8_t t) { this->target_ = t; }
  void setup() override;

 protected:
  void control(const std::string &value) override;
  KaisaiAC *parent_{nullptr};
  uint8_t target_{SEL_VANE_V};
};

// ---- Hlavný komponent: climate entita + UART ----
class KaisaiAC : public climate::Climate, public uart::UARTDevice, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  void set_poll_interval(uint32_t ms) { this->poll_interval_ = ms; }

  // registrácia detských entít
  void register_switch(KaisaiFunction f, KaisaiSwitch *sw);
  void register_select(uint8_t target, KaisaiSelect *sel);

  // API volané z detí
  void set_function_state(KaisaiFunction f, bool on);
  void set_select_by_name(uint8_t target, const std::string &name);

 protected:
  // --- rámce ---
  static uint16_t crc16_xmodem_(const uint8_t *data, size_t len);
  void send_command_(const std::vector<uint8_t> &payload);  // marker 0A 0A
  void send_poll_();                                         // 23 ... 80 0C
  void parse_buffer_();
  void process_status_(const uint8_t *d, size_t n);          // payload za 0C 0C

  void recompute_mode_();
  void recompute_swing_();

  // --- vyrovnávacie pamäte / časovanie ---
  std::vector<uint8_t> rx_buf_;
  std::vector<std::vector<uint8_t>> tx_queue_;
  uint8_t cmd_seq_{0};
  uint8_t poll_seq_{0};
  uint32_t poll_interval_{2000};
  uint32_t last_tx_{0};
  uint32_t last_poll_{0};

  // --- posledný známy stav ---
  bool have_state_{false};
  bool power_on_{false};
  uint8_t mode_field_{0};
  uint8_t fan_code_state_{0xFF};
  uint8_t vswing_{0}, hswing_{0};
  uint8_t vane_v_code_{0xFF}, vane_h_code_{0xFF};

  KaisaiSwitch *sw_[4] = {nullptr, nullptr, nullptr, nullptr};
  KaisaiSelect *sel_[3] = {nullptr, nullptr, nullptr};
};

}  // namespace kaisai_ac
}  // namespace esphome
