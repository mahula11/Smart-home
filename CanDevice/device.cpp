
#include <Arduino.h>

#include "device.h"

Device * Device::s_instance = nullptr;
//ArrivedConfiguration Device::s_arrivedConf;
MCP_CAN Device::s_can(10);
Configuration Device::s_conf;
ArrivedConfiguration * Device::s_arrivedConf = nullptr;
//volatile bool Device::s_newModifiedIsSet = false;
//uint16_t Device::s_deviceAddress = 0;
//bool Device::s_firstConfMessage = true;
//byte Device::s_numberOfMsgFromConf = 0;
//byte Device::s_numberOfArrivedMsg = 0;

Device::Device() {
	s_instance = this;
}


Device::~Device() {
}

void Device::iniCanBus(uint8_t canBusSpeed) {
	// Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s 
	// and the masks and filters disabled.
	if (s_can.begin(MCP_ANY, canBusSpeed, MCP_8MHZ) == CAN_OK) {
		DEBUG(F("MCP2515 Initialized Successfully:") << canBusSpeeds[canBusSpeed]);
	} else {
		DEBUG(F("MCP2515 Initializing Error"));
	}
	// Change to normal mode to allow messages to be transmitted
	s_can.setMode(MCP_NORMAL);
}

void Device::init() {
	//_eepromConf = eepromConf;
	s_arrivedConf = nullptr;
	//s_deviceAddress = _eepromConf.getMacAddress();
	
	//* inicialize canBus
	iniCanBus(CAN_1000KBPS);

	pinMode(CAN0_INT, INPUT);   // Configuring pin for /INT input

	attachInterrupt(digitalPinToInterrupt(CAN0_INT), interruptFromCanBus, FALLING);

	s_conf.setConfigurationStatic(eepromConf.getCountOfConf(), eepromConf.getWatchdogTimeout(), eepromConf.getAutoResetTime());

	//* skontroluje, ci mame konfiguracne spravy. pokial nie, tak treba poziadat o konfiguraciu
	if (eepromConf.getCountOfConf()) {
		//* nacitaj conf
		DEBUG(F("Set conf from eeprom."));
		s_conf.setConfiguration(eepromConf.readConf());
		
		s_conf.getMacAddress() vracia nulu, zistit preco???
		CTraficMsg_ImUp imUp((uint16_t)s_conf.getMacAddress());
		sendMsg(imUp);


		setPins();
	} else {
		//* pocet je 0, takze ziadnu konfiguraciu v eeprom nemame, treba poziadat o novu.
		//sendRequest_forConfiguration();
		DEBUG(F("Device will ask for conf."));
		CConfMsg_askForConfiguration afc(eepromConf.getMacAddress());
		sendMsg(afc);
	}
	DEBUG(F("Init is complet!"));
}

void Device::setPins() {
	for (int i = 0; i < s_conf.getCount(); i++) {
		switch (s_conf.getConf(i)->getType()) {
			case DEVICE_TYPE_LIGHT: {
				pinMode(((CConfMsg_light*)s_conf.getConf(i))->_gpio, OUTPUT);
				CTrafficMsg_askSwitchForData asfd(((CConfMsg_light*)s_conf.getConf(i))->_switchMacID, ((CConfMsg_light*)s_conf.getConf(i))->_switchGPIO);
				//sendRequest_askSwitchForValue(((CConfMsg_light*)s_conf.getConf(i))->_switchMacID, ((CConfMsg_light*)s_conf.getConf(i))->_switchGPIO);
				sendMsg(asfd);
			}
				break;
			//case DEVICE_TYPE_SOCKET:
			//	//pinMode(CanExt::getLightGPIO(pData), OUTPUT);
			//	break;
			
			//case DEVICE_TYPE_PUSH_BUTTON:
			//case DEVICE_TYPE_STAIR_CASE_SWITCH:
			//	break;
			case DEVICE_TYPE_SWITCH:
				pinMode(((CConfMsg_switch*)s_conf.getConf(i))->_gpio, INPUT);
				break;
		}
	}
}

