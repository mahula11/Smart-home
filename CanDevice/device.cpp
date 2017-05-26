
#include <Arduino.h>

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

void Device::init() {
	//_eepromConf = eepromConf;
	s_arrivedConf = nullptr;
	//s_deviceAddress = _eepromConf.getMacAddress();
	// Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
	if (s_can.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) == CAN_OK) {
		Serial << F("MCP2515 Initialized Successfully\n");
	} else {
		Serial << F("MCP2515 Initializing Error\n");
	}

	s_can.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
	pinMode(CAN0_INT, INPUT);   // Configuring pin for /INT input

	attachInterrupt(digitalPinToInterrupt(CAN0_INT), interruptFromCanBus, FALLING);

	//* skontroluje, ci mame konfiguracne spravy. pokial nie, tak treba poziadat o konfiguraciu
	if (eepromConf.getCountOfConf()) {
		//* nacitaj conf
		s_conf.setConfiguration(eepromConf.readConf());
		setPinModes();
	} else {
		//* pocet je 0, takze ziadnu konfiguraciu v eeprom nemame, treba poziadat o novu.
		sendRequestForConfiguration();
	}
}

void Device::setPinModes() {
	for (int i = 0; i < s_conf.getCount(); i++) {
		switch (s_conf.getConf(i)->getType()) {
			case DEVICE_TYPE_LIGHT:
			case DEVICE_TYPE_LIGHT_WITH_DIMMER:
				pinMode(((CONF_DATA_LIGHT*)s_conf.getConf(i))->_gpio, OUTPUT);
				break;
			case DEVICE_TYPE_SOCKET:
				//pinMode(CanExt::getLightGPIO(pData), OUTPUT);
				break;
			
			case DEVICE_TYPE_PUSH_BUTTON:
			case DEVICE_TYPE_STAIR_CASE_SWITCH:
				break;
			case DEVICE_TYPE_SWITCH:
				pinMode(((CONF_DATA_SWITCH*)s_conf.getConf(i))->_gpio, INPUT_PULLUP);
				break;
		}
	}
}

void Device::checkModifiedData(DATA_BASE * pConfData, byte index) {
	//* tato modifikacia je nastavena vtedy, ked pridu nove data cez CanBus 
	//* napr prepnutie vypinaca vysle spravu s novou hodnotou pre ziarovky, ktore pocuvaju pre dany vypinac
	if (s_conf.isModifiedValue()) {
		if (s_conf.getConfValue(index)._modified) {
			switch (pConfData->getType()) {
				case DEVICE_TYPE_LIGHT:
					digitalWrite(((CONF_DATA_LIGHT*)pConfData)->_gpio, s_conf.getConfValue(index)._value);
					break;
				case DEVICE_TYPE_LIGHT_WITH_DIMMER:
					break;
			}
		}
	}
}

void Device::checkValueOnPins(DATA_BASE * pConfData, byte index) {
	switch (pConfData->getType()) {
		case DEVICE_TYPE_SWITCH:
		case DEVICE_TYPE_PUSH_BUTTON:
		case DEVICE_TYPE_STAIR_CASE_SWITCH:
		{
			byte pinValue = digitalRead(((TRAFFIC_DATA_SWITCH*)pConfData)->_gpio);
			//* if values are different, then send message to the lights
			if (pinValue != s_conf.getConfValue(index)._value) {
				//* send message
				CanID canID = s_conf.getMacAddress();
				CanExt::setMsgFlagFromSwitch(canID);
				MsgData data;
				CanExt::setSwitchValue_toMsg(&data, pinValue);
				CanExt::setSwitchGPIO_toMsg(&data, ((TRAFFIC_DATA_SWITCH*)pConfData)->_gpio); //   CanExt::getSwitchGPIO_fromConf(pData));
				byte sndStat = s_can.sendMsgBuf(canID, 2, data[0]);
				if (sndStat != CAN_OK) {
					Serial << F("Error Sending Message\n");
				} else {
					Serial << F("Message Sent Successfully\n");
				}

				//* set value without modify flag
				s_conf.setConfValue(index, pinValue, false);
			}
		}
		break;
	}
}

