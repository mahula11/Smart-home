#pragma once

#include <smartHouse.h>
#include <Streaming.h>

struct VALUE {
	byte _value;
	bool _modified;
	VALUE() : _value(1), _modified(false) {}
};

class Configuration {
private:
	CONF * _pConf;
	VALUE * _pValues;
	//bool _modified;

	MacID _macID;
	uint8_t _watchdogTimeout;
	uint8_t _autoResetTime;
	uint8_t _confCount;
	uint8_t _canBusSpeed;

public:
	Configuration();
	~Configuration();

	void setConfigurationStatic(uint8_t confCount, uint8_t watchdogTimeout, uint8_t autoResetTime, MacID macID, uint8_t canBusSpeed);

	byte getCount();
	uint16_t getMacAddress();
	void setConfiguration(CONF * pConf);
	CDataBase * getConf(byte index);
	void setConfValue(byte index, byte value, bool modifyFlag);
	VALUE * getConfValue(byte index);
	//bool isModifiedValue();
	//void resetModifiedValue();
	unsigned long getAutoResetTime();
	void setAutoResetTime(uint8_t val);
	void setCanBusSpeed(uint8_t speed);
	uint8_t getCanBusSpeed();
};

