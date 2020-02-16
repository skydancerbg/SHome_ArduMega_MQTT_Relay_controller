//! Follow tasmota MQTT Message flow convention
// https://github.com/arendst/Tasmota/wiki/MQTT-Overview#mqtt-message-flow

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h> // https://pubsubclient.knolleary.net/api.html
#include <stdlib.h>
// #include <TimerAlarms.h>
// * In Definitions.h we #define Serial enable/disable and diferent setings (parameters)!
#include "Definitions.h"
#include "Relay.h"

// MAC address for the controller
byte mac[] = {
  // 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
   0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED //ot star comp MAC-a 
};

//////////*************
//!PLACE YOUR CREDENTIALS IN Private.h as follows:
//////////*************
// MQTT connection parameters:
//const char *mqtt_server = "IP of the server";
//const char *mqttClientId = "your client ID";
//const char *mqtt_user = "mqtt broker user";
//const char *mqtt_pass = "mqtt broker pass";
//////////*************


//////////*************
// Relay MQTT comands/states can be: ON and OFF ???? or 1 and 0 ???? //!
//////////*************
  const long mqtt_reconect_interval = 5000; // 5 seconds
  const long mqtt_stat_msgs_interval = 60000; //one minute for status refresh //300000; 5 min  ???da prashta status na 5 min kato sonoffa????
  unsigned long previous_stat_msgs_Millis = 0;  // will store last time STATUS was send to MQTT

// MQTT reconect in loop - non blocking!!!
  boolean  mqtt_reconnect();
// Handle incomming MQTT messages 
  void mqtt_callback(char* topic, byte* payload, unsigned int length);
// MQTT publish STAT message
  void mqtt_publish_stat_message(char* topic, char* payload);

// char* string manipulation
int stringToInteger(const char *inputstr);  

  EthernetClient ethClient;
  PubSubClient mqttClient(ethClient);
  long lastReconnectAttempt = 0;

// Execute reconect attempt
  boolean reconnect();

// Arduino Mega pinout http://www.circuitstoday.com/wp-content/uploads/2018/02/Arduino-Mega-Pin-Configuration.jpg   

// Relay pins for relays 1-32 ordered for the cabble connectors:
// 255 is invalid pin number used for the zero relay
// pins 18,19 and 22 to 48 even to 16 realy board
// pins 20,21,23 to 33 odd 8 relay board top
// pins 35 to 49 odd botom 8 relay board
//! DO NOT USE: Pins 0 and 1 Rx1,TX1 - used by serial
//! DO NOT USE: Pin 4 - SS pin for the SD card
//! DO NOT USE: Pin 10 - SS for the Ethernet module (10 is default)
//! DO NOT USE: Pin 13 - LED_BUILT_IN pin
//! DO NOT USE: 50, 51, 52, 53 - SPI pins for the Ethernet shield /on the raiser header or on he pin headers/
//! DO NOT USE: 54 do not use, needs to be set to Output HIGH

//! WARNING ! PIN 40 AND 39 SWAPPED BECOUSE OF WRONG CABLE!!!! USE THIS ORDER WITH THE CURRENT CABLE 
     int relayPins[33] = {255, 18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,40,39,41,42,43,44,45,46,47,48,49};
//! THE LINE BELOW IS THE ONE WIT THE RIGHT ORDER OF PINS TO BE USED WITH NEW CABLE
    //  int relayPins[33] = {255, 18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49};
     int ssrPins[6] = {2,3,5,6,7,8};
  // int relayPins[33] = {255, 18,19,22,24,26,28,30,32,34,36,38,40,42,44,46,48,20,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49};

// SS pin Ethernet shield
// se to 12, do not use the default (10)
// SPI pins for the Ethernet shield
// 50 - 53
// 54 do not unuse, needs to be set to Output HIGH
// LED_BUILTIN 13 use for watchdog/heartbeat???
// Available pins:
// 0 - 10

//  MQTT topics:
// const char willTopic[] = "arduino/will";
// const char inTopic[]   = "arduino/in";
// const char outTopic[]  = "arduino/out";
const char *mqtt_LWT_topic = "tele/ardu_mega_heating_ctrl_1/LWT";
// tele/ardu_mega_heating_ctrl_1/STATE
// tele/ardu_mega_heating_ctrl_1/SENSOR
const char *mqtt_command_topic_base = "cmnd/ardu_mega_heating_ctrl_1/";
// tele/ardu_mega_heating_ctrl_1/RESULT
Relay relay[(sizeof(ssrPins) / sizeof(int))+(sizeof(relayPins) / sizeof(int))];


