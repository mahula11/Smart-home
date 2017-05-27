#pragma once

#include <smartHouse.h>
#include <Streaming.h>

struct VALUE {
	byte _value;
	bool _modified;
	VALUE() : _value(0), _modified(false) {}
};

class Configuration {
private:
	CONF * _pConf;
	VALUE * _pValues;
	bool _modified;

public:
	Configuration();
	~Configuration();

	byte getCount();
	uint16_t getMacAddress();
	void setConfiguration(CONF * pConf);
	CDataBase * getConf(byte index);
	void setConfValue(byte index, byte value, bool modifyFlag);
	VALUE getConfValue(byte index);
	bool isModifiedValue();
};

