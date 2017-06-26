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
	Serial << F("MAC_ID:") << EEPROM.readInt(EEPROM_ADDRESS__MAC_ADDRESS) << endl << 
		F("WatchdogTimeout:") << EEPROM.readByte(EEPROM_ADDRESS__WATCHDOG_TIMEOUT) << endl <<
		F("Conf count:") << EEPROM.readByte(EEPROM_ADDRESS__CONF_COUNT) << endl;
	EEPROM.writeInt(EEPROM_ADDRESS__MAC_ADDRESS, address);
	EEPROM.writeByte(EEPROM_ADDRESS__WATCHDOG_TIMEOUT, WATCHDOG_TIMEOUT::to1000ms);
	EEPROM.writeByte(EEPROM_ADDRESS__CONF_COUNT, 0);
}

void loop() {
	Serial << F("Defined WRITE_MAC_ADDRESS, it's just for inserting unique MAC address, then please remove definition WRITE_MAC_ADDRESS") << endl;
	delay(15000);
}

#else

EepromConf eepromConf;
Device device;

void watchdogSetup(void);

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
	DEBUG(VAR(address));
	//Serial.print("eepromConf: ");
	//Serial.println((INT32U)&eepromConf, HEX);
	//DEBUG("eepromConf: " << &eepromConf << endl);
	device.init();

	watchdogSetup();
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

void watchdogSetup(void) {
	cli(); // disable all interrupts
	wdt_reset(); // reset the WDT timer
	/*
	Bit Name
	7	WDIF
	6	WDIE
	5	WDP3
	4	WDCE
	3	WDE
	2	WDP2
	1	WDP1
	0	WDP0

	WDP3 WDP2 WDP1 WDP0 Time-out(ms)
	0	  0    0    0    16
	0	  0    0    1    32
	0	  0    1    0    64
	0 	  0    1    1    125
	0 	  1    0    0    250
	0 	  1    0    1    500
	0 	  1    1    0    1000
	0 	  1    1    1    2000
	1 	  0    0    0    4000
	1 	  0    0    1    8000

	WDTCSR configuration:
	WDIE = 1: Interrupt Enable
	WDE = 1 :Reset Enable
	WDP3 = 0 :For 2000ms Time-out
	WDP2 = 1 :For 2000ms Time-out
	WDP1 = 1 :For 2000ms Time-out
	WDP0 = 1 :For 2000ms Time-out
	*/

	uint8_t wdp0 = 0;
	uint8_t wdp1 = 0;
	uint8_t wdp2 = 0;
	uint8_t wdp3 = 0;
	uint8_t a= eepromConf.getWatchdogTimeout();
	WATCHDOG_TIMEOUT to = (WATCHDOG_TIMEOUT) eepromConf.getWatchdogTimeout();
	switch (to) {
		//case to16ms:
		//	break;
		//case to32ms:
		//	wdp0 = 1;
		//	break;
		//case to64ms:
		//	wdp1 = 1;
		//	break;
		//case to125ms:
		//	wdp0 = wdp1 =  1;
		//	break;
		case to250ms:
			wdp2 = 1;
			break;
		case to500ms:
			wdp0 = wdp2 = 1;
			break;
		case to1000ms:
			wdp1 = wdp2= 1;
			break;
		case to2000ms:
			wdp0 = wdp1 = wdp2 = 1;
			break;
		case to4000ms:
			wdp3 = 1;
			break;
		case to8000ms:
			wdp0 = wdp3 = 1;
			break;
	}

	// Enter Watchdog Configuration mode:
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	// Set Watchdog settings:
	WDTCSR = (1 << WDIE) | (1 << WDE) | (wdp3 << WDP3) | (wdp2 << WDP2) | (wdp1 << WDP1) | (wdp0 << WDP0);
	sei();
}

ISR(WDT_vect) // Watchdog timer interrupt.
{
	// Include your code here - be careful not to use functions they may cause the interrupt to hang and
	// prevent a reset.
	//* nedavat sem Serial.print(), lebo vyvolava prerusenie
}


#endif
