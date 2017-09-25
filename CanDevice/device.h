#pragma once

#include <avr/wdt.h>
#include <Arduino.h>

#include <SPI.h>
#include <mcp_can.h>
#include <smartHouse.h>
//#include <data.h>
#include <Streaming.h>

#include "configuration.h"
#include "arrivedConfiguration.h"
#include "eepromConf.h"
#include <SimpleFIFO.h>

#define CAN0_INT 2   // Set INT to pin 2

#define CRITICAL_SECTION_START cli(); {
#define CRITICAL_SECTION_END } sei();

//* speed must be detected
#define CANBUS__DETECT_SPEED 255

struct ST_CANBUS_RECEIVED_DATA {
	unsigned long canID;
	MsgData rxData;

	ST_CANBUS_RECEIVED_DATA() {
		canID = 0;
		opravit
		//rxData = s.rxData;
	}

	ST_CANBUS_RECEIVED_DATA(volatile const ST_CANBUS_RECEIVED_DATA & s) {
		canID = s.canID;
		opravit
		//rxData = s.rxData;
	}

	ST_CANBUS_RECEIVED_DATA & operator= (const ST_CANBUS_RECEIVED_DATA & s) {
		canID = s.canID;
		opravit
		//rxData = s.rxData;
		return *this;
	}
};

class Device {
private:
	uint32_t _counterOfProcessed;
	void processReceivedCanBusData();
	static SimpleFIFO<ST_CANBUS_RECEIVED_DATA, 10> s_bufferOfReceivedCanBusData;

	//volatile static bool s_newModifiedIsSet;
	//static INT8U sendMsgBuf(INT32U id, INT8U ext, INT8U len, INT8U *buf);
	static MCP_CAN s_can;     // Set CS to pin 10 in constructor
	static ArrivedConfiguration * s_arrivedConf;
	//byte _data[8] = {0};
	//static uint16_t s_deviceAddress;
	//EepromConf & _eepromConf;
	static Configuration s_conf;

	//static bool s_firstConfMessage;
	//static byte s_numberOfMsgFromConf;
	//static byte s_numberOfArrivedMsg;

	static void interruptFromCanBus();
	static uint8_t sendMsg(CDataBase & cdb);
	//void sendRequest_forConfiguration();
	//void sendRequest_askSwitchForValue(MacID macId, uint8_t pin);
	//void static sendRequest_fromSwitch(byte gpio, byte pinValue);
	
	static Device * s_instance;
	static Device * getInstance() {
		return s_instance;
	}

	void iniCanBus(uint8_t canBusSpeed);
	void detectCanBusSpeed();
public:
	Device();
	~Device();

	void init();
	void update();
	void setPins();
	void checkModifiedData(CDataBase * pConfData, byte index);
	void checkValueOnPins(CDataBase * pConfData, byte index);
	static void doReset(uint16_t upperBoundOfRandomTime = 0);
};

extern EepromConf eepromConf;