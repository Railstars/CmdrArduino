
/********************
* Creates a minimum DCC command station that flips a turnout open and closed.
* The DCC waveform is output on Pin 9, and is suitable for connection to an LMD18200-based booster directly,
* or to a single-ended-to-differential driver, to connect with most other kinds of boosters.
********************/

#include <DCCPacket.h>
#include <DCCPacketQueue.h>
#include <DCCPacketScheduler.h>


DCCPacketScheduler dps;
byte prev_state = 1;
unsigned long timer = 0;
unsigned int address = 4; //this address is not, strictly speaking, the accessory decoder address, but the address as it appears to the user

void setup() {
  Serial.begin(115200);
  dps.setup();

  //set up button on pin 4
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH); //activate built-in pull-up resistor  
}

void loop() {
  if(millis() - timer > 1000) //only do this one per seconds
  {
    if(prev_state)
    {
      dps.setBasicAccessory((address >> 2) + 1, ((address - 1) & 0x03) << 1);
    }
    else
    {
      dps.unsetBasicAccessory((address >> 2) + 1, ((address - 1) & 0x03) << 1);
    }
    prev_state = prev_state?0:1;
    timer = millis();
  }
  
  dps.update();
}
