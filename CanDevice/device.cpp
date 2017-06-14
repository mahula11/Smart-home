
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

	DEBUG("1------------: " << eepromConf.getCountOfConf());

	//* skontroluje, ci mame konfiguracne spravy. pokial nie, tak treba poziadat o konfiguraciu
	if (eepromConf.getCountOfConf()) {
		//* nacitaj conf
		DEBUG("2------------: " << eepromConf.getCountOfConf());
		s_conf.setConfiguration(eepromConf.readConf());
		DEBUG("21------------");
		setPinModes();
		DEBUG("22------------");
	} else {
		DEBUG("3------------");
		//DEBUG(F("Send request for configuration") << endl);
		//* pocet je 0, takze ziadnu konfiguraciu v eeprom nemame, treba poziadat o novu.
		sendRequestForConfiguration();
	}
}

void Device::setPinModes() {
	for (int i = 0; i < s_conf.getCount(); i++) {
		switch (s_conf.getConf(i)->getType()) {
			case DEVICE_TYPE_LIGHT:
			case DEVICE_TYPE_LIGHT_WITH_DIMMER:
				pinMode(((CConfDataLight*)s_conf.getConf(i))->_gpio, OUTPUT);
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
				DEBUG("_value:" << s_conf.getConfValue(index)._value);
				//* send message
				CCanID canID;
				canID.setMacID(s_conf.getMacAddress());
				canID.setFlag_fromSwitch();
				CTrafficDataSwitch dataSwitch(((CConfDataSwitch*)pConfData)->_gpio, pinValue);
				MsgData data;
				dataSwitch.serialize(data);
				DEBUG("data:" << PRINT_DATA(data));
				byte sndStat = s_can.sendMsgBuf(canID._canID, dataSwitch.getSize(), data);
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
		DEBUG("Ne conf will be procesed");
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

	DEBUG("Received CanID:" << canId._canID << ",MacID:" << canId.getMacID()
		<< ",fromConf:" << canId.hasFlag_fromConfiguration()
		<< ",fromSwitch:" << canId.hasFlag_fromSwitch());

	if (canId.hasFlag_fromConfiguration() && canId.getMacID() == eepromConf.getMacAddress()) {
		//* messages from configuration server
		if (s_arrivedConf == nullptr) {
			s_arrivedConf = new ArrivedConfiguration();
		}
		//* ked pride prva konfiguracna sprava, tak v datach, v prvom byte mame pocet sprav, ktore este pridu
		//* getCount vrati nulu, pretoze este neviemme pocet sprav
		if (s_arrivedConf->getCount()) {
			byte type = rxBuf[0];
			DEBUG("Conf arrived for:" << type);
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
			DEBUG("Number of confs arrived:" << rxBuf[0]);
			s_arrivedConf->setCount(rxBuf[0]);
		}
	} else if (canId.hasFlag_fromSwitch()) { //* message from switch to lights
		DEBUG("Messsage from switch arrived");
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
	}
}

void Device::sendRequestForConfiguration() {
	CCanID canID;
	canID.setMacID(eepromConf.getMacAddress());
	canID.setFlag_forConfiguration();

	uint16_t mac = canID._canID & 65535;

	byte data;
	DEBUG(F("Send request for configuration, canID:") << mac);
	byte sndStat = s_can.sendMsgBuf(canID._canID, 0, &data);
	if (sndStat != CAN_OK) {
		DEBUG(F("Error Sending Configuration"));
	} else {
		DEBUG(F("Configuration Sent Successfully"));
		//Serial.println(id);
	}
}