void Device::checkModifiedData(CDataBase * pConfData, byte index) {
	//* tato modifikacia je nastavena vtedy, ked pridu nove data cez CanBus 
	//* napr prepnutie vypinaca vysle spravu s novou hodnotou pre ziarovky, ktore pocuvaju pre dany vypinac
	if (s_conf.getConfValue(index)->_modified) {
		switch (pConfData->getType()) {
			case DEVICE_TYPE_LIGHT:
				DEBUG("Set light on PIN:" << ((CConfMsg_light*)pConfData)->_gpio << " value:" << s_conf.getConfValue(index)->_value);
				digitalWrite(((CConfMsg_light*)pConfData)->_gpio, s_conf.getConfValue(index)->_value);
				break;
			//case DEVICE_TYPE_LIGHT_WITH_DIMMER:
			//	break;
		}
		s_conf.getConfValue(index)->_modified = false;
	}
}

void Device::checkValueOnPins(CDataBase * pConfData, byte index) {
	switch (pConfData->getType()) {
		//case DEVICE_TYPE_PUSH_BUTTON:
		//case DEVICE_TYPE_STAIR_CASE_SWITCH:
		//	break;
		case DEVICE_TYPE_SWITCH:
		{
			byte pinValue = digitalRead(((CConfMsg_switch*)pConfData)->_gpio);			
			//* if values are different, then send message to the lights
			if (pinValue != s_conf.getConfValue(index)->_value) {
				DEBUG(VAR(pinValue));
				DEBUG(F("_value:") << s_conf.getConfValue(index)->_value);

				//* set value without modify flag
				s_conf.setConfValue(index, pinValue, false);				

				//sendRequest_fromSwitch(((CConfMsg_switch*)pConfData)->_gpio, pinValue);
				CTrafficMsg_fromSwitch msgFromSwitch(eepromConf.getMacAddress(), ((CConfMsg_switch*)pConfData)->_gpio, pinValue);
				sendMsg(msgFromSwitch);			
			}
		}
		break;
	}
}