void Device::update() {
	//* s_arrivedConf is read in interruptFromCanBus
	//* if arrived configuration is complet, then copy it to eeprom
	if (s_arrivedConf && s_arrivedConf->isComplet()) {		
		//* zapiseme do eeprom
		eepromConf.writeConf(s_arrivedConf->getConf());
		//* zapiseme do Configuration
		s_conf.setConfiguration(eepromConf.readConf());
		setPinModes();
		//* zmazeme _arriveConf
		delete s_arrivedConf;
		s_arrivedConf = nullptr;
	}

	for (int index = 0; index < s_conf.getCount(); index++) {
		//ConfData * pData = & s_conf.getConf(index)->_confData;
		DATA_BASE * pConfData = s_conf.getConf(index);
		
		//* pozrieme, ci neprisli nove data
		checkModifiedData(pConfData, index);

		//* spracujeme zmenene hodnoty na PINoch
		checkValueOnPins(pConfData, index);
	}
}

void Device::interruptFromCanBus() {
	//Device * instance = getInstance();
	CanID canId;
	byte len = 0;
	MsgData rxBuf;
	s_can.readMsgBuf(&canId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
	MacID receivedDeviceID = CanExt::getDeviceID(canId);

	if (CanExt::isMsgFlagFromConfiguration(canId) && receivedDeviceID == s_conf.getMacAddress()) {
		//* messages from configuration server
		if (s_arrivedConf == nullptr) {
			s_arrivedConf = new ArrivedConfiguration();
		}
		//* ked pride prva konfiguracna sprava, tak v datach, v prvom byte mame pocet sprav, ktore este pridu
		//* getCount vrati nulu, pretoze este neviemme pocet sprav
		if (s_arrivedConf->getCount()) {
			//CONF_DATA msg(len, rxBuf[0]);
			DATA_BASE * pConfData;
			switch (DATA_BASE::getType(rxBuf)) {
				case DEVICE_TYPE_SWITCH:
					pConfData = new CONF_DATA_SWITCH(rxBuf);					
					break;
				case DEVICE_TYPE_LIGHT:
					pConfData = new CONF_DATA_LIGHT(rxBuf);
					break;
			}
			s_arrivedConf->addConf(pConfData);
		} else {
			//* prisla prva sprava, prislo cislo, ktore je pocet sprav, ktore este pridu z CanConf
			s_arrivedConf->setCount(rxBuf[0]);
		}
	} else if (CanExt::isMsgFlagFromSwitch(canId)) { //* message from switch to lights
		//* teraz skontrolovat ci ID vypinaca patri niektoremu vypinacu v nasej konfiguracii (pre niektoru ziarovku)
		TRAFFIC_DATA_SWITCH switchData(rxBuf);		
		for (byte i = 0; i < s_conf.getCount(); i++) {			
			//* vyhladavame len typ "ziarovky" a potom ich IDcka vypinacov
			//* 0 - typ (ziarovka), 1 - gpio, 2 - id vypinaca
			DATA_BASE * pData = s_conf.getConf(i);
			if (pData->getType() == DEVICE_TYPE_LIGHT && ((CONF_DATA_LIGHT*)pData)->_switchMacID == receivedDeviceID && ((CONF_DATA_LIGHT*)pData)->_switchGPIO == switchData._gpio) {
				s_conf.setConfValue(i, switchData._value, true);
			}
		}
	}
}

void Device::sendRequestForConfiguration() {
	CanID canID = s_conf.getMacAddress();
	CanExt::setMsgFlagForConfiguration(canID);
	byte data;
	byte sndStat = s_can.sendMsgBuf(canID, 0, &data);
	if (sndStat != CAN_OK) {
		Serial << F("Error Sending Configuration\n");
	} else {
		Serial << F("Configuration Sent Successfully\n");
		//Serial.println(id);
	}
}