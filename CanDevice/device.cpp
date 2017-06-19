
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
		DEBUG(F("MCP2515 Initialized Successfully"));
	} else {
		DEBUG(F("MCP2515 Initializing Error"));
	}

	s_can.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
	pinMode(CAN0_INT, INPUT);   // Configuring pin for /INT input

	attachInterrupt(digitalPinToInterrupt(CAN0_INT), interruptFromCanBus, FALLING);

	//* skontroluje, ci mame konfiguracne spravy. pokial nie, tak treba poziadat o konfiguraciu
	if (eepromConf.getCountOfConf()) {
		//* nacitaj conf
		s_conf.setConfiguration(eepromConf.readConf());
		setPins();
	} else {
		//* pocet je 0, takze ziadnu konfiguraciu v eeprom nemame, treba poziadat o novu.
		sendRequest_forConfiguration();
	}
}

void Device::setPins() {
	for (int i = 0; i < s_conf.getCount(); i++) {
		switch (s_conf.getConf(i)->getType()) {
			case DEVICE_TYPE_LIGHT:
			case DEVICE_TYPE_LIGHT_WITH_DIMMER:
				pinMode(((CConfDataLight*)s_conf.getConf(i))->_gpio, OUTPUT);
				sendRequest_askSwitchForValue(((CConfDataLight*)s_conf.getConf(i))->_switchMacID, ((CConfDataLight*)s_conf.getConf(i))->_switchGPIO);
				break;
			case DEVICE_TYPE_SOCKET:
				//pinMode(CanExt::getLightGPIO(pData), OUTPUT);
				break;
			
			case DEVICE_TYPE_PUSH_BUTTON:
			case DEVICE_TYPE_STAIR_CASE_SWITCH:
				break;
			case DEVICE_TYPE_SWITCH:
				pinMode(((CConfDataSwitch*)s_conf.getConf(i))->_gpio, INPUT_PULLUP);
				break;
		}
	}
}

void Device::checkModifiedData(CDataBase * pConfData, byte index) {
	//* tato modifikacia je nastavena vtedy, ked pridu nove data cez CanBus 
	//* napr prepnutie vypinaca vysle spravu s novou hodnotou pre ziarovky, ktore pocuvaju pre dany vypinac
	if (s_conf.isModifiedValue()) {
		if (s_conf.getConfValue(index)._modified) {
			switch (pConfData->getType()) {
				case DEVICE_TYPE_LIGHT:
					DEBUG("Set light on PIN:" << ((CConfDataLight*)pConfData)->_gpio << " value:" << s_conf.getConfValue(index)._value);
					digitalWrite(((CConfDataLight*)pConfData)->_gpio, s_conf.getConfValue(index)._value);
					break;
				case DEVICE_TYPE_LIGHT_WITH_DIMMER:
					break;
			}
			s_conf.getConfValue(index)._modified = false;
		}
	}
}

