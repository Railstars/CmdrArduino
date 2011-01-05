/********************
* Creates a minimum DCC command station from a potentiometer connected to analog pin 0,
* and a button connected to ground on one end and digital pin 4 on the other end. See this link
* http://www.arduino.cc/en/Tutorial/AnalogInput
* The DCC waveform is output on Pin 9, and is suitable for connection to an LMD18200-based booster directly,
* or to a single-ended-to-differential driver, to connect with most other kinds of boosters.
********************/

#include <DCCPacket.h>
#include <DCCPacketQueue.h>
#include <DCCPacketScheduler.h>


DCCPacketScheduler dps;
unsigned int analog_value;
char speed_byte, old_speed = 0;
byte count = 0;
byte prev_state = 1;
boolean toggle = false;

void setup() {
  dps.setup();

  //set up button on pin 4
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH); //activate built-in pull-up resistor  
  
  //Serial.begin(9600);
}

void loop() {
  //handle reading button, controls F0
  byte button_state = digitalRead(4); //high == not pushed; low == pushed
  if(button_state && (button_state != prev_state))
  {
    //toggle!
    toggle = !toggle;
    if(toggle)
      dps.opsProgramCV(3,3,10); //set decoder address 03's CV3 (acceleration rate) to 10
    else
      dps.opsProgramCV(3,3,0); //set decoder address 03's CV3 (acceleration rate) to 0
  }
  prev_state = button_state;

  //handle reading throttle
  analog_value = analogRead(0);
  speed_byte = (analog_value >> 2)-127; //divide by two to take a 0-1023 range number and make it 0-127 range.
  if(speed_byte != old_speed)
  {
    dps.setSpeed128(3,speed_byte);
    old_speed = speed_byte;
  }
  dps.update();
  
  ++count;
}