void setup()
{
//! WARNING !! MUST INITIALIZE THE RELAYS FIRST IN ORDER TO AVOID INITIAL TEMPORARY CYCLING 
//! OF THE DEVICES CONNECTED TO THOSE RELAYS AFTER POWER ON 
// INITIALIZE Relays:
  for (size_t i = 0; i < sizeof(relayPins) / sizeof(int); i++)
  {
    Sprint("Initializing relay ");Sprint(i);Sprint(" With pin ");Sprintln(relayPins[i]); 
    relay[i].init(relayPins[i],true); //set the realys as activeLow
  }
  for (size_t i = 0; i < sizeof(ssrPins) / sizeof(int); i++)
  {
    int relayNum = ((sizeof(relayPins) / sizeof(int))+i);
    Sprint("Initializing SSR "); Sprint(relayNum); Sprint(" With pin "); Sprintln(ssrPins[i]); 
    relay[relayNum].init(ssrPins[i],false); //set the realys as activeLow
  }

////
  //! Oboard LED will be ON in case of lost connetion to the MQTT server
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Ethernet.init(pin) to configure the CS pin on the Ethernet shield
  // Ethernet.init(10);  // we use the default pin 10 / no need to set it up

 // Open serial communications and wait for port to open:
  Sbegin(115200);
  // while (!Serial) {
  //   ; // wait for serial port to connect. Needed for native USB port only
  // }

// Start the Ethernet connection:
  Sprintln(F("Initialize Ethernet with DHCP:"));
  if (Ethernet.begin(mac) == 0) {
    Sprintln(F("Failed to configure Ethernet using DHCP"));
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Sprintln(F("Ethernet shield was not found.  Sorry, can't run without hardware. :("));
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Sprintln(F("Ethernet cable is not connected."));
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }// END Start the Ethernet connection

// print local IP address:
  Sprintln(F("IP address: "));
  Sprintln(Ethernet.localIP());
  
// Set MQTT server/port and callback funcion  
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqtt_callback);

}//END setup
////


void loop()
{
// DHCP maintenance: 
  switch (Ethernet.maintain()) {
    case 1:
      //renewed fail
      Sprintln(F("Error: renewed fail"));
      break;

    case 2:
      //renewed success
      Sprintln(F("Renewed success"));
      //print your local IP address:
      Sprintln("My IP address: ");
      Sprintln(Ethernet.localIP());
      break;

    case 3:
      //rebind fail
      Sprintln(F("Error: rebind fail"));
      break;

    case 4:
      //rebind success
      Sprintln(F("Rebind success"));
      //print your local IP address:
      Sprint(F("My IP address: "));
      Sprintln(Ethernet.localIP());
      break;

    default:
      //nothing happened
      break;
  } // End DHCP maintenance

// PubSub MQTT client non blocking reconect based on  https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_reconnect_nonblocking/mqtt_reconnect_nonblocking.ino
// Actually is blocking while the connect attempt - https://github.com/knolleary/pubsubclient/issues/147 
//???? Solution??? https://github.com/knolleary/pubsubclient/issues/583 ??? 
  
  long now = millis();

// MQTT connection maintenance:
  
  if (!mqttClient.connected()) {
  digitalWrite(LED_BUILTIN, HIGH);

    // Sprint("failed, rc=");
    // Sprint(mqttClient.state());
    // Sprintln(" try connecting MQTT server again in " );
    // Sprint(mqtt_reconect_interval - (now - lastReconnectAttempt));
    // Sprintln(" miliseconds " );
    
      if (now - lastReconnectAttempt > mqtt_reconect_interval) {
        lastReconnectAttempt = now;
      // Attempt to reconnect
      Sprintln("" );  
      Sprintln("New MQTT connection attempt..." );  
      if (mqtt_reconnect()) {
        lastReconnectAttempt = 0;
        previous_stat_msgs_Millis = 0; // Schedule next STAT publish in mqtt_stat_msgs_interval
        now = millis(); // Get current millis
        Sprintln(" Connection to MQTT server successful! " );
        
        digitalWrite(LED_BUILTIN, LOW);
  
      }
    }
  } else {
    // Client connected
    mqttClient.loop();
    digitalWrite(LED_BUILTIN, LOW);
  } //END MQTT connection non blocking maintenance

//TODO Process sensors if any

// Send stat message every mqtt_periodic_stat_msgs_period with timer
    // if (now - previous_stat_msgs_Millis > mqtt_stat_msgs_interval) {
    //   mqtt_publish_stat_message();  // Publish mqtt STAT message
    //   previous_stat_msgs_Millis = 0; // Schedule next STAT publish in mqtt_stat_msgs_interval
    //   Sprintln(" Stat message published to mqtt server! " );

    //  }
////
}// END loop
////