void Device::checkValueOnPins(CDataBase * pConfData, byte index) {
	switch (pConfData->getType()) {
		case DEVICE_TYPE_PUSH_BUTTON:
		case DEVICE_TYPE_STAIR_CASE_SWITCH:
			break;
		case DEVICE_TYPE_SWITCH:
		{
			byte pinValue = digitalRead(((CConfDataSwitch*)pConfData)->_gpio);			
			//* if values are different, then send message to the lights
			if (pinValue != s_conf.getConfValue(index)._value) {
				DEBUG(VAR(pinValue));
				DEBUG(F("_value:") << s_conf.getConfValue(index)._value);

				sendRequest_fromSwitch(((CConfDataSwitch*)pConfData)->_gpio, pinValue);

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
		DEBUG("New conf will be processed");
		//* zapiseme do eeprom
		//eepromConf.writeConf(s_arrivedConf->getConf());
		////* zapiseme do Configuration
		//s_conf.setConfiguration(eepromConf.readConf());
		//setPins();
		////* zmazeme _arriveConf
		//delete s_arrivedConf;
		//s_arrivedConf = nullptr;
	}

	for (int index = 0; index < s_conf.getCount(); index++) {
		//ConfData * pData = & s_conf.getConf(index)->_confData;
		CDataBase * pConfData = s_conf.getConf(index);
		
		//* pozrieme, ci neprisli nove data
		checkModifiedData(pConfData, index);

		//* spracujeme zmenene hodnoty na PINoch
		checkValueOnPins(pConfData, index);
	}
	s_conf.resetModifiedValue();
}

void Device::interruptFromCanBus() {
	//Device * instance = getInstance();
	CCanID canId;
	byte len = 0;
	MsgData rxBuf;
	s_can.readMsgBuf(&canId._canID, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)

	DEBUG(F("Received CanID:") << canId._canID << F(",MacID:") << canId.getMacID()
		<< F(",fromConf:") << canId.hasFlag_fromConfiguration()
		<< F(",fromSwitch:") << canId.hasFlag_fromSwitch()
		<< F(",askSwitchForVal") << canId.hasFlag_askSwitchForValue());

	if (canId.hasFlag_fromConfiguration() && canId.getMacID() == eepromConf.getMacAddress()) {
		//* messages from configuration server
		if (s_arrivedConf == nullptr) {
			s_arrivedConf = new ArrivedConfiguration();
		}
		//* ked pride prva konfiguracna sprava, tak v datach, v prvom byte mame pocet sprav, ktore este pridu
		//* getCount vrati nulu, pretoze este neviemme pocet sprav
		if (s_arrivedConf->getCount()) {
			byte type = rxBuf[0];
			DEBUG(F("Conf arrived for:") << type);
			CDataBase * pConfData;
			switch (type) {
				case DEVICE_TYPE_SWITCH:
					pConfData = new CConfDataSwitch(rxBuf);					
					break;
				case DEVICE_TYPE_LIGHT:
					pConfData = new CConfDataLight(rxBuf);
					break;
			}
			s_arrivedConf->addConf(pConfData);
		} else {
			//* prisla prva sprava, prislo cislo, ktore je pocet sprav, ktore este pridu z CanConf
			DEBUG(F("Number of confs arrived:") << rxBuf[0]);
			s_arrivedConf->setCount(rxBuf[0]);
		}
	} else if (canId.hasFlag_fromSwitch()) { //* message from switch to lights
		DEBUG(F("Messsage from switch arrived"));
		//* teraz skontrolovat ci ID vypinaca patri niektoremu vypinacu v nasej konfiguracii (pre niektoru ziarovku)
		CTrafficDataSwitch switchData(rxBuf);		
		for (byte i = 0; i < s_conf.getCount(); i++) {			
			//* vyhladavame len typ "ziarovky" a potom ich IDcka vypinacov
			//* 0 - typ (ziarovka), 1 - gpio, 2 - id vypinaca
			CDataBase * pData = s_conf.getConf(i);
			if (pData->getType() == DEVICE_TYPE_LIGHT && ((CConfDataLight*)pData)->_switchMacID == canId.getMacID() && ((CConfDataLight*)pData)->_switchGPIO == switchData._gpio) {
				s_conf.setConfValue(i, switchData._value, true);
			}
		}
	} else if (canId.hasFlag_askSwitchForValue() && canId.getMacID() == eepromConf.getMacAddress()) {
		DEBUG(F("Message from light, asking switch for value"));

		byte gpio = rxBuf[0];
		byte pinValue = digitalRead(gpio);
		sendRequest_fromSwitch(gpio, pinValue);
	}
}

void Device::sendRequest_askSwitchForValue(MacID macId, uint8_t pin) {
	DEBUG(F("Ask switch ") << macId << F(", pin ") << pin << F(" for value"));
	CCanID canID;
	canID.setMacID(macId);
	canID.setFlag_askSwitchForValue();
	byte data = pin;
	byte sndStat = s_can.sendMsgBuf(canID._canID, 1, &data);
	if (sndStat != CAN_OK) {
		DEBUG(F("Error Sending Configuration"));
	} else {
		DEBUG(F("Configuration Sent Successfully"));
	}
}

void Device::sendRequest_forConfiguration() {
	CCanID canID;
	canID.setMacID(eepromConf.getMacAddress());
	canID.setFlag_forConfiguration();

	DEBUG(F("Send request for configuration, canID:") << eepromConf.getMacAddress());
	byte data;
	byte sndStat = s_can.sendMsgBuf(canID._canID, 0, &data);
	if (sndStat != CAN_OK) {
		DEBUG(F("Error Sending Configuration"));
	} else {
		DEBUG(F("Configuration Sent Successfully"));
	}
}

void Device::sendRequest_fromSwitch(uint8_t gpio, byte pinValue) {
	//* send message
	CCanID canID;
	canID.setMacID(s_conf.getMacAddress());
	canID.setFlag_fromSwitch();
	CTrafficDataSwitch dataSwitch(gpio, pinValue);
	MsgData data;
	dataSwitch.serialize(data);
	DEBUG("data:" << PRINT_DATA(data));
	byte sndStat = s_can.sendMsgBuf(canID._canID, dataSwitch.getSize(), data);
	if (sndStat != CAN_OK) {
		Serial << F("Error Sending Message\n");
	} else {
		Serial << F("Message Sent Successfully\n");
	}
}