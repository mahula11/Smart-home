/*
 Name:		CanDevice.ino
 Created:	4/3/2017 8:57:36 PM
 Author:	Marek
*/

#include "device.h"

//#define WRITE_MAC_ADDRESS
#ifdef WRITE_MAC_ADDRESS

void setup() {
	Serial.begin(115200);
	//* write unique MAC address to EEPROM
	uint16_t address = 2;
	EEPROM.writeInt(EEPROM_ADDRESS__MAC_ADDRESS, address);
	EEPROM.writeInt(EEPROM_ADDRESS__COUNT, 0);
}

void loop() {
	Serial << F("Defined WRITE_MAC_ADDRESS, it's just for inserting unique MAC address, then please remove definition WRITE_MAC_ADDRESS") << endl;
	delay(15000);
}

#else

EepromConf eepromConf;
Device device;

void setup() {
	Serial.begin(115200);

	MacID address = eepromConf.getMacAddress();
	if (address == 65535) {		
		while (1) {
			DEBUG(F("No MAC address given, please configure address!!!") << endl
					<< F("Need to define WRITE_MAC_ADDRESS and put properly unique address.") << endl);
			delay(15000);
		}
	}
	DEBUG("address = " << address << endl);
	//Serial.print("eepromConf: ");
	//Serial.println((INT32U)&eepromConf, HEX);
	//DEBUG("eepromConf: " << &eepromConf << endl);
	device.init();
}

void loop() {
	device.update();
	
	//if (Serial.available()) {
	//	int incomingByte = Serial.read();
	//	Serial.print(F("I received: "));
	//	Serial.println(incomingByte, DEC);
	//	if (incomingByte == 'r') {
	//		//sendRequestForConfiguration();
	//	} else if(incomingByte == 'a') {
	//		//printStruct();
	//	}
	//}
	
	delay(10);   // send data per 100ms
}

#endif
