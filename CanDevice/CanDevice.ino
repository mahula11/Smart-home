/*
 Name:		CanDevice.ino
 Created:	4/3/2017 8:57:36 PM
 Author:	Marek
*/

#include "device.h"
#include "eepromConf.h"

//#define WRITE_MAC_ADDRESS
#ifdef WRITE_MAC_ADDRESS

void setup() {
	//* write unique MAC address to EEPROM
	uint16_t address = 1;
	EEPROM.writeInt(EEPROM_ADDRESS__MAC_ADDRESS, address);
}

#else

EepromConf eepromConf;
Device device;

void setup() {
	Serial.begin(115200);

	uint16_t address = eepromConf.getMacAddress();
	if (address == 0) {		
		while (1) {
			Serial.println("No MAC address given, please configure address!!!");
			delay(1000);
		}
	}
	device.init(eepromConf);
}

void loop() {
	device.update();
	
	if (Serial.available()) {
		int incomingByte = Serial.read();
		Serial.print(F("I received: "));
		Serial.println(incomingByte, DEC);
		if (incomingByte == 'r') {
			//sendRequestForConfiguration();
		} else if(incomingByte == 'a') {
			//printStruct();
		}
	}
	
	delay(10);   // send data per 100ms
}

//void printStruct() {
//	Serial.println();
//	char msgString[128];
//	for (int i = 0; i < 25; i++) {
//		//if (gConfMessages[i].macID != 0) {
//		//	sprintf(msgString, "MacID:%ld, deviceType:%d, pin:%d, canID:%d, routable:%d", gConfMessages[i].macID,
//		//		gConfMessages[i].confData[0],
//		//		gConfMessages[i].confData[1],
//		//		gConfMessages[i].confData[2],
//		//		gConfMessages[i].confData[3]);
//		//	Serial.println(msgString);
//		//}
//	}
//}

#endif