void Device::update() {
	//* vynuteny restart
	//* pokial je millis() mensi ako nastaveny cas (4 hodiny), dovtedy sa watchdog bude resetovat. 
	//* pokial tento cas presiahne, tak nedovolime reset watchdogu a tym bude vynuteny reset procesoru
	if (s_conf.getAutoResetTime() == 0 || millis() < s_conf.getAutoResetTime()) {
		//* resetuje watchdog, zabrani restartu
		wdt_reset();
	}
		//* test watchdog timeouts, set millis in delay higher then watchdog timer, processor will be reseting around
		//DEBUG("po wdt_resete");
		//delay(2500);
	//* s_arrivedConf is read in interruptFromCanBus
	//* if arrived configuration is complet, then copy it to eeprom
	if (s_arrivedConf && s_arrivedConf->isComplet()) {		
		DEBUG(F("New conf will be processed"));
		//* zapiseme do eeprom
		eepromConf.writeConf(s_arrivedConf->getCount(), s_arrivedConf->getConf());
		DEBUG(F("Processor reset"));
		//* do reset and do not have to clean and set up configuration
		doReset();
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

	if (Serial.available()) {
		int incomingByte = Serial.read();
		Serial.print(F("I received: "));
		Serial.println(incomingByte, DEC);
		switch (incomingByte) {
			case 'r': {
				//* set to zero conter of configurations
				//* this enforce new conf
				EEPROM.writeByte(EEPROM_ADDRESS__CONF_COUNT, 0);
				//* do reset
				doReset();
				break;
			}
			case 'g': {
				//* inicialize canBus
				iniCanBus(CAN_500KBPS);
				break;
			}
			case 'h': {
				//* inicialize canBus
				iniCanBus(CAN_100KBPS);
				break;
			}
			case 'j': {
				//* inicialize canBus
				iniCanBus(CAN_1000KBPS);
				break;
			}
		}
	}
}

unsigned long gCounter = 0;

void Device::doReset(uint16_t upperBoundOfRandomTime) {
	if (upperBoundOfRandomTime) {
		long rand = random(0, upperBoundOfRandomTime);
		DEBUG(F("Reset will be do in ") << rand << F("ms"));
		delay(rand);
	}
	wdt_enable(WDTO_15MS);
	delay(1000);
}

void Device::interruptFromCanBus() {
	s_can.setMode(MCP_NORMAL);
	//Device * instance = getInstance();
	unsigned long counter = gCounter++;
	DEBUG(F("-----------------------") << endl << F("InterruptFromCanBusStart:") << counter << F(",milis:") << millis());
	CanID canId;
	byte len = 0;
	MsgData rxBuf;
	uint8_t readMsgStat;
	while ((readMsgStat = s_can.readMsgBuf(&canId._canID, &len, rxBuf)) == CAN_OK) {
		DEBUG(F("Received:\n CanID:") << canId._canID << F(",MacID:") << canId.getMacID()
			<< F(",\n fromConf:") << canId.hasFlag_fromConfiguration()
			<< F(",\n fromSwitch:") << canId.hasFlag_fromSwitch()
			<< F(",\n askSwitchForVal:") << canId.hasFlag_askSwitchForValue()
			<< F(",\n ping:") << canId.hasFlag_ping() 
			<< F(",\n ImUp:") << canId.hasFlag_ImUp());

		if (canId.hasFlag_fromConfiguration() && canId.getMacID() == eepromConf.getMacAddress()) {
			//* messages from configuration server
			if (s_arrivedConf == nullptr) {
				s_arrivedConf = new ArrivedConfiguration();
			}

			//* sprava moze prist z FE, bez vyziadania
			//* nastavime timeout pre watchdog
			if (canId.hasFlag_fromConfSetWatchdog()) {
				DEBUG(F("Set watchdog message, val:") << rxBuf[0]);
				eepromConf.setWatchdogTimeout((WATCHDOG_TIMEOUT)rxBuf[0]);
				continue;
			}

			//* received reset from conf
			//* disable wdt_reset
			if (canId.hasFlag_fromConfReset()) {
				DEBUG(F("Reset message!"));
				doReset();
			}

			if (canId.hasFlag_fromConfAutoResetTime()) {
				DEBUG(F("AutoReset message, val:") << rxBuf[0]);
				eepromConf.setAutoResetTime(rxBuf[0]);
				s_conf.setAutoResetTime(rxBuf[0]);
				continue;
			}

			//* ked pride prva konfiguracna sprava, tak v datach, v prvom byte mame pocet sprav, ktore este pridu
			//* getCount vrati nulu, pretoze este neviemme pocet sprav
			if (s_arrivedConf->getCount()) {
				byte type = canId.getConfigPart();
				DEBUG(F("Conf arrived for type:") << type);
				CDataBase * pConfData;
				switch (type) {
					case DEVICE_TYPE_SWITCH:
						pConfData = new CConfMsg_switch(rxBuf);
						break;
					case DEVICE_TYPE_LIGHT:
						pConfData = new CConfMsg_light(rxBuf);
						break;
				}
				s_arrivedConf->addConf(pConfData);
			} else {
				//* prisla prva sprava, prislo cislo, ktore je pocet sprav, ktore este pridu z CanConf
				DEBUG(F("Number of confs will arrive:") << rxBuf[0]);
				s_arrivedConf->setCount(rxBuf[0]);
			}
		} else if (canId.hasFlag_fromSwitch()) { //* message from switch to lights
			DEBUG(F("Messsage from switch arrived"));
			//* teraz skontrolovat ci ID vypinaca patri niektoremu vypinacu v nasej konfiguracii (pre niektoru ziarovku)
			CTrafficMsg_fromSwitch switchData(rxBuf);
			for (byte i = 0; i < s_conf.getCount(); i++) {
				//* vyhladavame len typ "ziarovky" a potom ich IDcka vypinacov
				//* 0 - typ (ziarovka), 1 - gpio, 2 - id vypinaca
				CDataBase * pData = s_conf.getConf(i);
				if (pData->getType() == DEVICE_TYPE_LIGHT && 
					((CConfMsg_light*)pData)->_switchMacID == canId.getMacID() && 
					((CConfMsg_light*)pData)->_switchGPIO == switchData._gpio) 
				{
					//s_newModifiedIsSet = true;
					s_conf.setConfValue(i, switchData._value, true);
					DEBUG(F("Nastavit hodnotu:") << switchData._value << F(" pre switch z MacID:") << canId.getMacID() 
						<< F(" a gpio:") << switchData._gpio);
				}
			}
		} else if (canId.hasFlag_askSwitchForValue() && canId.getMacID() == eepromConf.getMacAddress()) {
			DEBUG(F("Message from light, asking switch for value"));

			byte gpio = rxBuf[0];
			byte pinValue = digitalRead(gpio);
			//sendRequest_fromSwitch(gpio, pinValue);
			CTrafficMsg_fromSwitch msgFromSwitch(eepromConf.getMacAddress(), gpio, pinValue);
			sendMsg(msgFromSwitch);

		}
	}
	DEBUG(F("InterruptFromCanBusEnd:") << counter 
		<< F(",milis:") << millis() 
		<< F(",") << VAR(readMsgStat) << endl 
		<< F("-----------------------"));
}

//void Device::interruptFromCanBus() {
//	long counter = gCounter++;
//	DEBUG(F("Start interruptFromCanBus:") << counter << F(",milis:") << millis());
//	//Device * instance = getInstance();
//	CanID canId;
//	byte len = 0;
//	MsgData rxBuf;
//	//while(s_can.readMsgBuf(&canId._canID, &len, rxBuf) == CAN_OK)
//	s_can.readMsgBuf(&canId._canID, &len, rxBuf);
//	{
//		DEBUG(F("Received CanID:") << canId._canID << F(",MacID:") << canId.getMacID()
//			<< F(",fromConf:") << canId.hasFlag_fromConfiguration()
//			<< F(",fromSwitch:") << canId.hasFlag_fromSwitch()
//			<< F(",askSwitchForVal:") << canId.hasFlag_askSwitchForValue());
//
//		if (canId.hasFlag_fromConfiguration() && canId.getMacID() == eepromConf.getMacAddress()) {
//			//* messages from configuration server
//			if (s_arrivedConf == nullptr) {
//				s_arrivedConf = new ArrivedConfiguration();
//			}
//
//			//* sprava moze prist z FE, bez vyziadania
//			//* nastavime timeout pre watchdog
//			if (canId.hasFlag_fromConfSetWatchdog()) {
//				eepromConf.setWatchdogTimeout((WATCHDOG_TIMEOUT)rxBuf[0]);
//				DEBUG(F("Watchdog set: ") << (WATCHDOG_TIMEOUT)rxBuf[0] << F("ms"));
//				//continue;
//				return;
//			}
//
//			if (canId.hasFlag_fromConfNumber()) {
//				//* prisla prva sprava, prislo cislo, ktore je pocet sprav, ktore este pridu z CanConf
//				DEBUG(F("Number of confs arrived:") << rxBuf[0]);
//				s_arrivedConf->setCount(rxBuf[0]);
//				//continue;
//				return;
//			}
//
//			//* ked pride prva konfiguracna sprava, tak v datach, v prvom byte mame pocet sprav, ktore este pridu
//			//* getCount vrati nulu, pretoze este neviemme pocet sprav, cize pocet musi prist prvy
//			if (s_arrivedConf->getCount()) {
//				byte type = canId.getConfigPart();
//				DEBUG(F("Conf arrived for:") << type);
//				CDataBase * pConfData;
//				switch (type) {
//					case DEVICE_TYPE_SWITCH:
//						pConfData = new CConfMsg_switch(rxBuf);
//						break;
//					case DEVICE_TYPE_LIGHT:
//						pConfData = new CConfMsg_light(rxBuf);
//						break;
//					default:
//						DEBUG(F("UNSUPPORTED DEVICE!!!"));
//						break;
//				}
//				s_arrivedConf->addConf(pConfData);
//			}
//		} else if (canId.hasFlag_fromSwitch()) { //* message from switch to lights
//			DEBUG(F("Messsage from switch arrived"));
//			//* teraz skontrolovat ci ID vypinaca patri niektoremu vypinacu v nasej konfiguracii (pre niektoru ziarovku)
//			CTrafficDataSwitch switchData(rxBuf);
//			for (byte i = 0; i < s_conf.getCount(); i++) {
//				//* vyhladavame len typ "ziarovky" a potom ich IDcka vypinacov
//				//* 0 - typ (ziarovka), 1 - gpio, 2 - id vypinaca
//				CDataBase * pData = s_conf.getConf(i);
//				if (pData->getType() == DEVICE_TYPE_LIGHT && ((CConfMsg_light*)pData)->_switchMacID == canId.getMacID() && ((CConfMsg_light*)pData)->_switchGPIO == switchData._gpio) {
//					s_conf.setConfValue(i, switchData._value, true);
//				}
//			}
//		} else if (canId.hasFlag_askSwitchForValue() && canId.getMacID() == eepromConf.getMacAddress()) {
//			DEBUG(F("Message from light, asking switch for value"));
//
//			byte gpio = rxBuf[0];
//			byte pinValue = digitalRead(gpio);
//			sendRequest_fromSwitch(gpio, pinValue);
//		}
//	}
//	DEBUG(F("End interruptFromCanBus:") << counter << F(",milis:") << millis());
//}

INT8U Device::sendMsgBuf(INT32U id, INT8U ext, INT8U len, INT8U *buf) {
	INT8U res;

	s_can.setMsg(id, 0, ext, len, buf);
	res = s_can.sendMsg();

	return res;
}

void Device::sendMsg(CDataBase & cdb) {
	byte data[8];
	cdb.serialize(data);
	uint8_t ret = 0;
	static uint32_t counter = 0;
	
	s_can.setMode(MCP_NORMAL);
	DEBUG(F("Sending msg to CanBus:\n CanID:") << cdb._destCanID._canID
		<< F(",MacID:") << cdb._destCanID.getMacID()
		<< F(",\n deviceType:") << cdb.getType());
	//ret = s_can.sendMsgBuf(cdb._destCanID._canID, 1, cdb.getSize(), data);
	ret = sendMsgBuf(cdb._destCanID._canID, 1, cdb.getSize(), data);
#ifdef DEBUG_BUILD
	if (ret == CAN_OK) {
		DEBUG(F("Msg was sent to CanBus:") << ++counter);
	} else {
		DEBUG(F("Failure when send to CanBus:\n CanID:") << cdb._destCanID._canID << F(",error:") << ret);
	}
#endif


//	if (1) {
//		s_can.setMode(MCP_LOOPBACK);
//		//s_can.setMode(MCP_NORMAL);
//		DEBUG(F("Sending msg to loopback:\n CanID:") << cdb._destCanID._canID
//			<< F(",MacID:") << cdb._destCanID.getMacID()
//			<< F(",\n deviceType:") << cdb.getType());
//		DEBUG(F("---------------------send1"));
//		//ret = s_can.sendMsgBuf(cdb._destCanID._canID, cdb.getSize(), data);
//		ret = sendMsgBuf(cdb._destCanID._canID, 1, cdb.getSize(), data);
//		DEBUG(F("---------------------send2"));
//#ifdef DEBUG_BUILD
//		if (ret == CAN_OK) {
//			DEBUG(F("Msg was sent to loopback"));
//		} else {
//			DEBUG(F("Failure when send over loopback:\n CanID:") << cdb._destCanID._canID << F(",error:") << ret);
//		}
//#endif
//		//DEBUG(F("Set back normal mode and send again message to CanBus"));
//		//s_can.setMode(MCP_NORMAL);
//	}
//	s_can.setMode(MCP_NORMAL);
}


//void Device::sendRequest_askSwitchForValue(MacID macId, uint8_t pin) {
//	DEBUG(F("Ask switch for value, macId:") << macId << F(", pin ") << pin);
//	CanID canID;
//	canID.setMacID(macId);
//	canID.setFlag_askSwitchForValue();
//	byte data = pin;
//	byte sndStat = s_can.sendMsgBuf(canID._canID, 1, &data);
//	if (sndStat != CAN_OK) {
//		DEBUG(F("Error Sending Configuration"));
//	} else {
//		DEBUG(F("Configuration Sent Successfully"));
//	}
//}

//void Device::sendRequest_forConfiguration() {
//	CanID canID;
//	canID.setMacID(eepromConf.getMacAddress());
//	canID.setFlag_forConfiguration();
//
//	DEBUG(F("Send request for configuration, canID:") << eepromConf.getMacAddress());
//	byte data;
//	byte sndStat = s_can.sendMsgBuf(canID._canID, 0, &data);
//	if (sndStat != CAN_OK) {
//		DEBUG(F("Error Sending Configuration"));
//	} else {
//		DEBUG(F("Configuration Sent Successfully"));
//	}
//}
//
//void Device::sendRequest_fromSwitch(uint8_t gpio, byte pinValue) {
//	//* send message
//	CanID canID;
//	canID.setMacID(eepromConf.getMacAddress());
//	canID.setFlag_fromSwitch();
//	CTrafficMsg_fromSwitch dataSwitch(eepromConf.getMacAddress(), gpio, pinValue);
//	MsgData data = {0};
//	dataSwitch.serialize(data);
//	DEBUG("data:" << PRINT_DATA(data));
//	byte sndStat = s_can.sendMsgBuf(canID._canID, dataSwitch.getSize(), data);
//	if (sndStat != CAN_OK) {
//		Serial << F("Error Sending Message\n");
//	} else {
//		Serial << F("Message Sent Successfully\n");
//	}
//}