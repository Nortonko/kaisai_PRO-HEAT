#include "kaisai_ac.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome {
namespace kaisai_ac {

static const char *const TAG = "kaisai_ac";

// Názvy stupňov ventilátora podľa kódu poľa 0x05 (musia sa zhodovať so select.py)
static const char *const FAN_NAMES[8] = {
    "AUTO", "MUTE", "LOW", "LOW-MID", "MID", "MID-HIGH", "HIGH", "TURBO"};

// Mapy kódov polôh lamiel (musia sa presne zhodovať s options v select.py)
struct VaneOpt {
  uint8_t code;
  const char *name;
};
static const VaneOpt VANE_V[] = {{1, "kývanie"}, {2, "prúd nadol"}, {3, "prúd nahor"},
                                 {9, "hore"},    {10, "hore-stred"}, {11, "stred"},
                                 {12, "stred-dole"}, {13, "dole"}};
static const VaneOpt VANE_H[] = {{1, "kývanie"}, {2, "prúd vľavo"}, {3, "prúd stred"},
                                 {4, "prúd vpravo"}, {9, "vľavo"},   {10, "vľavo-stred"},
                                 {11, "stred"},  {12, "stred-vpravo"}, {13, "vpravo"}};

// -------- pomocné extraktory polí zo stavového rámca --------
// pole tvaru: 00 <ID> <val> 00 <after>
static bool ext8(const uint8_t *d, size_t n, uint8_t id, uint8_t after, uint8_t &out) {
  for (size_t i = 0; i + 4 < n; i++)
    if (d[i] == 0x00 && d[i + 1] == id && d[i + 3] == 0x00 && d[i + 4] == after) {
      out = d[i + 2];
      return true;
    }
  return false;
}
// ako ext8, ale akceptuje dva rôzne "after" bajty (napr. ventilátor: 0C aj 72)
static bool ext8b(const uint8_t *d, size_t n, uint8_t id, uint8_t a1, uint8_t a2, uint8_t &out) {
  for (size_t i = 0; i + 4 < n; i++)
    if (d[i] == 0x00 && d[i + 1] == id && d[i + 3] == 0x00 && (d[i + 4] == a1 || d[i + 4] == a2)) {
      out = d[i + 2];
      return true;
    }
  return false;
}
// 16-bit hodnota tvaru: 00 <ID> 00 00 HI LO
static bool ext16(const uint8_t *d, size_t n, uint8_t id, uint16_t &out) {
  for (size_t i = 0; i + 5 < n; i++)
    if (d[i] == 0x00 && d[i + 1] == id && d[i + 2] == 0x00 && d[i + 3] == 0x00) {
      out = ((uint16_t) d[i + 4] << 8) | d[i + 5];
      return true;
    }
  return false;
}

// ================= CRC =================
uint16_t KaisaiAC::crc16_xmodem_(const uint8_t *data, size_t len) {
  uint16_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t) data[i] << 8;
    for (int b = 0; b < 8; b++)
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
  }
  return crc;
}

// ============== odosielanie ==============
// Príkaz: A5 01 01 21 <seq> 00 00 <total> <crcHi crcLo> 0A 0A <payload>
void KaisaiAC::send_command_(const std::vector<uint8_t> &payload) {
  uint8_t seq = this->cmd_seq_++;
  uint8_t total = (uint8_t)(8 + 2 + 2 + payload.size());
  std::vector<uint8_t> cd = {0xA5, 0x01, 0x01, 0x21, seq, 0x00, 0x00, total, 0x0A, 0x0A};
  cd.insert(cd.end(), payload.begin(), payload.end());
  uint16_t crc = crc16_xmodem_(cd.data(), cd.size());
  std::vector<uint8_t> f = {0xA5, 0x01, 0x01, 0x21, seq, 0x00, 0x00, total,
                            (uint8_t)(crc >> 8), (uint8_t)(crc & 0xFF), 0x0A, 0x0A};
  f.insert(f.end(), payload.begin(), payload.end());
  this->tx_queue_.push_back(f);
}

