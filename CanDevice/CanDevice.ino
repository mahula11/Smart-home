/*
 Name:		CanDevice.ino
 Created:	4/3/2017 8:57:36 PM
 Author:	Marek
*/


#include <SimpleFIFO.h>
#include <smartHouse.h>
#include <mcp_can.h>
#include <SPI.h>

#define CAN0_INT 2   // Set INT to pin 2
MCP_CAN CAN0(10);     // Set CS to pin 10

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

volatile SimpleFIFO<CONF_MESSAGE, 25> gFifoConfMessages;

void setup() {
	Serial.begin(115200);

	// Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
	if (CAN0.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) == CAN_OK) 
		Serial.println("MCP2515 Initialized Successfully!");
	else 
		Serial.println("Error Initializing MCP2515...");

	CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
	pinMode(CAN0_INT, INPUT);                            // Configuring pin for /INT input

	attachInterrupt(digitalPinToInterrupt(CAN0_INT), interruptFromCanBus, FALLING);

	sendRequestForConf();
}

void interruptFromCanBus() {
	//Serial.println("Interrupt");
	
	INT32U rxId;
	unsigned char len = 0;
	unsigned char rxBuf[8];
	CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)

	if (CanExt::isConfMes(rxId)) {
		//* prislo nastavenie pre konfiguraciu
		INT32U macID = CanExt::getNormalizedID(rxId);
		//Serial.print("Konfiguracia pre: ");
		Serial.println(macID);
		CONF_MESSAGE msg(macID, len, rxBuf[0]);
		gFifoConfMessages.enqueue(msg);

		//for (int i = 0; i < 25; i++) {
		//	if (gConfMessages[i].macID == 0) {
		//		gConfMessages[i].macID = macID;
		//		for (int ii = 0; ii < len; ii++) {
		//			gConfMessages[i].confData[ii] = rxBuf[ii];
		//			//sprintf(msgString, "buf:%d", rxBuf[ii]);
		//			//Serial.print(msgString);
		//		}
		//		break;
		//	}
		//}
	}
}

byte counter = 0;

void sendRequestForConf() {
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

bool printData = false;
bool wasRead = false;

void loop() {
	//data[7] = counter;
	// send data:  ID = 0x100, Standard CAN Frame, Data length = 8 bytes, 'data' = array of data bytes to send
	//* byte sndStat = CAN0.sendMsgBuf(268435456, 1, 8, data);

	if (Serial.available()) {
		int incomingByte = Serial.read();
		Serial.print(F("I received: "));
		Serial.println(incomingByte, DEC);
		if (incomingByte == 'r') {
			sendRequestForConf();
		} else if(incomingByte == 'a') {
			printStruct();
		}
	}
	
	//if (!digitalRead(CAN0_INT)) {                         // If CAN0_INT pin is low, read receive buffer
	//	wasRead = true;
	//	INT32U rxId;
	//	unsigned char len = 0;
	//	unsigned char rxBuf[8];
	//	CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
	//	
	//	char msgString[128] = { 0 };
	//	//sprintf(msgString, "Extended ID: 0x%.9lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
	//	//Serial.print(msgString);
	//	//for (byte i = 0; i<len; i++) {
	//	//	sprintf(msgString, " 0x%.2X", rxBuf[i]);
	//	//	Serial.print(msgString);
	//	//}
	//	//Serial.println();

	//	//Serial.println(rxId);

	//	//* 27 bit v odpovedi znamena odpoved na ziadost o konfiguraciu
	//	if (bitRead(rxId, 27) == 1) {
	//		//* prislo nastavenie pre konfiguraciu
	//		INT32U id = rxId;
	//		bitClear(id, 27);
	//		bitClear(id, 28);
	//		bitClear(id, 29);
	//		bitClear(id, 30);
	//		bitClear(id, 31);
	//		INT32U macID = id;
	//		//Serial.print("Konfiguracia pre: ");
	//		//Serial.println(macID);
	//		
	//		for (int i = 0; i < 25; i++) {
	//			if (gConfMessages[i].macID == 0) {
	//				gConfMessages[i].macID = macID;
	//				for (int ii = 0; ii < len; ii++) {
	//					gConfMessages[i].confData[ii] = rxBuf[ii];
	//					//sprintf(msgString, "buf:%d", rxBuf[ii]);
	//					//Serial.print(msgString);
	//				}
	//				printData = true;
	//				break;
	//			}
	//		}
	//	}	
		//printStruct();
	//}
	//if (!wasRead && printData) {
	//	printStruct();
	//	printData = false;
	//}
	//wasRead = false;
	delay(10);   // send data per 100ms
}
