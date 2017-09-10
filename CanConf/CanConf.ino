/*
 Name:		CanConf.ino
 Created:	4/3/2017 8:55:46 PM
 Author:	Marek
*/

#include <Streaming.h>
#include <smartHouse.h>
#include <mcp_can.h>
#include <SPI.h>


//CONF_DEVICE gConfDevices[] = {	{234, CAN_500KBPS, 65, switchButton}, 
//								{2, CAN_500KBPS, 34, light} };
//
////* pole predstavuje tabulku s konfiguracnymi datami pre jednotlive CanBus zariadenia
//CONF_MESSAGE gConfMessages[] = { { 234, { switchButton, PIN_A0, 50, routable, -1 } },
//								{ 234, { switchButton, PIN_A2, 51, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A2, 52, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A5, 53, routable, -1 } },
//								{ 234, { pushButton, PIN_A5, 54, routable, -1 } },
//								{ 234, { switchButton, PIN_A5, 55, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A2, 56, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A2, 57, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A2, 58, noRoutable, -1 } },
//								{ 234, { stairCaseSwitch, PIN_A5, 59, routable, -1 } },
//								{ 234, { switchButton, PIN_A5, 60, routable, -1 } },
//								{ 234, { switchButton, PIN_A5, 61, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A2, 62, noRoutable, -1 } },
//								{ 222, { light, PIND0, 50, -1 } },
//								{ 234, { switchButton, PIN_A2, 63, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A2, 64, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A5, 65, routable, -1 } },
//								{ 234, { switchButton, PIN_A5, 66, routable, -1 } },
//								{ 234, { switchButton, PIN_A5, 67, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A2, 68, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A2, 69, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A2, 70, noRoutable, -1 } },
//								{ 234, { switchButton, PIN_A5, 71, routable, -1 } },
//								{ 234, { switchButton, PIN_A5, 72, routable, -1 } },
//								{ 234, { light, PIN_A2, 73, -1 } },
//								{ -1, { -1 } } }; //* posledny zaznam, len ukoncene pole


//MSG_DATA gListOfConfs[] = {
//	MSG_DATA(1, 2, (MsgData*)(DEVICE_TYPE_SWITCH, PD7)),
//	MSG_DATA(1, 5, (MsgData*)(DEVICE_TYPE_LIGHT, PB0, (MacID)1, PD7))
//};

struct MSG_DATA {
	//MacID _macID;	//* Identifikator z CanBus zariadenia (z EEPROM)
	CDataBase * _pData;

	MSG_DATA(CDataBase * pData) {
		//_macID = macID;
		_pData = pData;
	}

	MSG_DATA() {
		//_macID = 0;
		_pData = nullptr;
	};
};

MSG_DATA gListOfConfs[] = {
	MSG_DATA(new CConfMsg_switch(1, A4)),
	MSG_DATA(new CConfMsg_light(1, A2, 2, 5)),
	MSG_DATA(new CConfMsg_light(1, A5, 2, 6)),
	MSG_DATA(new CConfMsg_switch(2, 5)),
	MSG_DATA(new CConfMsg_switch(2, 6)),
	MSG_DATA(new CConfMsg_light(2, 7, 1, A4)),
	MSG_DATA(nullptr)
};

volatile bool gNewRequestForConfiguration = false;

const byte NUM_MACIDS = 40;
volatile MacID gMacIDs[NUM_MACIDS] = { 0 };

#define CAN0_INT 2   // Set INT to pin 2
MCP_CAN CAN0(10);    // Set CS to pin 10


void setup() {
	Serial.begin(115200);

	// Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
	if (CAN0.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) == CAN_OK) {
		DEBUG(F("MCP2515 Initialized Successfully!"));
	} else {
		DEBUG(F("Error Initializing MCP2515..."));
	}

	CAN0.setMode(MCP_NORMAL);                     // Set operation mode to normal so the MCP2515 sends acks to received data.
	pinMode(CAN0_INT, INPUT);                            // Configuring pin for /INT input

	attachInterrupt(digitalPinToInterrupt(CAN0_INT), interruptFromCanBus, FALLING);
}