// PubSub MQTT reconnect  
  boolean mqtt_reconnect() {
    // Attempt to connect
      if (mqttClient.connect(mqttClientId,mqtt_user,mqtt_pass, mqtt_LWT_topic, 1, true, "Offline")) {
        Sprintln("connected");
        // Once connected, publish an "Online" announcement...
        mqttClient.publish(mqtt_LWT_topic, "Online");
        Sprintln("Online message published on LWT topic");
 //! TEST     REMOVE  
// String str_payload=String(1);
// String str_relay_number=String(13);
// String mqtt_command_topic_str = "cmnd/ardu_mega_heating_ctrl_1/POWER";
// mqtt_command_topic_str += str_relay_number;
// // in a nutshell the statement works where payload is a declared String using latest IDE and PubSubClient
// // client.publish(topic, (char*) payload.c_str())
// // See other issue published at here - igrr/esp8266_pubsubclient.ino
// mqttClient.publish(mqtt_command_topic, (char*) str_payload.c_str())

// и друг начин:
// //Preparing for mqtt send

//     temp_str = String(ftemp); //converting ftemp (the float variable above) to a string
//     temp_str.toCharArray(temp, temp_str.length() + 1); //packaging up the data to publish to mqtt whoa...

//     hum_str = String(humidity); //converting Humidity (the float variable above) to a string
//     hum_str.toCharArray(hum, hum_str.length() + 1); //packaging up the data to publish to mqtt whoa...

//     Serial.print("Publish message: ");//all of these Serial prints are to help with debuging
   
//     client.publish("Workshop Temperature", temp); //money shot
//     client.publish("Workshop Humidity", hum);

// Example 2: String to Integer conversion Arduino
// String val = “1234”;
// int result = val.toInt();  //Converts string to integer

// https://forum.arduino.cc/index.php?topic=593844.0
// float pi = 3.14159;
// char msg[30];
// char val[10];
// dtostrf(pi, 8, 5, val);
// sprintf(msg, "I like pie: %s", val);
//       // Once connected, publish an announcement...
//  client.publish("smartdevice", msg);

// The dtostrf() function converts the double value passed in val into an ASCII
// MQTT commonly uses forward slashes (/) as path delimiters. If you want to use backslashes (\), you have to escape them in C++ code:

// Code: [Select]
// char buffer[15];
// client.publish("WeatherStation1\\BME280\\Temperature", dtostrf(bme.readTemperature(),8,2));

// incomming = .....

// char charBuf[incoming.length() + 1];
// incoming.toCharArray(charBuf,incoming.length() + 1);
// client.publish(mqtt_topic_pub, charBuf);

// Try it ^^

// if ((char)payload[0]=='1'){
//     relays_on();
//   }
//   else{
//     relays_off();
//   }

//! END TEST

        //Subscribe to MQTT topics:
        //
        // Subscribe to  command topic base -  "cmnd/ardu_mega_heating_ctrl_1/"
        //! Subscribe to  command topic POWER for each realy attached ( i.e."cmnd/ardu_mega_heating_ctrl_1/" POWER1, POWER2 ...POWER(n) )
        for (size_t i = 0; i < ((sizeof(ssrPins) / sizeof(int))+(sizeof(relayPins) / sizeof(int))); i++)
        // for (size_t i = 0; i < MAX_NUMBER_OF_RELAYS; i++)
        {
          char str[37] = "cmnd/ardu_mega_heating_ctrl_1/POWER"; // cmnd/ardu_mega_heating_ctrl_1/POWER is 35 chars + 2 chars for - and \0
          char buf[4]; // "-32\0"
          itoa(i,buf,10);
          strcat((char*) str,(char*) buf);
          Sprint("Topic to subscribe: ");
          Sprintln(str);

          mqttClient.subscribe(str);
          //  mqttClient.subscribe("cmnd/ardu_mega_heating_ctrl_1/POWER1");
         /* code */
        }
        
        // mqttClient.subscribe(mqtt_command_topic_base);
        
        // Publish to MQTT topics:
        //
        //???? Publish initial STAT message to mqtt_stat_topic for each relay 
        // mqtt_publish_stat_message();
      }
    return mqttClient.connected();
  }//END mqtt_reconnect()


