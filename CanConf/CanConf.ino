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
	MacID _macID;	//* Identifikator z CanBus zariadenia (z EEPROM)
	CDataBase * _pData;

	MSG_DATA(MacID macID, CDataBase * pData) {
		_macID = macID;
		_pData = pData;
	}

	MSG_DATA() {
		_macID = 0;
		_pData = nullptr;
	};
};

MSG_DATA gListOfConfs[] = {
	MSG_DATA(1, new CConfDataSwitch(A4)),
	MSG_DATA(1, new CConfDataLight(A2, 2, 5)),
	MSG_DATA(2, new CConfDataSwitch(5)),
	MSG_DATA(2, new CConfDataLight(7, 1, A4)),
	MSG_DATA(-1, nullptr)
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
	CCanID canId;
	byte len = 0;
	MsgData rxBuf;
	CAN0.readMsgBuf(&canId._canID, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
	DEBUG(F("-----------------") << endl << F("Receive msg:") << canId._canID << ",MacID:" << canId.getMacID() << endl);

	if (canId.hasFlag_forConfiguration()) {
		gNewRequestForConfiguration = true;
		DEBUG(F("Configuration msg from (MacID): ") << canId.getMacID() << endl);
		for (int i = 0; i < NUM_MACIDS; i++) {
			if (gMacIDs[i] == 0) {
				gMacIDs[i] = canId.getMacID();
				break;
			}
		}
	}
}

void loop() {
	if (gNewRequestForConfiguration) {
		gNewRequestForConfiguration = false;

		CCanID canID;
		MacID macID = 0;
		canID.setFlag_fromConfiguration();
		DEBUG(F("Conf going to send ..") << endl);

		for (byte i = 0; i < NUM_MACIDS; i++) {
			if (gMacIDs[i] != 0) {
				//* get MacID
				macID = gMacIDs[i];
				//* release place for new MacID adddresses
				gMacIDs[i] = 0;
				//* insert Mac address to CanBus ID
				canID.setMacID(macID);
				DEBUG("canid:" << canID._canID << ", macid:" << macID << endl);
				//* get number of macIDs in DB
				byte cont = 1;
				byte countOfConf = 0;
				byte ii = 0;
				MSG_DATA * pData = &gListOfConfs[ii];
				//* get number of configurations for macID
				while (pData->_macID != -1) {					
					if (macID == pData->_macID) {
						countOfConf++;
					}
					pData = &gListOfConfs[++ii];
				}
				//* send number of configuration
				byte sndStat = CAN0.sendMsgBuf(canID._canID, 1, 1, &countOfConf);
#ifdef DEBUG_BUILD
				DEBUG(F("Number of configuration: ") << countOfConf << endl);
				if (sndStat != CAN_OK) {
					DEBUG(F("Error Sending Configuration!\n"));
				} else {
					DEBUG(F("Configuration Sent Successfully!\n"));
				}
#endif
				//* get particular configuration
				ii = 0;
				pData = &gListOfConfs[ii];
				while (pData->_macID != -1) {
					if (pData->_macID == macID) {
						byte data[10];
						pData->_pData->serialize(data);
						CAN0.sendMsgBuf(canID._canID, 1, pData->_pData->getSize(), data);
						DEBUG(F("Send conf CanID:") << canID._canID << ",deviceType:" << *data << endl);
					}
					pData = &gListOfConfs[++ii];
				}
				//* send it
				delay(50);
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