void interruptFromCanBus() {
	CanID canId;
	byte len = 0;
	MsgData rxBuf;
	CAN0.readMsgBuf(&canId._canID, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
	DEBUG(F("-----------------") << endl << F("Receive msg:") << canId._canID << ",MacID:" << canId.getMacID());

	if (canId.hasFlag_forConfiguration()) {
		gNewRequestForConfiguration = true;
		DEBUG(F("Configuration msg from (MacID): ") << canId.getMacID());
		for (int i = 0; i < NUM_MACIDS; i++) {
			if (gMacIDs[i] == 0) {
				gMacIDs[i] = canId.getMacID();
				break;
			}
		}
	}
}

void sendMsg(CDataBase & cdb) {
	byte data[8];
	cdb.serialize(data);
	INT8U ret = CAN0.sendMsgBuf(cdb._destCanID._canID, 1, cdb.getSize(), data);
#ifdef DEBUG_BUILD
	if (ret == CAN_OK) {
		DEBUG(F("Send msg CanID:") << cdb._destCanID._canID << ",deviceType:" << cdb.getType());
	} else {
		DEBUG(F("Failure when send CanID:") << cdb._destCanID._canID << F(",error:") << ret);
	}
#endif
}

void loop() {
	if (gNewRequestForConfiguration) {
		gNewRequestForConfiguration = false;

		//CanID canID;
		MacID macID;
		//canID.setFlag_fromConfiguration();
		//DEBUG(F("Conf going to send .."));

		for (byte i = 0; i < NUM_MACIDS; i++) {
			if (gMacIDs[i] != 0) {
				//* get MacID
				macID = gMacIDs[i];
				//* release place for new MacID adddresses
				gMacIDs[i] = 0;
				//* insert Mac address to CanBus ID
				//canID.setMacID(macID);
				//DEBUG(F("macid:") << macID);
				//* get number of macIDs in DB
				//byte cont = 1;
				byte countOfConf = 0;
				byte ii = 0;
				MSG_DATA * pData = &gListOfConfs[ii];
				//* get number of configurations for macID
				//while (pData->_macID != -1) {					
				while (pData->_pData != nullptr) {
					if (macID == pData->_pData->_destCanID.getMacID()) {
						countOfConf++;
					}
					pData = &gListOfConfs[++ii];
				}
				DEBUG(F("Going to send 1+") << countOfConf << F(" messages to MacID:") << macID);
				//* send number of configuration
				//canID.setFlag_fromConfNumber();
				CConfMsg_numOfConf dc(macID, countOfConf);
				sendMsg(dc);
				//byte sndStat = CAN0.sendMsgBuf(canID._canID, 1, 1, &countOfConf);
//#ifdef DEBUG_BUILD
//				DEBUG(F("Number of configuration:") << countOfConf);
//				if (sndStat != CAN_OK) {
//					DEBUG(F("Error Sending Configuration!"));
//				} else {
//					DEBUG(F("Configuration Sent Successfully!"));
//				}
//#endif
				//* get particular configuration
				ii = 0;
				pData = &gListOfConfs[ii];
				while (pData->_pData != nullptr) {
					if (macID == pData->_pData->_destCanID.getMacID()) {
						//canID.clearConfigPart();
						//canID.setConfigPart(pData->_pData->getType());
						sendMsg(*pData->_pData);
					}
					pData = &gListOfConfs[++ii];
				}
				//* send it
				delay(50);
			}
		}
	}

	if (Serial.available()) {
		int incomingByte = Serial.read();
		Serial.print(F("I received: "));
		Serial.println(incomingByte, DEC);
		switch (incomingByte) {
			case 'r': {
				//*posle reset na MacID 1
				CConfMsg_reset reset(1);
				sendMsg(reset);
			}
				break;
			case 't': {
				//* posle reset na MacID 2
				CConfMsg_reset reset(2);
				sendMsg(reset);
				break;
			}
			case 'a': {
				//* posle zmenu watchdogu
				CConfMsg_watchdog wd(1, to8000ms);
				sendMsg(wd);
				break;
			}
			case 's': {
				//* posle zmenu watchdogu
				CConfMsg_watchdog wd(1, to1000ms);
				sendMsg(wd);
				break;
			}
			case 'p': {
				//* nastavi auto reset
				CConfMsg_autoReset ar(1, ar10s);
				sendMsg(ar);
				break;
			}
			case 'o': {
				//* nastavi auto reset
				CConfMsg_autoReset ar(1, arDisable);
				sendMsg(ar);
				break;
			}
			case 'i': {
				//* nastavi auto reset
				CConfMsg_autoReset ar(1, ar1m);
				sendMsg(ar);
				break;
			}
		}
	}

	delay(100);
}

//INT32U resetBit(INT32U value, byte bit) {
//	return value & ~(1 << bit);
//}

// the loop function runs over and over again until power down or reset
//void loop1() {
//	INT32U rxId;
//	unsigned char len = 0;
//	unsigned char rxBuf[8];
//	char msgString[128];                        // Array to store serial string
//
//	bool isExtended = false;
//	bool isRemote = false;
//
//	if (!digitalRead(CAN0_INT))                         // If CAN0_INT pin is low, read receive buffer
//	{
//		CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
//
//		if ((rxId & 0x80000000) == 0x80000000) {    // Determine if ID is standard (11 bits) or extended (29 bits)
//			sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
//			isExtended = true;
//		} else {
//			sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);
//			isExtended = false;
//		}
//		Serial.print(msgString);
//
//		if ((rxId & 0x40000000) == 0x40000000) {    // Determine if message is a remote request frame.
//			sprintf(msgString, " REMOTE REQUEST FRAME");
//			Serial.print(msgString);
//			isRemote = true;
//		} else {
//			isRemote = false;
//			sprintf(msgString, " STANDARD REQUEST FRAME");
//			for (byte i = 0; i<len; i++) {
//				sprintf(msgString, " 0x%.2X", rxBuf[i]);
//				Serial.print(msgString);
//			}
//		}
//		Serial.println();
//
//		//* potrebujeme najst remote a standard frame
//		//* extrahujeme macID a posleme naspat konfiguraciu
//		if (isRemote && isExtended) {
//			//* zistime, ci je nastaveny bit pre zistenie konfiguracie
//			//* tieto spravy sa budu filtrovat cez masku a filtre, cize sem nepreidu ine spravy, ako s tymto nastavenym bitom
//			if (bitRead(rxId, 28) == 1) {
//				Serial.print("Configuration request id:");
//				bitClear(rxId, 28); //* vymazeme konfiguracny bit (pre request)
//				bitClear(rxId, 31);
//				bitClear(rxId, 30);
//				bitClear(rxId, 29);
//				INT32U messageMacID = rxId;
//				Serial.println(messageMacID);
//				int index = 0;
//				while (gConfMessages[index].macID != -1) {
//					if (messageMacID == gConfMessages[index].macID) {
//						//Serial.println(messageMacID);
//						byte data[8] = { 0 }; // = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
//						INT8U dataLenght = 0;
//						for (int pos = 0; pos < 8; pos++) {
//							if (gConfMessages[index].confData[pos] != (byte)-1) {
//								//Serial.println(gConfMessages[index].confData[pos]);
//								data[pos] = gConfMessages[index].confData[pos];
//								dataLenght++;
//								//Serial.println(gConfMessages[index].confData[dataLenght]);
//							} else {
//								break;
//							}
//						}
//						INT32U id = messageMacID;
//						bitSet(id, 27); //* tento bit hovori, ze sa posiela konfiguracia (prijimac bude moct rozlisit co prislo)
//						byte sndStat = CAN0.sendMsgBuf(id, 1, dataLenght, data);
//						if (sndStat != CAN_OK) {
//							Serial.println("Error Sending Configuration!");
//						} else {
//							Serial.println("Configuration Sent Successfully!");
//							//Serial.println(dataLenght);
//						}
//					}
//					index++;
//					//delay(50);
//				}				
//			}
//		}
//	}
//	//delay(1000);
//}
