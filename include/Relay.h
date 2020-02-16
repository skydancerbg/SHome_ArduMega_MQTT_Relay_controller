#ifndef Relay_h
#define Relay_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "Definitions.h"

class Relay
{
protected:
	int pin_; // relay pin number 
	bool isActiveLow_ = true;
	// int pinNumber_; // relay pin number 
	bool state_ = false; // false - OFF, true - ON
public:
	Relay();
	void init(int pin, bool activeLow);
  	void on();
  	void off();
  	bool state();
	// void switchOn();
	// void switchOff();
	// bool isActiveLow();
	// bool isOn();
};
//extern Relay relay;

#endif