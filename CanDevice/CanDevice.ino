/*
 Name:		CanDevice.ino
 Created:	4/3/2017 8:57:36 PM
 Author:	Marek
*/

//#define _VM_AUTO_REPORT_LAST_MS_ 0

#include <SPI.h>
#include <EEPROMVar.h>
#include <EEPROMex.h>
#include <Streaming.h>
#include <smartHouse.h>
#include <dataTypes.h>
#include <CanID.h>
#include <mcp_can_dfs.h>
#include <mcp_can.h>
#include "device.h"

//#define WRITE_MAC_ADDRESS
#ifdef WRITE_MAC_ADDRESS

void setup() {
	Serial.begin(115200);
	//* write unique MAC address to EEPROM
	uint16_t address = 2;
	Serial << F("MAC_ID:") << EEPROM.readInt(EEPROM_ADDRESS__MAC_ADDRESS) << endl << 
		F("WatchdogTimeout:") << EEPROM.readByte(EEPROM_ADDRESS__WATCHDOG_TIMEOUT) << endl <<
		F("AutoResetTime:") << EEPROM.readByte(EEPROM_ADDRESS__AUTO_RESET_TIME) << endl <<
		F("Conf count:") << EEPROM.readByte(EEPROM_ADDRESS__CONF_COUNT) << endl;
	EEPROM.writeInt(EEPROM_ADDRESS__MAC_ADDRESS, address);
	EEPROM.writeByte(EEPROM_ADDRESS__WATCHDOG_TIMEOUT, WATCHDOG_TIMEOUT::to2000ms);
	EEPROM.writeByte(EEPROM_ADDRESS__AUTO_RESET_TIME, AUTO_RESET_TIMES::arDisable);
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

//#define _VM_AUTO_REPORT_LAST_MS_ 0

void setup() {
	// immediately disable watchdog timer so set will not get interrupted
	wdt_disable();

	//Serial.begin(115200);
	//Serial.begin(921600);
	Serial.begin(500000);

	MacID address = eepromConf.getMacAddress();
	if (address == 65535) {		
		while (1) {
			DEBUG(F("No MAC address given, please configure address!!!") << endl
					<< F("Need to define WRITE_MAC_ADDRESS and put properly unique address.") << endl);
			delay(15000);
		}
	}

	DEBUG(F("--CanDevice started!--"));
	DEBUG(F("MacID:") << address);
	DEBUG(F("WATCHDOG_TIMEOUT:") << EEPROM.readByte(EEPROM_ADDRESS__WATCHDOG_TIMEOUT));
	DEBUG(F("AutoResetTime:") << EEPROM.readByte(EEPROM_ADDRESS__AUTO_RESET_TIME));
	DEBUG(F("Conf count:") << EEPROM.readByte(EEPROM_ADDRESS__CONF_COUNT));
	device.init();

	//watchdogSetup();
}

void loop() {
	device.update();
}

void watchdogSetup() {
	// enable the watchdog timer. There are a finite number of timeouts allowed (see wdt.h).
	// Notes I have seen say it is unwise to go below 250ms as you may get the WDT stuck in a
	// loop rebooting.
	// The timeouts I'm most likely to use are:
	// WDTO_1S
	// WDTO_2S
	// WDTO_4S
	// WDTO_8S

	WATCHDOG_TIMEOUT to = (WATCHDOG_TIMEOUT)eepromConf.getWatchdogTimeout();
	//DEBUG(F("WATCHDOG_TIMEOUT1:") << to);
	switch (to) {
		case to250ms:
			wdt_enable(WDTO_250MS);
			break;
		case to500ms:
			wdt_enable(WDTO_500MS);
			break;
		case to1000ms:
			wdt_enable(WDTO_1S);
			break;
		case to2000ms:
			wdt_enable(WDTO_2S);
			break;
		case to4000ms:
			wdt_enable(WDTO_4S);
			break;
		case to8000ms:
		default:
			wdt_enable(WDTO_8S);
			break;
	}
}

//void watchdogSetup(void) {
//	cli(); // disable all interrupts
//	wdt_reset(); // reset the WDT timer
//	/*
//	Bit Name
//	7	WDIF
//	6	WDIE
//	5	WDP3
//	4	WDCE
//	3	WDE
//	2	WDP2
//	1	WDP1
//	0	WDP0
//
//	WDP3 WDP2 WDP1 WDP0 Time-out(ms)
//	0	  0    0    0    16
//	0	  0    0    1    32
//	0	  0    1    0    64
//	0 	  0    1    1    125
//	0 	  1    0    0    250
//	0 	  1    0    1    500
//	0 	  1    1    0    1000
//	0 	  1    1    1    2000
//	1 	  0    0    0    4000
//	1 	  0    0    1    8000
//
//	WDTCSR configuration:
//	WDIE = 1: Interrupt Enable
//	WDE = 1 :Reset Enable
//	WDP3 = 0 :For 2000ms Time-out
//	WDP2 = 1 :For 2000ms Time-out
//	WDP1 = 1 :For 2000ms Time-out
//	WDP0 = 1 :For 2000ms Time-out
//	*/
//
//	uint32_t wdp0 = 0;
//	uint32_t wdp1 = 0;
//	uint32_t wdp2 = 0;
//	uint32_t wdp3 = 0;
//	//uint8_t a= eepromConf.getWatchdogTimeout();
//	WATCHDOG_TIMEOUT to = (WATCHDOG_TIMEOUT) eepromConf.getWatchdogTimeout();
//	//to = 3;
//	//DEBUG(to);
//	// Enter Watchdog Configuration mode:
//	//WDTCSR |= (1 << WDCE) | (1 << WDE);
//	switch (to) {
//		//case to16ms:
//		//	break;
//		//case to32ms:
//		//	wdp0 = 1;
//		//	break;
//		//case to64ms:
//		//	wdp1 = 1;
//		//	break;
//		//case to125ms:
//		//	wdp0 = wdp1 =  1;
//		//	break;
//		case to250ms:
//			wdp2 = 1;
//			//DEBUG(F("22222"));
//			// Enter Watchdog Configuration mode:
//			WDTCSR |= (1 << WDCE) | (1 << WDE);
//			// Set Watchdog settings:
//			WDTCSR = (1 << WDIE) | (1 << WDE) | (wdp3 << WDP3) | (wdp2 << WDP2) | (wdp1 << WDP1) | (wdp0 << WDP0);
//			break;
//		case to500ms:
//			wdp0 = wdp2 = 1;
//			//DEBUG(F("33333"));
//			// Enter Watchdog Configuration mode:
//			WDTCSR |= (1 << WDCE) | (1 << WDE);
//			// Set Watchdog settings:
//			WDTCSR = (1 << WDIE) | (1 << WDE) | (wdp3 << WDP3) | (wdp2 << WDP2) | (wdp1 << WDP1) | (wdp0 << WDP0);
//			break;
//		case to1000ms:
//			wdp1 = wdp2= 1;
//			//DEBUG(F("44444444"));
//			// Enter Watchdog Configuration mode:
//			WDTCSR |= (1 << WDCE) | (1 << WDE);
//			// Set Watchdog settings:
//			WDTCSR = (1 << WDIE) | (1 << WDE) | (wdp3 << WDP3) | (wdp2 << WDP2) | (wdp1 << WDP1) | (wdp0 << WDP0);
//			break;
//		case to2000ms:
//		{
//			wdp0 = wdp1 = wdp2 = 1;
//			wdp3 = 0;
//			// Set Watchdog settings:
//			//DEBUG(F("111111"));
//			// Enter Watchdog Configuration mode:
//			WDTCSR |= (1 << WDCE) | (1 << WDE);
//			// Set Watchdog settings:
//			//WDTCSR = (1 << WDIE) | (1 << WDE) | (wdp3 << WDP3) | (wdp2 << WDP2) | (wdp1 << WDP1) | (wdp0 << WDP0);
//			WDTCSR = 79;
//			break;
//		}
//		case to4000ms:
//			wdp3 = 1;
//			//DEBUG(F("5555555555555"));
//			// Enter Watchdog Configuration mode:
//			WDTCSR |= (1 << WDCE) | (1 << WDE);
//			// Set Watchdog settings:
//			WDTCSR = (1 << WDIE) | (1 << WDE) | (wdp3 << WDP3) | (wdp2 << WDP2) | (wdp1 << WDP1) | (wdp0 << WDP0);
//			break;
//		case to8000ms:
//			wdp0 = wdp3 = 1;
//			//DEBUG(F("66666666666"));
//			// Enter Watchdog Configuration mode:
//			WDTCSR |= (1 << WDCE) | (1 << WDE);
//			// Set Watchdog settings:
//			WDTCSR = (1 << WDIE) | (1 << WDE) | (wdp3 << WDP3) | (wdp2 << WDP2) | (wdp1 << WDP1) | (wdp0 << WDP0);
//			break;
//	}
//
//
//	//// Enter Watchdog Configuration mode:
//	WDTCSR |= (1 << WDCE) | (1 << WDE);
//	// Set Watchdog settings:
//	WDTCSR = (1 << WDIE) | (1 << WDE) | (0 << WDP3) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);
//	
//	//WDTCSR = (1 << WDIE) | (1 << WDE) | (wdp3 << WDP3) | (wdp2 << WDP2) | (wdp1 << WDP1) | (wdp0 << WDP0);
//
//	//DEBUG(F("Watchdog setting:"));
//	//DEBUG(WDTCSR);
//
//	sei();
//}

//ISR(WDT_vect) // Watchdog timer interrupt.
//{
//	// Include your code here - be careful not to use functions they may cause the interrupt to hang and
//	// prevent a reset.
//	//* nedavat sem Serial.print(), lebo vyvolava prerusenie
//}


#endif