// Publish STAT MQTT message
  void mqtt_publish_message(char* out_topic, char* payload){
        mqttClient.publish(out_topic, payload);
        Sprint("Message published on "); 
        Sprint(out_topic); 
        Sprint(" topic with payload: ");
        Sprintln(payload); 
    }  
//! Periodic stat example:
//! {"Time":"2020-02-09T19:25:44","Uptime":"3T06:15:04","UptimeSec":281704,"Heap":28,"SleepMode":"Dynamic","Sleep":50,"LoadAvg":19,"MqttCount":8,"POWER1":"OFF","POWER2":"OFF","POWER3":"OFF","POWER4":"OFF","Wifi":{"AP":1,"SSId":"code","BSSId":"F8:D1:11:3B:4B:16","Channel":9,"RSSI":100,"LinkCount":1,"Downtime":"0T00:00:37"}}
// void mqttPublishRESULTmessage(bool state) {
//   mqttClient.publish("stat/WiFi_Mailbox_sensor/MailboxDoor", state ? "CLOSED" : "OPEN");
// }


// Handle incomming MQTT messages
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  //! https://www.instructables.com/id/Arduino-String-Manipulation-Using-Minimal-Ram/

  Sprintln("Message arrived");
    Sprint("Originalniq topic - ");
    Sprintln(topic);
    // strncpy(outputString, (char*)str, i); // copy to outputString
    // outputString[i] = '\0'; // put in new null terminator
    char topic_copy[40]; 
    strncpy(topic_copy, (char*)topic, strlen(topic)+1);
    Sprintln(topic_copy);

  // handle message arrived
//mqtt_command_topic = "cmnd/ardu_mega_heating_ctrl_1/POWER" has 35 characters (plus terminating "\0")!!
//! tova e dobawiane na nomer na rele
  // const char* mod_topic = (char*)topic;
  // char* str_relay_num = "12";
  // strcat((char*) mod_topic,str_relay_num);
