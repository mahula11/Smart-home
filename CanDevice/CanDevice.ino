/*
 Name:		CanDevice.ino
 Created:	4/3/2017 8:57:36 PM
 Author:	Marek
*/


//#include <EEPROMVar.h>
#include <QueueArray.h>
#include <Streaming.h>
#include <EEPROMex.h>
#include <SimpleFIFO.h>
#include <smartHouse.h>
#include <mcp_can.h>
#include <SPI.h>

#include "arrivedConfiguration.h"
//#define WRITE_MAC_ADDRESS

#ifdef WRITE_MAC_ADDRESS

void setup() {
	//* write unique MAC address to EEPROM
	uint16_t address = 1;
	EEPROM.writeInt(0, address);
}

#else

#define CAN0_INT 2   // Set INT to pin 2
MCP_CAN CAN0(10);     // Set CS to pin 10
ArrivedConfiguration arrivedConf;

byte data[8] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };

//enum DEVICE_TYPE { switchButton, pushButton, stairCaseSwitch, light, lightWithDimmer, socket };
//enum ROUTABLE_MESSAGES { routable, noRoutable };
//struct CONF_MESSAGE {
//	INT32U macID;	//* Identifikator z CanBus zariadenia (z EEPROM)
//	INT8U confData[8];
//	//DEVICE_TYPE deviceType;	//* urcenie zariadenia vzhladom na GPIO pin
//	//INT8U gpio;		//* pin, ktory je pouzity (pri ziarovke/zasuvke ako vystupny, pri vypinaci ako vstupny, podla deviceType)
//	//INT32U canID;			//* ID spravy, ktore bude poslane pri udalosti. ked to bude vypinac, tak bude poslana sprava s tymto ID a ziarovky/zasuvky to budu odchytavat
//	//ROUTABLE_MESSAGES routable;	//* urci, ci sprava bude moct byt presmerovana do inych segmentov siete
//};

//CONF_MESSAGE gConfMessages[25] = { 0 };

//volatile SimpleFIFO<CONF_MESSAGE, 25> gFifoConfMessages;
//volatile QueueArray <CONF_MESSAGE> g_fifoConfMessages;

void sendRequestForConfiguration();
void interruptFromCanBus();
void printStruct();

void setup() {
	Serial.begin(115200);

	uint16_t address = EEPROM.readInt(0);
	if (address == 0) {		
		while (1) {
			Serial.println("No MAC address given, please configure address!!!");
			delay(1000);
		}
	}

	// Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
	if (CAN0.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) == CAN_OK) 
		Serial.println("MCP2515 Initialized Successfully!");
	else 
		Serial.println("Error Initializing MCP2515...");

	CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
	pinMode(CAN0_INT, INPUT);                            // Configuring pin for /INT input

	attachInterrupt(digitalPinToInterrupt(CAN0_INT), interruptFromCanBus, FALLING);

	sendRequestForConfiguration();
}

void interruptFromCanBus() {
	//Serial.println("Interrupt");
	
	INT32U rxId;
	unsigned char len = 0;
	unsigned char rxBuf[8];
	CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
	

	if (CanExt::isMessageFromCanConf(rxId)) {		
		static bool firstConfMessage = true;
		static uint8_t numberOfMessages = 0;
		static uint8_t count = 0;
		
		//* v prvej konfiguracnej sprave pride pocet kofiguracnych sprav, ktore pridu
		INT32U macID = CanExt::getNormalizedID(rxId);
		//* ked pride prva konfiguracna sprava, tak v datach, v prvom byte mame pocet sprav, ktore este pridu
		if (firstConfMessage && (len == 1)) {
			firstConfMessage = false;
			numberOfMessages = rxBuf[0];
			arrivedConf.setCount(numberOfMessages);
			//g_fifoConfMessages.resize(numberOfMessages);
		}
		
		//* ked prijmeme vsetky spravy, tak resetneme premenne
		if (numberOfMessages == count) {
			firstConfMessage = true;
			numberOfMessages = 0;
			count = 0;
		}
		count++;
		//Serial.print("Konfiguracia pre: ");
		//Serial.println(macID);
		CONF_MESSAGE msg(macID, len, rxBuf[0]);
		//g_fifoConfMessages.push(msg);
		//gFifoConfMessages.enqueue(msg);
	}
}

//byte counter = 0;

void loop() {
	
	if (Serial.available()) {
		int incomingByte = Serial.read();
		Serial.print(F("I received: "));
		Serial.println(incomingByte, DEC);
		if (incomingByte == 'r') {
			sendRequestForConfiguration();
		} else if(incomingByte == 'a') {
			printStruct();
		}
	}
	
	//if (gFifoConfMessages.count() > 0) {
		//* zapis do eeprom
	//}

	delay(10);   // send data per 100ms
}

void sendRequestForConfiguration() {
	INT32U id;
	id = 234;
	bitSet(id, 31);				//* set extended message
	bitSet(id, 30);				//* set remote flag
	bitSet(id, 28);				//* set bit for request of configuration
	byte sndStat = CAN0.sendMsgBuf(id, 0, data);
	if (sndStat != CAN_OK) {
		Serial.println("Error Sending Configuration!");
	} else {
		Serial.println("Configuration Sent Successfully:");
		//Serial.println(id);
	}
}

void printStruct() {
	Serial.println();
	char msgString[128];
	for (int i = 0; i < 25; i++) {
		//if (gConfMessages[i].macID != 0) {
		//	sprintf(msgString, "MacID:%ld, deviceType:%d, pin:%d, canID:%d, routable:%d", gConfMessages[i].macID,
		//		gConfMessages[i].confData[0],
		//		gConfMessages[i].confData[1],
		//		gConfMessages[i].confData[2],
		//		gConfMessages[i].confData[3]);
		//	Serial.println(msgString);
		//}
	}
}

#endif