/**
* Create by Miguel Ángel López on 20/07/19
* and modify by xaxexa
* Refactoring & component making:
* Solovej so spájkovačkou 15.03.2024
**/
#include "esphome.h"
#include "esphome/core/defines.h"
#include "tclac.h"

namespace esphome{
namespace tclac{


ClimateTraits tclacClimate::traits() {
	auto traits = climate::ClimateTraits();
	traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
	
	// Zodpovedne vyhlasujem, že toto všetko som prevzal od christoph5180
	if (this->supported_modes_.empty()) {
		traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
		traits.add_supported_mode(climate::CLIMATE_MODE_HEAT_COOL);
	} else {
		for (auto mode : this->supported_modes_)
			traits.add_supported_mode(mode);
	}
	if (this->supported_presets_.empty()) {
		traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_NONE);
	} else {
		for (auto preset : this->supported_presets_)
			traits.add_supported_preset(preset);
	}
	if (this->supported_fan_modes_.empty()) {
		traits.add_supported_fan_mode(climate::CLIMATE_FAN_AUTO);
	} else {
		for (auto fan_mode : this->supported_fan_modes_)
			traits.add_supported_fan_mode(fan_mode);
	}
	if (this->supported_swing_modes_.empty()) {
		traits.add_supported_swing_mode(climate::CLIMATE_SWING_OFF);
	} else {
		for (auto swing_mode : this->supported_swing_modes_)
			traits.add_supported_swing_mode(swing_mode);
	}

