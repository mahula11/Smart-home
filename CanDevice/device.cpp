#include <Arduino.h>
#include "device.h"

Device * Device::s_instance = nullptr;
ArrivedConfiguration Device::s_arrivedConf;
MCP_CAN Device::s_can(10);
uint16_t Device::s_deviceAddress = 0;
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
	s_deviceAddress = _eepromConf.getMacAddress();
	// Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
	if (s_can.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) == CAN_OK)
		Serial.println("MCP2515 Initialized Successfully!");
	else
		Serial.println("Error Initializing MCP2515...");

	s_can.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
	pinMode(CAN0_INT, INPUT);   // Configuring pin for /INT input

	attachInterrupt(digitalPinToInterrupt(CAN0_INT), interruptFromCanBus, FALLING);

	sendRequestForConfiguration();
}

void Device::update() {
	//* if arrived configuration is complet, then copy it to eeprom
	if (s_arrivedConf.isComplet()) {
		_eepromConf.setCountOfConf(s_arrivedConf.getCount());
		for (uint8_t i = 0; i < s_arrivedConf.getCount(); i++) {
			_eepromConf.writeConf(i, s_arrivedConf.getConf(i));
		}
	}
}

void Device::interruptFromCanBus() {
	//Device * instance = getInstance();
	uint32_t rxId;
	byte len = 0;
	byte rxBuf[8];
	s_can.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)

	uint16_t macID = CanExt::getDeviceID(rxId);

	if (CanExt::isConfigurationMsg(rxId) && macID == s_deviceAddress) {
		//* ked pride prva konfiguracna sprava, tak v datach, v prvom byte mame pocet sprav, ktore este pridu
		if (s_arrivedConf.getCount()) {
			CONF_MESSAGE msg(macID, len, rxBuf[0]);
			s_arrivedConf.addConf(msg);
		} else {
			//* prisla prva sprava, prislo cislo, ktore je pocet sprav, ktore este pridu z CanConf
			s_arrivedConf.setCount(rxBuf[0]);			
		}
	} //else if () {

	//}
}

void Device::sendRequestForConfiguration() {
	INT32U id;
	id = 234;
	bitSet(id, 31);				//* set extended message
	bitSet(id, 30);				//* set remote flag
	bitSet(id, 28);				//* set bit for request of configuration
	byte sndStat = s_can.sendMsgBuf(id, 0, _data);
	if (sndStat != CAN_OK) {
		Serial.println("Error Sending Configuration!");
	} else {
		Serial.println("Configuration Sent Successfully:");
		//Serial.println(id);
	}
}