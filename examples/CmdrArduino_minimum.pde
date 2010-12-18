/********************
* Creates a minimum DCC command station from a potentiometer connected to analog pin 0, as per this link
* http://www.arduino.cc/en/Tutorial/AnalogInput
* The DCC waveform is output on Pin 9, and is suitable for connection to an LMD18200-based booster directly,
* or to a single-ended-to-differential driver, to connect with most other kinds of boosters.
********************/

#include <DCCPacket.h>
#include <DCCPacketScheduler.h>
#include <PacketQueue.h>

DCCPacketScheduler dps;
unsigned int analog_value;
byte speed_byte;

void setup() {
  dps.setup();
}

void loop() {
  //read analog pin 1, use to drive the loco at address 03
  analog_value = analogRead(0);
  speed_byte = analog_value >> 3; //divide by two to take a 0-1023 range number and make it 0-127 range.
  dps.setSpeed128(3,speed_byte);
  dps.update();
}
