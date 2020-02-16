#pragma once

#define DEBUG

#ifdef DEBUG
//https://arduino.stackexchange.com/questions/9857/can-i-make-the-arduino-ignore-serial-print
//If you insist on top performance, the best thing would be to use a macro for that:

#define Sbegin(a) (Serial.begin(a))
#define Sprintln(a) (Serial.println(a))
#define Sprint(a) (Serial.print(a))
#define Swrite(a) (Serial.write(a))

//Then instead of
//Serial.println(F("Hello world!"));
//write
//Sprintln(F("Hello world!"));
#else
//etc.To deactivate the Serial printing, define the macro empty :
#define Sbegin(a)
#define Sprintln(a)
#define Sprint(a)
#define Swrite(a)
#endif

// #define MAX_NUMBER_OF_RELAYS 33 // no need of this
#define INVALID_RELAY_PIN 255


// A better versionis:
// #ifndef DEBUG
//     NSLog(@"-1");
// #elif DEBUG == 0
//     NSLog(@"0");
// #else
//     NSLog(@"%d", DEBUG);
// #endif