// Poll: A5 01 01 23 00 <seq> 00 0C <crcHi crcLo> 80 0C   (seq je na bajte [5]!)
void KaisaiAC::send_poll_() {
  uint8_t seq = this->poll_seq_++;
  uint8_t cd[10] = {0xA5, 0x01, 0x01, 0x23, 0x00, seq, 0x00, 0x0C, 0x80, 0x0C};
  uint16_t crc = crc16_xmodem_(cd, 10);
  std::vector<uint8_t> f = {0xA5, 0x01, 0x01, 0x23, 0x00, seq,
                            0x00, 0x0C, (uint8_t)(crc >> 8), (uint8_t)(crc & 0xFF), 0x80, 0x0C};
  this->write_array(f.data(), f.size());
}

// ============== príjem a spracovanie ==============
void KaisaiAC::parse_buffer_() {
  while (this->rx_buf_.size() >= 8) {
    if (this->rx_buf_[0] != 0xA5) {
      this->rx_buf_.erase(this->rx_buf_.begin());
      continue;
    }
    uint8_t len = this->rx_buf_[7];
    if (len < 12 || len > 200) {
      this->rx_buf_.erase(this->rx_buf_.begin());
      continue;
    }
    if (this->rx_buf_.size() < len)
      break;

    std::vector<uint8_t> cd(this->rx_buf_.begin(), this->rx_buf_.begin() + 8);
    cd.insert(cd.end(), this->rx_buf_.begin() + 10, this->rx_buf_.begin() + len);
    uint16_t crc = crc16_xmodem_(cd.data(), cd.size());
    uint16_t got = ((uint16_t) this->rx_buf_[8] << 8) | this->rx_buf_[9];

    if (crc == got) {
      if (this->rx_buf_[10] == 0x0C && this->rx_buf_[11] == 0x0C && len > 12)
        this->process_status_(&this->rx_buf_[12], (size_t)(len - 12));
      this->rx_buf_.erase(this->rx_buf_.begin(), this->rx_buf_.begin() + len);
    } else {
      this->rx_buf_.erase(this->rx_buf_.begin());
    }
  }
}

void KaisaiAC::process_status_(const uint8_t *d, size_t n) {
  bool changed = false;
  uint8_t v;
  uint16_t v16;

  bool have_power = ext8(d, n, 0x01, 0x02, v);
  if (have_power && (v != 0) != this->power_on_) {
    this->power_on_ = (v != 0);
    changed = true;
  }
  bool have_mode = ext8(d, n, 0x12, 0xDF, v);
  if (have_mode && v != this->mode_field_) {
    this->mode_field_ = v;
    changed = true;
  }
  if (have_power || have_mode) {
    auto old = this->mode;
    this->recompute_mode_();
    if (this->mode != old)
      changed = true;
  }

  if (ext16(d, n, 0x02, v16)) {
    float t = v16 / 100.0f;
    if (t >= 10 && t <= 40 && this->target_temperature != t) {
      this->target_temperature = t;
      changed = true;
    }
  }
  if (ext16(d, n, 0x03, v16)) {
    float t = v16 / 100.0f;
    if (t >= -20 && t <= 60 && this->current_temperature != t) {
      this->current_temperature = t;
      changed = true;
    }
  }

  // ventilátor -> select
  if (ext8b(d, n, 0x05, 0x0C, 0x72, v) && v < 8 && v != this->fan_code_state_) {
    this->fan_code_state_ = v;
    if (this->sel_[SEL_FAN] != nullptr)
      this->sel_[SEL_FAN]->publish_state(FAN_NAMES[v]);
  }

  // swing -> climate
  bool sw_ch = false;
  if (ext8(d, n, 0x0C, 0x0D, v) && v != this->vswing_) {
    this->vswing_ = v;
    sw_ch = true;
  }
  if (ext8(d, n, 0x0D, 0x0E, v) && v != this->hswing_) {
    this->hswing_ = v;
    sw_ch = true;
  }
  if (sw_ch) {
    this->recompute_swing_();
    changed = true;
  }

  // polohy lamiel -> select
  if (ext8(d, n, 0x11, 0x12, v) && v != this->vane_v_code_) {
    this->vane_v_code_ = v;
    if (this->sel_[SEL_VANE_V] != nullptr)
      for (auto &o : VANE_V)
        if (o.code == v) {
          this->sel_[SEL_VANE_V]->publish_state(o.name);
          break;
        }
  }
  if (ext8(d, n, 0x0E, 0x11, v) && v != this->vane_h_code_) {
    this->vane_h_code_ = v;
    if (this->sel_[SEL_VANE_H] != nullptr)
      for (auto &o : VANE_H)
        if (o.code == v) {
          this->sel_[SEL_VANE_H]->publish_state(o.name);
          break;
        }
  }

  // binárne funkcie -> switch
  if (ext8(d, n, 0xDF, 0xC9, v) && this->sw_[FUNC_ECO] != nullptr)
    this->sw_[FUNC_ECO]->publish_state(v != 0);
  if (ext8(d, n, 0x22, 0x25, v) && this->sw_[FUNC_SLEEP] != nullptr)
    this->sw_[FUNC_SLEEP]->publish_state(v != 0);
  if (ext8(d, n, 0x15, 0xA4, v) && this->sw_[FUNC_HEALTH] != nullptr)
    this->sw_[FUNC_HEALTH]->publish_state(v != 0);
  if (ext8(d, n, 0x27, 0x2D, v) && this->sw_[FUNC_MILDEW] != nullptr)
    this->sw_[FUNC_MILDEW]->publish_state(v != 0);

  this->have_state_ = true;
  if (changed)
    this->publish_state();
}

void KaisaiAC::recompute_mode_() {
  if (!this->power_on_) {
    this->mode = climate::CLIMATE_MODE_OFF;
    return;
  }
  switch (this->mode_field_) {
    case 1: this->mode = climate::CLIMATE_MODE_COOL; break;
    case 2: this->mode = climate::CLIMATE_MODE_DRY; break;
    case 3: this->mode = climate::CLIMATE_MODE_FAN_ONLY; break;
    case 4: this->mode = climate::CLIMATE_MODE_HEAT; break;
    default: this->mode = climate::CLIMATE_MODE_AUTO; break;
  }
}

void KaisaiAC::recompute_swing_() {
  bool vv = this->vswing_ != 0, hh = this->hswing_ != 0;
  if (vv && hh)
    this->swing_mode = climate::CLIMATE_SWING_BOTH;
  else if (vv)
    this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
  else if (hh)
    this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
  else
    this->swing_mode = climate::CLIMATE_SWING_OFF;
}

// ============== climate rozhranie ==============
climate::ClimateTraits KaisaiAC::traits() {
  auto t = climate::ClimateTraits();
  t.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_DRY,
      climate::CLIMATE_MODE_FAN_ONLY,
      climate::CLIMATE_MODE_AUTO,
  });
  t.set_visual_min_temperature(16);
  t.set_visual_max_temperature(31);
  t.set_visual_temperature_step(1);
  t.set_supported_swing_modes({
      climate::CLIMATE_SWING_OFF,
      climate::CLIMATE_SWING_VERTICAL,
      climate::CLIMATE_SWING_HORIZONTAL,
      climate::CLIMATE_SWING_BOTH,
  });
  return t;
}

void KaisaiAC::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    auto m = *call.get_mode();
    if (m == climate::CLIMATE_MODE_OFF) {
      this->send_command_({0x00, 0x01, 0x00});
      this->power_on_ = false;
    } else {
      this->send_command_({0x00, 0x01, 0x01});
      this->power_on_ = true;
      uint8_t mf = 0;
      switch (m) {
        case climate::CLIMATE_MODE_COOL: mf = 1; break;
        case climate::CLIMATE_MODE_DRY: mf = 2; break;
        case climate::CLIMATE_MODE_FAN_ONLY: mf = 3; break;
        case climate::CLIMATE_MODE_HEAT: mf = 4; break;
        default: mf = 0; break;
      }
      this->send_command_({0x00, 0x12, mf});
      this->mode_field_ = mf;
    }
    this->mode = m;
  }

  if (call.get_target_temperature().has_value()) {
    float tf = *call.get_target_temperature();
    uint16_t v = (uint16_t) lroundf(tf * 100.0f);
    this->send_command_({0x00, 0x02, 0x00, 0x00, (uint8_t)(v >> 8), (uint8_t)(v & 0xFF)});
    this->target_temperature = tf;
  }

  if (call.get_swing_mode().has_value()) {
    auto s = *call.get_swing_mode();
    uint8_t vv = (s == climate::CLIMATE_SWING_VERTICAL || s == climate::CLIMATE_SWING_BOTH) ? 1 : 0;
    uint8_t hh = (s == climate::CLIMATE_SWING_HORIZONTAL || s == climate::CLIMATE_SWING_BOTH) ? 1 : 0;
    this->send_command_({0x00, 0x0C, vv});
    this->send_command_({0x00, 0x0D, hh});
    this->vswing_ = vv;
    this->hswing_ = hh;
    this->swing_mode = s;
  }

  this->publish_state();
}