	return traits;
}


void tclacClimate::setup() {

#ifdef CONF_RX_LED
	this->rx_led_pin_->setup();
	this->rx_led_pin_->digital_write(false);
#endif
#ifdef CONF_TX_LED
	this->tx_led_pin_->setup();
	this->tx_led_pin_->digital_write(false);
#endif
}

void tclacClimate::loop()  {
	// Ak je niečo v UART buffri, prečítame to
	if (esphome::uart::UARTDevice::available() > 0) {
		// linka je zaneprázdnená príjmom — odoslanie príkazových rámcov pozdržíme
		this->last_rx_ms_ = millis();
		dataShow(0, true);
		dataRX[0] = esphome::uart::UARTDevice::read();
		// Ak prijatý bajt nie je hlavička (0xBB), jednoducho opustíme cyklus
		if (dataRX[0] != 0xBB) {
			ESP_LOGD("TCL", "Wrong byte");
			dataShow(0,0);
			return;
		}
		// Ak sa zhoduje hlavička (0xBB), začneme reťazovo čítať ďalšie 4 bajty
		// Občas treba pri niektorých klimatizáciách pridať delay(5) medzi pakety. Prečo — ktovie, ale treba. No nie vždy. Hoci občas áno. Ale nie zakaždým. Zriedka. Stáva sa.
		// delay(5);
		dataRX[1] = esphome::uart::UARTDevice::read();
		// delay(5);
		dataRX[2] = esphome::uart::UARTDevice::read();
		// delay(5);
		dataRX[3] = esphome::uart::UARTDevice::read();
		// delay(5);
		dataRX[4] = esphome::uart::UARTDevice::read();

		//auto raw = getHex(dataRX, 5);
		//ESP_LOGD("TCL", "first 5 byte : %s ", raw.c_str());

		// OCHRANA PROTI PRETEČENIU: piaty bajt je dĺžka užitočnej časti, ďalej
		// sa číta dataRX[4]+1 bajtov do dataRX+5. Legitímne sú len tri dĺžky
		// rámca (0x37=61, 0x3b=65, 0x3e=68 bajtov spolu). Na zašumenej linke
		// read() môže vrátiť -1 (0xFF) alebo príde odpad — potom by dataRX[4]+6
		// presiahlo hranicu dataRX[68]. Takýto rámec zahodíme.
		if (dataRX[4] != 0x37 && dataRX[4] != 0x3b && dataRX[4] != 0x3e) {
			ESP_LOGW("TCL", "Bad frame length 0x%02X, dropped", dataRX[4]);
			while (esphome::uart::UARTDevice::available() > 0) esphome::uart::UARTDevice::read();
			dataShow(0,0);
			return;
		}

		// Z prvých 5 bajtov potrebujeme piaty — obsahuje dĺžku správy.
		// read_array vráti false pri timeoute (rámec sa prerušil v polovici) —
		// vtedy je v buffri odpad z predošlého rámca, nemožno ho spracovať.
		if (!esphome::uart::UARTDevice::read_array(dataRX+5, dataRX[4]+1)) {
			ESP_LOGW("TCL", "Frame read timeout, dropped");
			dataShow(0,0);
			return;
		}

		// Získavame kontrolný súčet:
		if (dataRX[4] == 0x3e){
			// Pre dátový paket dĺžky 68 bajtov
			check = getChecksum(dataRX, 68);
		} else if (dataRX[4] == 0x37){
			// Pre dátový paket dĺžky 61 bajtov
			check = getChecksum(dataRX, 61);
		} else {
			// Pre dátový paket dĺžky 65 bajtov
			check = getChecksum(dataRX, 65);
		}

		//raw = getHex(dataRX, sizeof(dataRX));
		//ESP_LOGD("TCL", "RX full : %s ", raw.c_str());
		
		// Overujeme kontrolný súčet:
		if (dataRX[4] == 0x3e){
			// Pre dátový paket dĺžky 68 bajtov
			if (check != dataRX[67]) {
				ESP_LOGD("TCL", "Invalid checksum %x", check);
				this->dataShow(0,0);
				return;
			} else {
				//ESP_LOGD("TCL", "checksum OK %x", check);
			}
		} else if (dataRX[4] == 0x37){
			if (check != dataRX[60]) {
				// Pre dátový paket dĺžky 61 bajtov
				ESP_LOGD("TCL", "Invalid checksum %x", check);
				this->dataShow(0,0);
				return;
			} else {
				//ESP_LOGD("TCL", "checksum OK %x", check);
			}
		} else {
			if (check != dataRX[64]) {
				// Pre dátový paket dĺžky 65 bajtov
				ESP_LOGD("TCL", "Invalid checksum %x", check);
				this->dataShow(0,0);
				return;
			} else {
				//ESP_LOGD("TCL", "checksum OK %x", check);
			}
		}
		this->dataShow(0,0);
		// Po prečítaní celého buffra pristúpime k spracovaniu dát
		this->readData();
	}
}

void tclacClimate::update() {
	tclacClimate::dataShow(1,1);
	this->esphome::uart::UARTDevice::write_array(poll, sizeof(poll));
	// po dopyte začne klimatizácia odpovedať — príkazové rámce počkajú
	this->poll_sent_ms_ = millis();
	//auto raw = tclacClimate::getHex(poll, sizeof(poll));
	ESP_LOGD("TCL", "chek status sended");
	tclacClimate::dataShow(1,0);
}

void tclacClimate::readData() {
	
	// Túto konštrukciu navrhla neurónka Claude, takýmto vychytávkam vôbec nerozumiem, tak ju vkladám tak, ako je.
	current_temperature = ((float)((dataRX[17] << 8) | dataRX[18]) / 374.0f - 32.0f) / 1.8f;
	
	target_temperature = (dataRX[FAN_SPEED_POS] & SET_TEMP_MASK) + 16;

	//ESP_LOGD("TCL", "TEMP: %f ", current_temperature);

	if (dataRX[MODE_POS] & ( 1 << 4)) {
		// Ak je klimatizácia zapnutá, spracujeme dáta na zobrazenie
		ESP_LOGD("TCL", "AC is on");

		// Synchronizujeme stav displeja so skutočným (bit DISPLAY_BIT v
		// bajte režimu): diaľkovým ovládačom sa displej prepína mimo modulu, a bez
		// synchronizácie by nasledujúci príkaz modulu prepísal stav
		// displeja starou hodnotou. Len keď je klimatizácia zapnutá.
		this->display_status_ = (dataRX[MODE_POS] & DISPLAY_BIT) != 0;

		uint8_t modeswitch = MODE_MASK & dataRX[MODE_POS];
		uint8_t fanspeedswitch = FAN_SPEED_MASK & dataRX[FAN_SPEED_POS];
		uint8_t swingmodeswitch = SWING_MODE_MASK & dataRX[SWING_POS];

		switch (modeswitch) {
			case MODE_AUTO:
				this->mode = climate::CLIMATE_MODE_HEAT_COOL;
				break;
			case MODE_COOL:
				this->mode = climate::CLIMATE_MODE_COOL;
				break;
			case MODE_DRY:
				this->mode = climate::CLIMATE_MODE_DRY;
				break;
			case MODE_FAN_ONLY:
				this->mode = climate::CLIMATE_MODE_FAN_ONLY;
				break;
			case MODE_HEAT:
				this->mode = climate::CLIMATE_MODE_HEAT;
				break;
			default:
				this->mode = climate::CLIMATE_MODE_HEAT_COOL;
		}

		if ( dataRX[FAN_QUIET_POS] & FAN_QUIET) {
			fan_mode = climate::CLIMATE_FAN_QUIET;
		} else if (dataRX[MODE_POS] & FAN_DIFFUSE){
			fan_mode = climate::CLIMATE_FAN_DIFFUSE;
		} else {
			switch (fanspeedswitch) {
				case FAN_AUTO:
					fan_mode = climate::CLIMATE_FAN_AUTO;
					break;
				case FAN_LOW:
					fan_mode = climate::CLIMATE_FAN_LOW;
					break;
				case FAN_MIDDLE:
					fan_mode = climate::CLIMATE_FAN_MIDDLE;
					break;
				case FAN_MEDIUM:
					fan_mode = climate::CLIMATE_FAN_MEDIUM;
					break;
				case FAN_HIGH:
					fan_mode = climate::CLIMATE_FAN_HIGH;
					break;
				case FAN_FOCUS:
					fan_mode = climate::CLIMATE_FAN_FOCUS;
					break;
				default:
					fan_mode = climate::CLIMATE_FAN_AUTO;
			}
		}

		switch (swingmodeswitch) {
			case SWING_OFF: 
				swing_mode = climate::CLIMATE_SWING_OFF;
				break;
			case SWING_HORIZONTAL:
				swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
				break;
			case SWING_VERTICAL:
				swing_mode = climate::CLIMATE_SWING_VERTICAL;
				break;
			case SWING_BOTH:
				swing_mode = climate::CLIMATE_SWING_BOTH;
				break;
		}
		
		// Spracovanie údajov o presete
		preset = ClimatePreset::CLIMATE_PRESET_NONE;
		if (dataRX[7] & (1 << 6)){
			preset = ClimatePreset::CLIMATE_PRESET_ECO;
		} else if (dataRX[9] & (1 << 2)){
			preset = ClimatePreset::CLIMATE_PRESET_COMFORT;
		} else if (dataRX[19] & (1 << 0)){
			preset = ClimatePreset::CLIMATE_PRESET_SLEEP;
		}
		
	} else {
		ESP_LOGD("TCL", "AC is OFF");
		// Ak je klimatizácia vypnutá, všetky režimy sa zobrazujú ako vypnuté
		this->mode = climate::CLIMATE_MODE_OFF;
		//fan_mode = climate::CLIMATE_FAN_OFF;
		this->swing_mode = climate::CLIMATE_SWING_OFF;
		this->preset = ClimatePreset::CLIMATE_PRESET_NONE;
	}
	// Publikujeme dáta
	this->publish_state();
	allow_take_control = true;
   }

// Climate control
void tclacClimate::control(const climate::ClimateCall &call) {
	
	ESP_LOGD("TCL", "Call from UI");

	// OCHRANA PROTI ECHO-SLUČKE: príkaz, ktorý nemení aktuálny stav, sa ignoruje.
	// Integrácie (napr. klient Yandex Alice alebo MQTT most) môžu odrážať
	// stav späť do príkazu; bez tejto kontroly by echo-rámec opakovane volal
	// control() -> publikáciu stavu -> znova echo, a vznikla by búrka.
	bool changed = false;
	if (call.get_mode().has_value() && *call.get_mode() != this->mode) changed = true;
	if (call.get_target_temperature().has_value()
		&& (int) *call.get_target_temperature() != (int) this->target_temperature) changed = true;
	if (call.get_fan_mode().has_value()
		&& (!this->fan_mode.has_value() || *call.get_fan_mode() != this->fan_mode.value())) changed = true;
	if (call.get_swing_mode().has_value() && *call.get_swing_mode() != this->swing_mode) changed = true;
	if (call.get_preset().has_value()
		&& (!this->preset.has_value() || *call.get_preset() != this->preset.value())) changed = true;
	if (!changed) {
		ESP_LOGD("TCL", "Call from UI: no changes, skipped");
		return;
	}

	// Toto a nižšie som odkukal od Vi3jo.

	if (call.get_mode().has_value()) this->mode = *call.get_mode();
    if (call.get_target_temperature().has_value()) this->target_temperature = *call.get_target_temperature();
    if (call.get_fan_mode().has_value()) this->fan_mode = *call.get_fan_mode();
	if (call.get_swing_mode().has_value()) this->swing_mode = *call.get_swing_mode();
	if (call.get_preset().has_value()) this->preset = *call.get_preset();
	
	this->publish_state();
	this->takeControl();
	this->allow_take_control = true;
}
	
	
void tclacClimate::takeControl() {
	
	dataTX[7]  = 0b00000000;
	dataTX[8]  = 0b00000000;
	dataTX[9]  = 0b00000000;
	dataTX[10] = 0b00000000;
	dataTX[11] = 0b00000000;
	dataTX[19] = 0b00000000;
	dataTX[32] = 0b00000000;
	dataTX[33] = 0b00000000;
	
	// Ochrana proti odpadu v bajte žiadanej hodnoty: do prvého stavového rámca
	// je target_temperature = NaN, a (int)NaN je nedefinované správanie.
	if (isnan(target_temperature) || target_temperature < 16 || target_temperature > 31) {
		target_temperature = 24;
	}
	uint8_t target_temperature_set = 31-(int)target_temperature;
	
	// Zapíname alebo vypíname pípanie podľa prepínača v nastaveniach
	if (beeper_status_){
		ESP_LOGD("TCL", "Beep mode ON");
		dataTX[7] += 0b00100000;
	} else {
		ESP_LOGD("TCL", "Beep mode OFF");
		dataTX[7] += 0b00000000;
	}
	
	// Zapíname alebo vypíname displej na klimatizácii podľa prepínača v nastaveniach
	// Displej zapíname iba ak je klimatizácia v niektorom z pracovných režimov
	
	// POZOR! Pri vypnutí displeja klimatizácia sama vynútene prejde do automatického režimu!
	
	if ((display_status_) && (mode != climate::CLIMATE_MODE_OFF)){
		ESP_LOGD("TCL", "Dispaly turn ON");
		dataTX[7] += 0b01000000;
	} else {
		ESP_LOGD("TCL", "Dispaly turn OFF");
		dataTX[7] += 0b00000000;
	}
		
	// Nastavujeme prevádzkový režim klimatizácie
	switch (this->mode) {
		case climate::CLIMATE_MODE_OFF:
			dataTX[7] += 0b00000000;
			dataTX[8] += 0b00000000;
			break;
		case climate::CLIMATE_MODE_HEAT_COOL:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00001000;
			break;
		case climate::CLIMATE_MODE_COOL:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000011;	
			break;
		case climate::CLIMATE_MODE_DRY:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000010;	
			break;
		case climate::CLIMATE_MODE_FAN_ONLY:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000111;	
			break;
		case climate::CLIMATE_MODE_HEAT:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000001;	
			break;
	}

	// Nastavujeme režim ventilátora
	if (this->fan_mode.has_value()) {
		switch(*this->fan_mode) {
			case climate::CLIMATE_FAN_AUTO:
				dataTX[8]	+= 0b00000000;
				dataTX[10]	+= 0b00000000;
				break;
			case climate::CLIMATE_FAN_QUIET:
				dataTX[8]	+= 0b10000000;
				dataTX[10]	+= 0b00000000;
				break;
			case climate::CLIMATE_FAN_LOW:
				dataTX[8]	+= 0b00000000;
				dataTX[10]	+= 0b00000001;
				break;
			case climate::CLIMATE_FAN_MIDDLE:
				dataTX[8]	+= 0b00000000;
				dataTX[10]	+= 0b00000110;
				break;
			case climate::CLIMATE_FAN_MEDIUM:
				dataTX[8]	+= 0b00000000;
				dataTX[10]	+= 0b00000011;
				break;
			case climate::CLIMATE_FAN_HIGH:
				dataTX[8]	+= 0b00000000;
				dataTX[10]	+= 0b00000111;
				break;
			case climate::CLIMATE_FAN_FOCUS:
				dataTX[8]	+= 0b00000000;
				dataTX[10]	+= 0b00000101;
				break;
			case climate::CLIMATE_FAN_DIFFUSE:
				dataTX[8]	+= 0b01000000;
				dataTX[10]	+= 0b00000000;
				break;
		}
	}
	
	// Nastavujeme režim kývania lamiel
	switch(this->swing_mode) {
		case climate::CLIMATE_SWING_OFF:
			dataTX[10]	+= 0b00000000;
			dataTX[11]	+= 0b00000000;
			break;
		case climate::CLIMATE_SWING_VERTICAL:
			dataTX[10]	+= 0b00111000;
			dataTX[11]	+= 0b00000000;
			break;
		case climate::CLIMATE_SWING_HORIZONTAL:
			dataTX[10]	+= 0b00000000;
			dataTX[11]	+= 0b00001000;
			break;
		case climate::CLIMATE_SWING_BOTH:
			dataTX[10]	+= 0b00111000;
			dataTX[11]	+= 0b00001000;  
			break;
	}
	
	// Nastavujeme presety klimatizácie
	if (this->preset.has_value()) {
		switch(*this->preset) {
			case ClimatePreset::CLIMATE_PRESET_NONE:
				break;
			case ClimatePreset::CLIMATE_PRESET_ECO:
				dataTX[7]	+= 0b10000000;
				break;
			case ClimatePreset::CLIMATE_PRESET_SLEEP:
				dataTX[19]	+= 0b00000001;
				break;
			case ClimatePreset::CLIMATE_PRESET_COMFORT:
				dataTX[8]	+= 0b00010000;
				break;
		}
	}

        //Režim lamiel
		//	Vertikálna lamela
		//		Kývanie vertikálnej lamely [bajt 10, maska 00111000]:
		//			000 - Kývanie vypnuté, lamela v poslednej polohe alebo vo fixácii
		//			111 - Kývanie zapnuté vo zvolenom režime
		//		Režim kývania vertikálnej lamely (režim fixácie nehrá rolu, ak je kývanie zapnuté) [bajt 32, maska 00011000]:
		//			01 - kývanie zhora nadol, PREDVOLENE
		//			10 - kývanie v hornej polovici
		//			11 - kývanie v dolnej polovici
		//		Režim fixácie lamely (režim kývania nehrá rolu, ak je kývanie vypnuté) [bajt 32, maska 00000111]:
		//			000 - bez fixácie, PREDVOLENE
		//			001 - fixácia hore
		//			010 - fixácia medzi vrchom a stredom
		//			011 - fixácia v strede
		//			100 - fixácia medzi stredom a spodkom
		//			101 - fixácia dole
		//	Horizontálne lamely
		//		Kývanie horizontálnych lamiel [bajt 11, maska 00001000]:
		//			0 - Kývanie vypnuté, lamely v poslednej polohe alebo vo fixácii
		//			1 - Kývanie zapnuté vo zvolenom režime
		//		Režim kývania horizontálnych lamiel (režim fixácie nehrá rolu, ak je kývanie zapnuté) [bajt 33, maska 00111000]:
		//			001 - kývanie zľava doprava, PREDVOLENE
		//			010 - kývanie vľavo
		//			011 - kývanie v strede
		//			100 - kývanie vpravo
		//		Režim fixácie horizontálnych lamiel (režim kývania nehrá rolu, ak je kývanie vypnuté) [bajt 33, maska 00000111]:
		//			000 - bez fixácie, PREDVOLENE
		//			001 - fixácia vľavo
		//			010 - fixácia medzi ľavou stranou a stredom
		//			011 - fixácia v strede
		//			100 - fixácia medzi stredom a pravou stranou
		//			101 - fixácia vpravo
		
		
	// Nastavujeme režim kývania vertikálnej lamely
	switch(vertical_swing_direction_) {
		case VerticalSwingDirection::UP_DOWN:
			dataTX[32]	+= 0b00001000;
			ESP_LOGD("TCL", "Vertical swing: up-down");
			break;
		case VerticalSwingDirection::UPSIDE:
			dataTX[32]	+= 0b00010000;
			ESP_LOGD("TCL", "Vertical swing: upper");
			break;
		case VerticalSwingDirection::DOWNSIDE:
			dataTX[32]	+= 0b00011000;
			ESP_LOGD("TCL", "Vertical swing: downer");
			break;
	}
	// Nastavujeme režim kývania horizontálnych lamiel
	switch(horizontal_swing_direction_) {
		case HorizontalSwingDirection::LEFT_RIGHT:
			dataTX[33]	+= 0b00001000;
			ESP_LOGD("TCL", "Horizontal swing: left-right");
			break;
		case HorizontalSwingDirection::LEFTSIDE:
			dataTX[33]	+= 0b00010000;
			ESP_LOGD("TCL", "Horizontal swing: lefter");
			break;
		case HorizontalSwingDirection::CENTER:
			dataTX[33]	+= 0b00011000;
			ESP_LOGD("TCL", "Horizontal swing: center");
			break;
		case HorizontalSwingDirection::RIGHTSIDE:
			dataTX[33]	+= 0b00100000;
			ESP_LOGD("TCL", "Horizontal swing: righter");
			break;
	}
	// Nastavujeme polohu fixácie vertikálnej lamely
	switch(vertical_direction_) {
		case AirflowVerticalDirection::LAST:
			dataTX[32]	+= 0b00000000;
			ESP_LOGD("TCL", "Vertical fix: last position");
			break;
		case AirflowVerticalDirection::MAX_UP:
			dataTX[32]	+= 0b00000001;
			ESP_LOGD("TCL", "Vertical fix: up");
			break;
		case AirflowVerticalDirection::UP:
			dataTX[32]	+= 0b00000010;
			ESP_LOGD("TCL", "Vertical fix: upper");
			break;
		case AirflowVerticalDirection::CENTER:
			dataTX[32]	+= 0b00000011;
			ESP_LOGD("TCL", "Vertical fix: center");
			break;
		case AirflowVerticalDirection::DOWN:
			dataTX[32]	+= 0b00000100;
			ESP_LOGD("TCL", "Vertical fix: downer");
			break;
		case AirflowVerticalDirection::MAX_DOWN:
			dataTX[32]	+= 0b00000101;
			ESP_LOGD("TCL", "Vertical fix: down");
			break;
	}
	// Nastavujeme polohu fixácie horizontálnych lamiel
	switch(horizontal_direction_) {
		case AirflowHorizontalDirection::LAST:
			dataTX[33]	+= 0b00000000;
			ESP_LOGD("TCL", "Horizontal fix: last position");
			break;
		case AirflowHorizontalDirection::MAX_LEFT:
			dataTX[33]	+= 0b00000001;
			ESP_LOGD("TCL", "Horizontal fix: left");
			break;
		case AirflowHorizontalDirection::LEFT:
			dataTX[33]	+= 0b00000010;
			ESP_LOGD("TCL", "Horizontal fix: lefter");
			break;
		case AirflowHorizontalDirection::CENTER:
			dataTX[33]	+= 0b00000011;
			ESP_LOGD("TCL", "Horizontal fix: center");
			break;
		case AirflowHorizontalDirection::RIGHT:
			dataTX[33]	+= 0b00000100;
			ESP_LOGD("TCL", "Horizontal fix: righter");
			break;
		case AirflowHorizontalDirection::MAX_RIGHT:
			dataTX[33]	+= 0b00000101;
			ESP_LOGD("TCL", "Horizontal fix: right");
			break;
	}

	// Nastavenie teploty
	dataTX[9] = target_temperature_set;
		
	// Skladáme pole bajtov na odoslanie do klimatizácie
	dataTX[0] = 0xBB;	//štartovací bajt hlavičky
	dataTX[1] = 0x00;	//štartovací bajt hlavičky
	dataTX[2] = 0x01;	//štartovací bajt hlavičky
	dataTX[3] = 0x03;	//0x03 - riadenie, 0x04 - dopyt
	dataTX[4] = 0x20;	//0x20 - riadenie, 0x19 - dopyt
	dataTX[5] = 0x03;	//??
	dataTX[6] = 0x01;	//??
	//dataTX[7] = 0x64;	//eco,display,beep,ontimerenable, offtimerenable,power,0,0
	//dataTX[8] = 0x08;	//mute,0,turbo,health, mode(4) mode 01 heat, 02 dry, 03 cool, 07 fan, 08 auto, health(+16), 41=turbo-heat 43=turbo-cool (turbo = 0x40+ 0x01..0x08)
	//dataTX[9] = 0x0f;	//0 -31 ;    15 - 16 0,0,0,0, temp(4) settemp 31 - x
	//dataTX[10] = 0x00;	//0,timerindicator,swingv(3),fan(3) fan+swing modes //0=auto 1=low 2=med 3=high
	//dataTX[11] = 0x00;	//0,offtimer(6),0
	dataTX[12] = 0x00;	//fahrenheit,ontimer(6),0 cf 80=f 0=c
	dataTX[13] = 0x01;	//??
	dataTX[14] = 0x00;	//0,0,halfdegree,0,0,0,0,0
	dataTX[15] = 0x00;	//??
	dataTX[16] = 0x00;	//??
	dataTX[17] = 0x00;	//??
	dataTX[18] = 0x00;	//??
	//dataTX[19] = 0x00;	//sleep on = 1 off=0
	dataTX[20] = 0x00;	//??
	dataTX[21] = 0x00;	//??
	dataTX[22] = 0x00;	//??
	dataTX[23] = 0x00;	//??
	dataTX[24] = 0x00;	//??
	dataTX[25] = 0x00;	//??
	dataTX[26] = 0x00;	//??
	dataTX[27] = 0x00;	//??
	dataTX[28] = 0x00;	//??
	dataTX[29] = 0x00;	//??
	dataTX[30] = 0x00;	//??
	dataTX[31] = 0x00;	//??
	//dataTX[32] = 0x00;	//0,0,0,režim vertikálneho kývania(2),režim vertikálnej fixácie(3)
	//dataTX[33] = 0x00;	//0,0,režim horizontálneho kývania(3),režim horizontálnej fixácie(3)
	dataTX[34] = 0x00;	//??
	dataTX[35] = 0x00;	//??
	dataTX[36] = 0x00;	//??
	dataTX[37] = 0xFF;	//Kontrolný súčet
	dataTX[37] = tclacClimate::getChecksum(dataTX, sizeof(dataTX));

	tclacClimate::sendData(dataTX, sizeof(dataTX));
	allow_take_control = false;
	is_call_control = false;
}

// Odoslanie dát do klimatizácie.
// Stratégia spoľahlivosti na linke bez prevodníka úrovní (3.3V -> 5V UART):
//  1) „Počúvaj, potom hovor“: nevysielať, kým vysiela samotná klimatizácia
//     (alebo čakáme na jej odpoveď na dopyt) — riadiaca doska klimatizácie je de facto
//     poloduplexná a stráca rámce prijaté počas vlastného vysielania.
//  2) TX_REPEAT opakovaní rámca s pauzou TX_REPEAT_SPACING_MS — každé opakovanie
//     je samostatný plnohodnotný pokus (tesná fronta sa prehltne ako jeden rámec).
// Rámce sú idempotentné: klimatizácia použije prvý správne prijatý.
// Všetko je neblokujúce (set_timeout), loop() sa nezmrazuje.
void tclacClimate::sendData(uint8_t * message, uint8_t size) {
	tclacClimate::dataShow(1,1);
	this->tx_size_ = size;  // message vždy ukazuje na dataTX (člen triedy)
	for (uint8_t k = 0; k < TX_REPEAT; k++) {
		if (k == 0) {
			this->try_send_frame_(0, TX_MAX_DEFERS);
		} else {
			esphome::App.scheduler.set_timeout("tx_rep", k * TX_REPEAT_SPACING_MS, [this, k]() {
				this->try_send_frame_(k, TX_MAX_DEFERS);
			});
		}
	}
	ESP_LOGD("TCL", "Message to TCL queued (x%d, spacing %dms)", TX_REPEAT, TX_REPEAT_SPACING_MS);
	tclacClimate::dataShow(1,0);
}

// Je linka voľná na vysielanie
bool tclacClimate::bus_quiet_() {
	const uint32_t now = millis();
	// práve teraz prebieha príjem
	if (esphome::uart::UARTDevice::available() > 0)
		return false;
	// príjem bol práve teraz — rámec môže pokračovať
	if (now - this->last_rx_ms_ < BUS_QUIET_MS)
		return false;
	// odoslali sme dopyt a odpoveď ešte nezačala prichádzať — nevstupujeme
	if (now - this->poll_sent_ms_ < POLL_RESPONSE_WINDOW_MS
		&& (int32_t)(this->last_rx_ms_ - this->poll_sent_ms_) < 0)
		return false;
	return true;
}

// Odoslať rámec, ak je linka voľná; inak odložiť o BUS_QUIET_MS
void tclacClimate::try_send_frame_(uint8_t attempt, uint8_t defers_left) {
	if (!this->bus_quiet_() && defers_left > 0) {
		esphome::App.scheduler.set_timeout("tx_def", BUS_QUIET_MS,
			[this, attempt, defers_left]() {
				this->try_send_frame_(attempt, defers_left - 1);
			});
		return;
	}
	this->esphome::uart::UARTDevice::write_array(this->dataTX, this->tx_size_);
	this->esphome::uart::UARTDevice::flush();
}

// Prevod bajtu do čitateľného formátu
String tclacClimate::getHex(uint8_t *message, uint8_t size) {
	String raw;
	for (int i = 0; i < size; i++) {
		raw += "\n" + String(message[i]);
	}
	raw.toUpperCase();
	return raw;
}

// Výpočet kontrolného súčtu
uint8_t tclacClimate::getChecksum(const uint8_t * message, size_t size) {
	uint8_t position = size - 1;
	uint8_t crc = 0;
	for (int i = 0; i < position; i++)
		crc ^= message[i];
	return crc;
}

// Blikáme LED diódami
void tclacClimate::dataShow(bool flow, bool shine) {
	if (module_display_status_){
		if (flow == 0){
			if (shine == 1){
#ifdef CONF_RX_LED
				this->rx_led_pin_->digital_write(true);
#endif
			} else {
#ifdef CONF_RX_LED
				this->rx_led_pin_->digital_write(false);
#endif
			}
		}
		if (flow == 1) {
			if (shine == 1){
#ifdef CONF_TX_LED
				this->tx_led_pin_->digital_write(true);
#endif
			} else {
#ifdef CONF_TX_LED
				this->tx_led_pin_->digital_write(false);
#endif
			}
		}
	}
}

// Práca s dátami z konfigurácie

// Získanie stavu pípania
void tclacClimate::set_beeper_state(bool state) {
	this->beeper_status_ = state;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Získanie stavu displeja klimatizácie
void tclacClimate::set_display_state(bool disp_state) {
	this->display_status_ = disp_state;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Získanie stavu režimu vynúteného použitia nastavení
void tclacClimate::set_force_mode_state(bool f_state) {
	this->force_mode_status_ = f_state;
}
// Získanie pinu LED príjmu dát
#ifdef CONF_RX_LED
void tclacClimate::set_rx_led_pin(GPIOPin *rx_led_pin) {
	this->rx_led_pin_ = rx_led_pin;
}
#endif
// Získanie pinu LED vysielania dát
#ifdef CONF_TX_LED
void tclacClimate::set_tx_led_pin(GPIOPin *tx_led_pin) {
	this->tx_led_pin_ = tx_led_pin;
}
#endif
// Získanie stavu LED komunikácie modulu
void tclacClimate::set_module_display_state(bool d_state) {
	this->module_display_status_ = d_state;
}
// Získanie režimu fixácie vertikálnej lamely
void tclacClimate::set_vertical_airflow(AirflowVerticalDirection v_airflow) {
	this->vertical_direction_ = v_airflow;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Získanie režimu fixácie horizontálnych lamiel
void tclacClimate::set_horizontal_airflow(AirflowHorizontalDirection h_airflow) {
	this->horizontal_direction_ = h_airflow;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Získanie režimu kývania vertikálnej lamely
void tclacClimate::set_vertical_swing_direction(VerticalSwingDirection vs_direction) {
	this->vertical_swing_direction_ = vs_direction;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Získanie dostupných prevádzkových režimov klimatizácie
void tclacClimate::set_supported_modes(climate::ClimateModeMask modes) {
	this->supported_modes_ = modes;
	ESP_LOGD("TCL", "Set up Modes");
}
// Získanie režimu kývania horizontálnych lamiel
void tclacClimate::set_horizontal_swing_direction(HorizontalSwingDirection hs_direction) {
	horizontal_swing_direction_ = hs_direction;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Získanie dostupných rýchlostí ventilátora
void tclacClimate::set_supported_fan_modes(climate::ClimateFanModeMask fan_modes){
	this->supported_fan_modes_ = fan_modes;
}
// Získanie dostupných režimov kývania lamiel
void tclacClimate::set_supported_swing_modes(climate::ClimateSwingModeMask swing_modes) {
	this->supported_swing_modes_ = swing_modes;
}
// Získanie dostupných presetov
void tclacClimate::set_supported_presets(climate::ClimatePresetMask presets) {
  this->supported_presets_ = presets;
}


}
}
