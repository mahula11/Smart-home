#pragma once

#include <Arduino.h>

#include <SPI.h>
#include <mcp_can.h>
#include <smartHouse.h>

#include "arrivedConfiguration.h"
#include "eepromConf.h"

#define CAN0_INT 2   // Set INT to pin 2


class Device {
private:
	static MCP_CAN s_can;     // Set CS to pin 10 in constructor
	static ArrivedConfiguration s_arrivedConf;
	byte _data[8] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	static uint16_t s_deviceAddress;
	EepromConf & _eepromConf;

	//static bool s_firstConfMessage;
	//static byte s_numberOfMsgFromConf;
	//static byte s_numberOfArrivedMsg;

	static void interruptFromCanBus();
	void sendRequestForConfiguration();
	
	static Device * s_instance;
	static Device * getInstance() {
		return s_instance;
	}
public:
	Device();
	~Device();

	void init(EepromConf & eepromConf);
	void update();
};