//! tuka vmesto topic mi trqbwa substring ot na`aloto

    int number_of_chars = 30; // number of characters in the topic with /POWER at the end ia 35
    char stripped_topic[40];
    strncpy(stripped_topic, (char*)topic_copy, number_of_chars); // copy stripped part to the output String
    stripped_topic[number_of_chars] = '\0'; // put in new null terminator at the end
    Sprint("stripped topic: ");
    Sprintln(stripped_topic);

    // We have mqtt_command_topic_base = "cmnd/ardu_mega_heating_ctrl_1/"
    // now we add POWER at the end
    strcat((char*) stripped_topic,"POWER");
      Sprint("stripped topic with POWER: ");
    Sprintln(stripped_topic);
    
    // Asemble the stat topic name
    // Remove cmnd and repalce with stat
    char* topic_no_prefix = topic_copy + 4;  // new char pointer pointing at fift element of someData
    char stat_topic[40]={"stat"};
    strcat(stat_topic,topic_no_prefix);
    // stat_topic[number_of_chars] = '\0'; //? put in new null terminator at the end
    Sprint("STAT TOPIC NAME IS: ");
    Sprintln(stat_topic);

  if (strcmp(stripped_topic,"cmnd/ardu_mega_heating_ctrl_1/POWER") == 0){
    Sprintln("Mina proverkata za topik s POWER");
    Sprint("Originalniq topic - ");
    Sprintln(topic);

    // Take the realy number from the incomming topic name:
    int relay_number = atoi( &topic_copy[strlen("cmnd/ardu_mega_heating_ctrl_1/POWER")]);
    Sprint("dekodiran nomer na rele ot topika:");
    Sprintln(relay_number);
    // int value = atoi( &topic[35] );
// ... If there is always just one space.  14 is the length of the compare string, so &array [14] is 
// a pointer to the character after that.  
// atoi starts there, looking for digit characters.

//! Payloads ON and OFF are ommited as OpenHab sends both 1 and ON or 0 and OFF to the command topic,
//! We expect 0,1 or empty as payload!
    Sprint("Payload is:");
    Sprintln((char*)payload);
    // if ((char)payload[0]=='\0'){// empty payload means that openHab requests current status of the relay
    //     // Following tasmota convention empty payload on cmnd/ topic should return current state of the realy
    //     // We do not react on unexpected payload, just empty one shuld trigger status message
 
    //     mqttClient.publish(stat_topic, (relay[relay_number].isOn()) ? "ON" : "OFF");

    //     Sprint("Empty payload received on cmnd/ topic.");
    //     Sprint("Relay ");
    //     Sprint(relay_number);
    //     Sprint(" current state (stat/ mqtt message) payload: ");
    //     Sprintln((relay[relay_number].isOn()) ? "ON" : "OFF" );

    // }else 

    if ((char)payload[0]=='1'){
        relay[relay_number].on();
        mqttClient.publish(stat_topic, (relay[relay_number].state()) ? "ON" : "OFF");
        
        Sprint("Relay ");
        Sprintln(relay_number);
        Sprintln(" switchOn command issued");
      }

      else if ((char)payload[0]=='0'){
        relay[relay_number].off();
        mqttClient.publish(stat_topic, (relay[relay_number].state()) ? "ON" : "OFF");
        
        Sprint("Relay ");
        Sprintln(relay_number);
        Sprintln(" switchOFF command issued");

      }//END


  }//END message received on cmnd/ardu_mega_heating_ctrl_1/POWER(n) topic



  //  String stopic = topic;
  // //  If string compare substring .../Power...then
  // String s = "Watering: 1";
  // char last = s.charAt(s.length() - 1);

  // syzdawane na topic ot const char* i nomer na rele
  // const char* mod_topic = (char*)topic;
  // char* str_relay_num = "12";
  // strcat((char*) mod_topic,str_relay_num);
  // // strcat((char*) mod_topic,(char*) str2);


// char str[]={'h','e','l','l','o','!',0}; // completely equivalent to : char* str="hello!";

// if ((char)payload[0]=='1'){
//     relays_on();
//   }
//   else{
//     relays_off();
//   }



}
// char* basicStringAdd(const char *str1, const char *str2) // adds string 2 to the end of string 1
// {
    
//     strcat((char*) str1,(char*) str2); // add strings together, answer in str1
//     strcpy(outputString, str1); // copy string to outputstring
//     return outputString;
// }


// https://pseudo-server.com/blog/wi-fi-remote-using-an-esp8266/
/**
* Using ArduinoJson.h to send a JSON payload via MQTT
*/
// void sendMQTTMessage(char* topic, char* button) {

//   doc["sensor"] = "wooden-remote";
//   doc["button"] = String(button);
//   doc["batt"] = getBattLevel();

//   char buffer[512];
//   size_t n = serializeJson(doc, buffer);
//   client.publish(topic, buffer, n);

//   if(debug){
//     Serial.println("Sending MQTT");
//     Serial.println(button);
//   }

// }

// MQTT topics:

// tele/ardu_mega_heating_ctrl_1/LWT
// tele/ardu_mega_heating_ctrl_1/STATE
// tele/ardu_mega_heating_ctrl_1/SENSOR
// cmnd/ardu_mega_heating_ctrl_1/POWER
// tele/ardu_mega_heating_ctrl_1/RESULT





// PubSubClient:
// int state ()
// Returns the current state of the client. If a connection attempt fails, this can be used to get more information about the failure.

// Returns
// int - the client state, which can take the following values (constants defined in PubSubClient.h):
// -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
// -3 : MQTT_CONNECTION_LOST - the network connection was broken
// -2 : MQTT_CONNECT_FAILED - the network connection failed
// -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
// 0 : MQTT_CONNECTED - the client is connected
// 1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
// 2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
// 3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
// 4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
// 5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect