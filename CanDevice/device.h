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

#define CRITICAL_SECTION_START	\
	cli();						\
	{

#define CRITICAL_SECTION_END				\
	}										\
	if (s_isDisableInterrupts == false) {	\
		sei();								\
	}
	
#define DISABLE_INTERRUPTS			\
	cli();							\
	s_isDisableInterrupts = true; 

#define ENABLE_INTERRUPTS			\
	s_isDisableInterrupts = false;	\
	sei();


#define CAN_BUS__BUFFER_SIZE 10

//* speed must be detected
#define CANBUS__DETECT_SPEED 255

struct ST_CANBUS_RECEIVED_DATA {
	unsigned long _canID;
	MsgData rxData;

	ST_CANBUS_RECEIVED_DATA() {
		_canID = 0;
		memset(rxData, 0, sizeof(uint8_t) * 8);
	}

	ST_CANBUS_RECEIVED_DATA(uint32_t canID, MsgData &data) {
		_canID = canID;
		memcpy(rxData, data, sizeof(uint8_t) * 8);
	}

	ST_CANBUS_RECEIVED_DATA(const ST_CANBUS_RECEIVED_DATA & s) {
		_canID = s._canID;
		memcpy(rxData, s.rxData, sizeof(uint8_t) * 8);
	}

	ST_CANBUS_RECEIVED_DATA(const volatile ST_CANBUS_RECEIVED_DATA & s) {
		_canID = s._canID;
		memcpy(rxData, (const void *)s.rxData, sizeof(uint8_t) * 8);
	}

	ST_CANBUS_RECEIVED_DATA & operator= (const ST_CANBUS_RECEIVED_DATA & s) {
		_canID = s._canID;
		memcpy(rxData, s.rxData, sizeof(uint8_t) * 8);
		return *this;
	}

	//ST_CANBUS_RECEIVED_DATA & operator= (volatile const ST_CANBUS_RECEIVED_DATA & s) {
	//	canID = s.canID;
	//	//opravit
	//	//rxData = s.rxData;
	//	return *this;
	//}
};

class Device {
private:
	uint32_t _counterOfProcessed;
	void processReceivedCanBusData();
	static SimpleFIFO<ST_CANBUS_RECEIVED_DATA, CAN_BUS__BUFFER_SIZE> s_bufferOfReceivedCanBusData;
	volatile static bool s_isDisableInterrupts;

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
	static void readCanBusData();
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