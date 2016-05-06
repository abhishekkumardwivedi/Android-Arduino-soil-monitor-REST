#include <EtherCard.h>
/*
  Created by Abhishek Dwivedi
  on Jan 03, 2016
  Modified and tested on May 07, 2016
  -----------------------------------
  This minimal code generates sensor node inputs samples and being used
  for IoT analytics. There is also an Android app developed to test if the
  sensor node gives the right output.
 */

int sensorPin = A0;    // select the input pin for the potentiometer
int relayPin = 9;
int ledPin = 13;      // select the pin for the LED
int sensorValue = 0;  // variable to store the value coming from the sensor
int engineOnThreshold = 800; // turn on water machine when moisture reading is above 800.
int engineOffThreshold = 500; // turn off water engine when there is anough watering done.
int scanInterval = 1000; // check moisture level after every second. [1 sec = 1000 ms]

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x10,0x2A,0x3B,0x4C,0x5D,0x6E };

byte Ethernet::buffer[800];
BufferFiller bfill;
static uint32_t timer;

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  Serial.begin(9600);
  digitalWrite(relayPin, LOW);
  NetworkSetup();
}

// called when a ping comes in (replies to it are automatic)
static void gotPinged (byte* ptr) {
  ether.printIp(">>> ping from: ", ptr);
}

void NetworkSetup() {
 if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);

  // call this to report others pinging us
  ether.registerPingCallback(gotPinged);
  
  timer = -9999999; // start timing out right away
  Serial.println();
}

void loop() {
  // read the value from the sensor:
  readMoisture();
  controlMotor();
  sendInternetMessage();
  delay(scanInterval);                  
}

void readMoisture() {
  sensorValue = analogRead(sensorPin);
  Serial.println(sensorValue);
}

void controlMotor() {
  if (sensorValue > engineOnThreshold) {  // turn on engine if it is soid is turning dry
    if(!digitalRead(relayPin)) {
      digitalWrite(relayPin, HIGH);
      Serial.print("water engine turned on\n");
    }
  } else if (sensorValue < engineOffThreshold) { // turn off engine if it is too moist
    if (digitalRead(relayPin)) {
      digitalWrite(relayPin, LOW);
      Serial.print("water engine turned off\n");
    }
  }
}

void sendInternetMessage() {
  word len = ether.packetReceive(); // go receive new packets
  word pos = ether.packetLoop(len); // respond to incoming pings
  
  if (pos)  // check if valid tcp data is received
  ether.httpServerReply(xmlRespose()); // send web page data
}

static word xmlRespose() {  //respond to the requesting guy with sensor value in xml format.
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/xml\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "<?xml version=\"1.0\"?>"
    "<sensorValue>"
    "<moisture>$D</moisture>"
    "</sensorValue>"
  ),sensorValue);
  return bfill.position();
}