// ============== API pre detské entity ==============
void KaisaiAC::register_switch(KaisaiFunction f, KaisaiSwitch *sw) {
  if (f < 4)
    this->sw_[f] = sw;
}
void KaisaiAC::register_select(uint8_t target, KaisaiSelect *sel) {
  if (target < 3)
    this->sel_[target] = sel;
}

void KaisaiAC::set_function_state(KaisaiFunction f, bool on) {
  uint8_t val = on ? 1 : 0;
  switch (f) {
    case FUNC_ECO: this->send_command_({0x00, 0xDF, val}); break;
    case FUNC_SLEEP: this->send_command_({0x00, 0x22, val}); break;
    case FUNC_HEALTH: this->send_command_({0x00, 0x15, val}); break;
    case FUNC_MILDEW: this->send_command_({0x00, 0x27, val}); break;
  }
}

void KaisaiAC::set_select_by_name(uint8_t target, const std::string &name) {
  if (target == SEL_FAN) {
    for (uint8_t i = 0; i < 8; i++)
      if (name == FAN_NAMES[i]) {
        this->send_command_({0x00, 0x05, i});
        return;
      }
    return;
  }
  const VaneOpt *tab = (target == SEL_VANE_H) ? VANE_H : VANE_V;
  size_t cnt = (target == SEL_VANE_H) ? (sizeof(VANE_H) / sizeof(VaneOpt))
                                      : (sizeof(VANE_V) / sizeof(VaneOpt));
  uint8_t id = (target == SEL_VANE_H) ? 0x0E : 0x11;
  for (size_t i = 0; i < cnt; i++)
    if (name == tab[i].name) {
      this->send_command_({0x00, id, tab[i].code});
      return;
    }
}

// ============== životný cyklus ==============
void KaisaiAC::setup() {
  ESP_LOGCONFIG(TAG, "Spúšťam Kaisai AC (A5/115200)...");
  this->last_poll_ = 0;
}

void KaisaiAC::loop() {
  while (this->available()) {
    uint8_t c;
    if (this->read_byte(&c)) {
      this->rx_buf_.push_back(c);
      if (this->rx_buf_.size() > 512)
        this->rx_buf_.erase(this->rx_buf_.begin());
    }
  }
  this->parse_buffer_();

  uint32_t now = millis();

  if (!this->tx_queue_.empty() && (now - this->last_tx_ >= 60)) {
    auto &f = this->tx_queue_.front();
    this->write_array(f.data(), f.size());
    this->tx_queue_.erase(this->tx_queue_.begin());
    this->last_tx_ = now;
  }

  if (this->poll_interval_ > 0 && (now - this->last_poll_ >= this->poll_interval_)) {
    this->send_poll_();
    this->last_poll_ = now;
  }
}

void KaisaiAC::dump_config() {
  ESP_LOGCONFIG(TAG, "Kaisai PRO HEAT+ (A5/115200):");
  ESP_LOGCONFIG(TAG, "  Poll interval: %u ms", this->poll_interval_);
  this->check_uart_settings(115200);
}

// ============== KaisaiSwitch ==============
void KaisaiSwitch::setup() {
  if (this->parent_ != nullptr)
    this->parent_->register_switch(this->func_, this);
}
void KaisaiSwitch::write_state(bool state) {
  if (this->parent_ != nullptr)
    this->parent_->set_function_state(this->func_, state);
  this->publish_state(state);
}

// ============== KaisaiSelect ==============
void KaisaiSelect::setup() {
  if (this->parent_ != nullptr)
    this->parent_->register_select(this->target_, this);
}
void KaisaiSelect::control(const std::string &value) {
  if (this->parent_ != nullptr)
    this->parent_->set_select_by_name(this->target_, value);
  this->publish_state(value);
}

}  // namespace kaisai_ac
}  // namespace esphome
