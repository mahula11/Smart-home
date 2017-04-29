#pragma once

#include <EEPROMex.h>

#include "Arduino.h"


class EepromConf {
public:
	EepromConf();
	~EepromConf();

	uint16_t getMacAddress();
};

