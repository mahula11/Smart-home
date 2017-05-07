
#include <Arduino.h>
#include <Streaming.h>

#include "device.h"

Device * Device::s_instance = nullptr;
//ArrivedConfiguration Device::s_arrivedConf;
MCP_CAN Device::s_can(10);
Configuration Device::s_conf;
ArrivedConfiguration * Device::s_arrivedConf = nullptr;
//uint16_t Device::s_deviceAddress = 0;
//bool Device::s_firstConfMessage = true;
//byte Device::s_numberOfMsgFromConf = 0;
//byte Device::s_numberOfArrivedMsg = 0;

Device::Device() {
	s_instance = this;
}


Device::~Device() {
}

void Device::init(EepromConf & eepromConf) {
	_eepromConf = eepromConf;
	s_arrivedConf = nullptr;
	//s_deviceAddress = _eepromConf.getMacAddress();
	// Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
	if (s_can.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) == CAN_OK) {
		Serial << F("MCP2515 Initialized Successfully\n");
	} else {
		Serial << F("Error Initializing MCP2515\n");
	}

	s_can.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
	pinMode(CAN0_INT, INPUT);   // Configuring pin for /INT input

	attachInterrupt(digitalPinToInterrupt(CAN0_INT), interruptFromCanBus, FALLING);

	//* skontroluje, ci mame konfiguracne spravy. pokial nie, tak treba poziadat o konfiguraciu
	if (_eepromConf.getCountOfConf()) {
		//* nacitaj conf
		s_conf.setConfiguration(_eepromConf.readConf());
	} else {
		//* pocet je 0, takze ziadnu konfiguraciu v eeprom nemame, treba poziadat o novu.
		sendRequestForConfiguration();
	}
}

void Device::update() {
	//* s_arrivedConf is read in interruptFromCanBus
	//* if arrived configuration is complet, then copy it to eeprom
	if (s_arrivedConf && s_arrivedConf->isComplet()) {		
		//* zapiseme do eeprom
		_eepromConf.writeConf(s_arrivedConf->getConf());
		//* zapiseme do Configuration
		s_conf.setConfiguration(_eepromConf.readConf());
		//* zmazeme _arriveConf
		delete s_arrivedConf;
		s_arrivedConf = nullptr;
	}
}

void Device::interruptFromCanBus() {
	//Device * instance = getInstance();
	uint32_t canId;
	byte len = 0;
	byte rxBuf[8];
	s_can.readMsgBuf(&canId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
	uint16_t macID = CanExt::getDeviceID(canId);

	if (CanExt::isConfigurationMsgFlag(canId) && macID == s_conf.getMacAddress()) {
		if (s_arrivedConf == nullptr) {
			s_arrivedConf = new ArrivedConfiguration();
		}
		//* ked pride prva konfiguracna sprava, tak v datach, v prvom byte mame pocet sprav, ktore este pridu
		if (s_arrivedConf->getCount()) {
			CONF_MESSAGE msg(macID, len, rxBuf[0]);
			s_arrivedConf->addConf(msg);
		} else {
			//* prisla prva sprava, prislo cislo, ktore je pocet sprav, ktore este pridu z CanConf
			s_arrivedConf->setCount(rxBuf[0]);
		}
	} //else if () {

	//}
}

void Device::sendRequestForConfiguration() {
	uint32_t canID;
	canID = s_conf.getMacAddress();
	CanExt::setConfiguationMsgFlag(canID);
	
	byte sndStat = s_can.sendMsgBuf(canID, 0, _data);
	if (sndStat != CAN_OK) {
		Serial << F("Error Sending Configuration\n");
	} else {
		Serial << F("Configuration Sent Successfully\n");
		//Serial.println(id);
	}
}