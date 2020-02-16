#include "Relay.h"
#include "Definitions.h"
// 
// 
// 

//RelayClass::RelayClass(HardwareSerial & serial) : Serial_(serial)
Relay::Relay()
{
}

void Relay::init(int pin, bool activeLow)
{
// The relay with invalid pinNumber
// will be used to trigger sending a stat message when openHab
// server reboots 
    pin_ = pin;
    isActiveLow_ = activeLow;
    // state_ = false;
	Sprint("INIT pin is ");
	Sprintln(pin_);
    if (pin_ != INVALID_RELAY_PIN)
    {
        pinMode(pin_, OUTPUT);
        // Always try to avoid duplicate code.
        // Instead of writing digitalWrite(pin, LOW) here,
        // call the function off() which already does that
        off();
    }

	// if(pinNumber != INVALID_RELAY_PIN){
	// pin_ = pinNumber;
	// pinMode(pinNumber_, OUTPUT);
	// digitalWrite(pinNumber_, HIGH);
	// // digitalWrite(pinNumber_, LOW);
	// state_ = false;
	// // Sprint("RelayClass: init relay pin ");Sprintln(pinNumber_);
	// }else{
	// 	// For the OpenHab message trigger fake relay we only set
	// 	// pinNumber_ and  state_ = false;
	// pinNumber_ = pinNumber;
	// state_ = false;
	// // Sprint("RelayClass: init relay pin ");Sprintln(pinNumber_);
	// }
}
void Relay::on()
{
 			Sprint("Relay.cpp on() pin ");
 			Sprint(pin_);
 			Sprintln(" ");
    if (pin_ != INVALID_RELAY_PIN)
    {
 			Sprintln("Mina proverka za invalid pin");

        if (isActiveLow_)
        {
            digitalWrite(pin_, LOW);
        }
        else
        {
            digitalWrite(pin_, HIGH);
        }
    }
	state_ = true;
}
void Relay::off()
{
 			Sprint("Relay.cpp off() pin ");
 			Sprint(pin_);
 			Sprintln(" ");
    if (pin_ != INVALID_RELAY_PIN)
    {
 			Sprintln("Mina proverka za invalid pin");
        if (isActiveLow_)
        {

           digitalWrite(pin_, HIGH);
			Sprintln("HIGH");
        }
        else
        {
            digitalWrite(pin_, LOW);
			Sprintln("LOW");

        }
    }
	state_ = false;
}

bool Relay::state()
{
    // false - OFF, true - ON
    return state_;
}

// void Relay::switchOn()
// {
// 	if(pinNumber_ != INVALID_RELAY_PIN){
// 		digitalWrite(pinNumber_, LOW);
// 		// digitalWrite(pinNumber_, HIGH);
// 		state_ = true;
// 	Sprint("RelayClass: ON relay pin ");Sprintln(pinNumber_);
// 	} else{
// 		// For the OpenHab message trigger fake relay we do nothing
// 	}
// }

// void Relay::switchOff()
// {	
// 	if(pinNumber_ != INVALID_RELAY_PIN){
// 	digitalWrite(pinNumber_, HIGH);
// 	// digitalWrite(pinNumber_, LOW);
// 	state_ = false;
// 	Sprint("RelayClass: OFF relay pin ");Sprintln(pinNumber_);
// 	} else{
// 		// For the OpenHab message trigger fake relay we do nothing
// 	}
	
// }

// bool Relay::isOn()
// {
// 	return state_;
// }

// Relay relay