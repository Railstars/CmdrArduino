#include "DCCPacketScheduler.h"

/*
 * DCC Waveform Generator
 *
 * 
 * Hardware requirements:
 *     *A DCC booster with Rail A and Rail B wired to pin 9 and 10 respectively.
 *     *A locomotive with a decoder installed, reset to factory defaults.
 *
 * Author: Don Goodman-Wilson dgoodman@artificial-science.org
 *
 * based on software by Wolfgang Kufer, http://opendcc.de
 *
 * Copyright 2010, 2011 Don Goodman-Wilson
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 */

/// An enumerated type for keeping track of the state machine used in the timer1 ISR
/** Given the structure of a DCC packet, the ISR can be in one of 5 states.
      *dos_idle: there is nothing to put on the rails. In this case, the only legal thing
                 to do is to put a '1' on the rails.  The ISR should almost never be in this state.
      *dos_send_premable: A packet has been made available, and so we should broadcast the preamble: 14 '1's in a row
      *dos_send_bstart: Each data byte is preceded by a '0'
      *dos_send_byte: Sending the current data byte
      *dos_end_bit: After the final byte is sent, send a '0'.
*/                 
enum DCC_output_state_t {
  dos_idle,
  dos_send_preamble,
  dos_send_bstart,
  dos_send_byte,
  dos_end_bit
};

DCC_output_state_t DCC_state = dos_idle; //just to start out

/// The currently queued packet to be put on the rails. Default is a reset packet.
byte current_packet[6] = {0,0,0,0,0,0};
/// How many data bytes in the queued packet?
volatile byte current_packet_size = 0;
/// How many bytes remain to be put on the rails?
volatile byte current_byte_counter = 0;
/// How many bits remain in the current data byte/preamble before changing states?
volatile byte current_bit_counter = 14; //init to 14 1's for the preamble
/// A fixed-content packet to send when idle
//byte DCC_Idle_Packet[3] = {255,0,255};
/// A fixed-content packet to send to reset all decoders on layout
//byte DCC_Reset_Packet[3] = {0,0,0};


/************AVR CODE*************/
#if defined(__AVR__)
/// Timer1 TOP values for one and zero
/** S 9.1 A specifies that '1's are represented by a square wave with a half-period of 58us (valid range: 55-61us)
    and '0's with a half-period of >100us (valid range: 95-9900us)
    Because '0's are stretched to provide DC power to non-DCC locos, we need two zero counters,
     one for the top half, and one for the bottom half.

   Here is how to calculate the timer1 counter values (from ATMega168 datasheet, 15.9.2):
 f_{OC1A} = \frac{f_{clk_I/O}}{2*N*(1+OCR1A)})
 where N = prescalar, and OCR1A is the TOP we need to calculate.
 We know the desired half period for each case, 58us and >100us.
 So:
 for ones:
 58us = (8*(1+OCR1A)) / (16MHz)
 58us * 16MHz = 8*(1+OCR1A)
 58us * 2MHz = 1+OCR1A
 OCR1A = 115

 for zeros:
 100us * 2MHz = 1+OCR1A
 OCR1A = 199
 
 Thus, we also know that the valid range for stretched-zero operation is something like this:
 9900us = (8*(1+OCR1A)) / (16MHz)
 9900us * 2MHz = 1+OCR1A
 OCR1A = 19799
 
*/

unsigned int one_count=115; //58us
unsigned int zero_high_count=199; //100us
unsigned int zero_low_count=199; //100us

/// Setup phase: configure and enable timer1 CTC interrupt, set OC1A and OC1B to toggle on CTC
void setup_DCC_waveform_generator() {
  
 //Set the OC1A and OC1B pins (Timer1 output pins A and B) to output mode
 //On Arduino UNO, etc, OC1A is Port B/Pin 1 and OC1B Port B/Pin 2
 //On Arduino MEGA, etc, OC1A is or Port B/Pin 5 and OC1B Port B/Pin 6
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90CAN128__) || defined(__AVR_AT90CAN64__) || defined(__AVR_AT90CAN32__)
  DDRB |= (1<<DDB5) | (1<<DDB6);
#else
  DDRB |= (1<<DDB1) | (1<<DDB2);
#endif

  // Configure timer1 in CTC mode, for waveform generation, set to toggle OC1A, OC1B, at /8 prescalar, interupt at CTC
  TCCR1A = (0<<COM1A1) | (1<<COM1A0) | (0<<COM1B1) | (1<<COM1B0) | (0<<WGM11) | (0<<WGM10);
  TCCR1B = (0<<ICNC1)  | (0<<ICES1)  | (0<<WGM13)  | (1<<WGM12)  | (0<<CS12)  | (1<<CS11) | (0<<CS10);

  // start by outputting a '1'
  OCR1A = OCR1B = one_count; //Whenever we set OCR1A, we must also set OCR1B, or else pin OC1B will get out of sync with OC1A!
  TCNT1 = 0; //get the timer rolling (not really necessary? defaults to 0. Just in case.)
    
  //finally, force a toggle on OC1B so that pin OC1B will always complement pin OC1A
  TCCR1C |= (1<<FOC1B);

}

void DCC_waveform_generation_hasshin()
{
  //enable the compare match interrupt
  TIMSK1 |= (1<<OCIE1A);
}

/// This is the Interrupt Service Routine (ISR) for Timer1 compare match.
ISR(TIMER1_COMPA_vect)
{
  //in CTC mode, timer TCINT1 automatically resets to 0 when it matches OCR1A. Depending on the next bit to output,
  //we may have to alter the value in OCR1A, maybe.
  //to switch between "one" waveform and "zero" waveform, we assign a value to OCR1A.
  
  //remember, anything we set for OCR1A takes effect IMMEDIATELY, so we are working within the cycle we are setting.
  //first, check to see if we're in the second half of a byte; only act on the first half of a byte
  //On Arduino UNO, etc, OC1A is digital pin 9, or Port B/Pin 1
  //On Arduino MEGA, etc, OC1A is digital pin 11, or Port B/Pin 5
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90CAN128__) || defined(__AVR_AT90CAN64__) || defined(__AVR_AT90CAN32__)
  if(PINB & (1<<PINB6)) //if the pin is low, we need to use a different zero counter to enable streched-zero DC operation
#else
  if(PINB & (1<<PINB1)) //if the pin is low, we need to use a different zero counter to enable streched-zero DC operation
#endif

  {
    if(OCR1A == zero_high_count) //if the pin is low and outputting a zero, we need to be using zero_low_count
      {
        OCR1A = OCR1B = zero_low_count;
      }
  }
  else //the pin is high. New cycle is begining. Here's where the real work goes.
  {
     //time to switch things up, maybe. send the current bit in the current packet.
     //if this is the last bit to send, queue up another packet (might be the idle packet).
    switch(DCC_state)
    {
      /// Idle: Check if a new packet is ready. If it is, fall through to dos_send_premable. Otherwise just stick a '1' out there.
      case dos_idle:
        if(!current_byte_counter) //if no new packet
        {
//          Serial.println("X");
          OCR1A = OCR1B = one_count; //just send ones if we don't know what else to do. safe bet.
          break;
        }
        //looks like there's a new packet for us to dump on the wire!
        //for debugging purposes, let's print it out
//        if(current_packet[1] != 0xFF)
//        {
//          Serial.print("Packet: ");
//          for(byte j = 0; j < current_packet_size; ++j)
//          {
//            Serial.print(current_packet[j],HEX);
//            Serial.print(" ");
//          }
//          Serial.println("");
//        }
        DCC_state = dos_send_preamble; //and fall through to dos_send_preamble
      /// Preamble: In the process of producing 14 '1's, counter by current_bit_counter; when complete, move to dos_send_bstart
      case dos_send_preamble:
        OCR1A = OCR1B = one_count;
//        Serial.print("P");
        if(!--current_bit_counter)
          DCC_state = dos_send_bstart;
        break;
      /// About to send a data byte, but have to peceed the data with a '0'. Send that '0', then move to dos_send_byte
      case dos_send_bstart:
        OCR1A = OCR1B = zero_high_count;
        DCC_state = dos_send_byte;
        current_bit_counter = 8;
//        Serial.print(" 0 ");
        break;
      /// Sending a data byte; current bit is tracked with current_bit_counter, and current byte with current_byte_counter
      case dos_send_byte:
        if(((current_packet[current_packet_size-current_byte_counter])>>(current_bit_counter-1)) & 1) //is current bit a '1'?
        {
          OCR1A = OCR1B = one_count;
//          Serial.print("1");
        }
        else //or is it a '0'
        {
          OCR1A = OCR1B = zero_high_count;
//          Serial.print("0");
        }
        if(!--current_bit_counter) //out of bits! time to either send a new byte, or end the packet
        {
          if(!--current_byte_counter) //if not more bytes, move to dos_end_bit
          {
            DCC_state = dos_end_bit;
          }
          else //there are more bytes…so, go back to dos_send_bstart
          {
            DCC_state = dos_send_bstart;
          }
        }
        break;
      /// Done with the packet. Send out a final '1', then head back to dos_idle to check for a new packet.
      case dos_end_bit:
        OCR1A = OCR1B = one_count;
        DCC_state = dos_idle;
        current_bit_counter = 14; //in preparation for a premable...
//        Serial.println(" 1");
        break;
    }
  }
}

#endif /***AVR CODE***/

/************ARM CODE*************/
#if defined(__arm__)

/** S 9.1 A specifies that '1's are represented by a square wave with a half-period of 58us (valid range: 55-61us)
    and '0's with a half-period of >100us (valid range: 95-9900us)
    Because '0's are stretched to provide DC power to non-DCC locos, we need two zero counters,
     one for the top half, and one for the bottom half.

ARM does things, as you might imagine, a bit differently than AVR. Here, we are going to rely on the
LPC17xx motor-control PWM peripheral to do the heavy lifting for us. Very handy, that!

**/

//The LIMIT value is calculated as such
//As default, peripheral clock for MCPWM module is set to CPU clock / 4.
//A DCC '1' has freq = 1 / (58us*2) = 8.62068966 KHz
//Thus, for example, when the peripheral clock is 25MHz, then we should set LIMIT to:
// LIMIT = 25MHz/8.62068966KHz = 2900
// Thus, in general, LIMIT = PCLK * ON_TIME * 2
// where ON_TIME = the amount of time one of the outputs should be set high to indicate a 1 (58us) or 0 (100us or more)
//MATCH is simply half of LIMIT, for our purposes.


uint32_t one_count_half, one_count; //58us
uint32_t zero_count_half, zero_high_count, zero_low_count; //100us

void setup_DCC_waveform_generator()
{
    MCPWM_Init(LPC_MCPWM);
//    MCPWM_ConfigChannel(LPC_MCPWM_TypeDef *MCPWMx, uint32_t channelNum,
//            MCPWM_CHANNEL_CFG_Type * channelSetup)
    MCPWM_CHANNEL_CFG_Type config;
    config.channelType = MCPWM_CHANNEL_EDGE_MODE;
    config.channelPolarity = MCPWM_CHANNEL_PASSIVE_LO; //TODO REALLY!?
    config.channelDeadtimeEnable = DISABLE;
    config.channelUpdateEnable = ENABLE; //TODO REALLY!?
    config.channelTimercounterValue = 0; //TIMER value. Initialize to 0; will count up from here.

    //determine the value of the peripheral clock for the MCPWM peripheral. Not so straightfoward.
    uint32_t PCLK_MCPWM = CLKPWR_GetPCLK(CLKPWR_PCLKSEL_MC);
    one_count_half = PCLK_MCPWM * .000058;
    one_count = one_count_half * 2;
    zero_count_half = PCLK_MCPWM * .000100;
    zero_high_count = zero_count_half * 2;
    zero_low_count = zero_high_count;
    //We start with a '1' because that is a safe value.
    config.channelPeriodValue = one_count; //LIMIT value
    config.channelPulsewidthValue = one_count_half; //MATCH value
    MCPWM_ConfigChannel(LPC_MCPWM, 0, &config);
}

void DCC_waveform_generation_hasshin()
{
  //enable the compare match interrupt
}
extern "C" void MCPWM_IRQHandler(void)
{
    //This interrupt is called every time that TODO
}

#endif

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////
  
DCCPacketScheduler::DCCPacketScheduler(void) : packet_counter(1), default_speed_steps(128), last_packet_address(255)
{
  e_stop_queue.setup(E_STOP_QUEUE_SIZE);
  high_priority_queue.setup(HIGH_PRIORITY_QUEUE_SIZE);
  low_priority_queue.setup(LOW_PRIORITY_QUEUE_SIZE);
  repeat_queue.setup(REPEAT_QUEUE_SIZE);
  //periodic_refresh_queue.setup(PERIODIC_REFRESH_QUEUE_SIZE);
}
    
//for configuration
void DCCPacketScheduler::setDefaultSpeedSteps(byte new_speed_steps)
{
  default_speed_steps = new_speed_steps;
}
    
void DCCPacketScheduler::setup(void) //for any post-constructor initialization
{
  setup_DCC_waveform_generator();
  
  //TODO ELIMINATE THE JERKY START! WHAT'S GOING ON!?

  //Following RP 9.2.4, begin by putting 20 reset packets and 10 idle packets on the rails.
  //use the e_stop_queue to do this, to ensure these packets go out first!

  
  DCCPacket p;
  byte data[] = {0x00};
  
  //reset packet: address 0x00, data 0x00, XOR 0x00; S 9.2 line 75
  p.addData(data,1);
  p.setAddress(0x00);
  p.setRepeat(20);
  p.setKind(reset_packet_kind);
  e_stop_queue.insertPacket(&p);
  
  //WHy in the world is it that what gets put on the rails is 4 reset packets, followed by
  //10 god know's what, followed by something else?
  // C0 FF 00 FF
  // 00 FF FF   what are these?
  
  //idle packet: address 0xFF, data 0x00, XOR 0xFF; S 9.2 line 90
  p.setAddress(0xFF);
  p.setRepeat(10);
  p.setKind(idle_packet_kind);
  e_stop_queue.insertPacket(&p); //e_stop_queue will be empty, so no need to check if insertion was OK.
  
}

//helper functions
void DCCPacketScheduler::repeatPacket(DCCPacket *p)
{
  switch(p->getKind())
  {
    case idle_packet_kind:
    case e_stop_packet_kind: //e_stop packets automatically repeat without having to be put in a special queue
      break;
    case speed_packet_kind: //speed packets go to the periodic_refresh queue
    //  periodic_refresh_queue.insertPacket(p);
    //  break;
    case function_packet_1_kind: //all other packets go to the repeat_queue
    case function_packet_2_kind: //all other packets go to the repeat_queue
    case function_packet_3_kind: //all other packets go to the repeat_queue
    case accessory_packet_kind:
    case reset_packet_kind:
    case ops_mode_programming_kind:
    case other_packet_kind:
    default:
      repeat_queue.insertPacket(p);
  }
}
    
//for enqueueing packets

//setSpeed* functions:
//new_speed contains the speed and direction.
// a value of 0 = estop
// a value of 1/-1 = stop
// a value >1 (or <-1) means go.
// valid non-estop speeds are in the range [1,127] / [-127,-1] with 1 = stop
bool DCCPacketScheduler::setSpeed(unsigned int address,  char new_speed, byte steps)
{
  byte num_steps = steps;
  //steps = 0 means use the default; otherwise use the number of steps specified
  if(!steps)
    num_steps = default_speed_steps;
    
  switch(num_steps)
  {
    case 14:
      return(setSpeed14(address, new_speed));
    case 28:
      return(setSpeed28(address, new_speed));
    case 128:
      return(setSpeed128(address, new_speed));
  }
  return false; //invalid number of steps specified.
}

bool DCCPacketScheduler::setSpeed14(unsigned int address, char new_speed, bool F0)
{
  DCCPacket p(address);
  byte dir = 1;
  byte speed_data_bytes[] = {0x40};
  unsigned int speed = new_speed;
  if(new_speed<0)
  {
    dir = 0;
    speed = new_speed * -1;
  }
  if(!new_speed) //estop!
    speed_data_bytes[0] |= 0x01; //estop
  else if(new_speed == 1) //regular stop!
    speed_data_bytes[0] |= 0x00; //stop
  else //movement
    speed_data_bytes[0] |= map(speed, 2, 127, 2, 15); //convert from [2-127] to [1-14]
  speed_data_bytes[0] |= (0x20*dir); //flip bit 3 to indicate direction;
  //Serial.println(speed_data_bytes[0],BIN);
  p.addData(speed_data_bytes,1);

  p.setRepeat(SPEED_REPEAT);
  
  p.setKind(speed_packet_kind);  

  //speed packets get refreshed indefinitely, and so the repeat doesn't need to be set.
  //speed packets go to the high proirity queue
  return(high_priority_queue.insertPacket(&p));
}

bool DCCPacketScheduler::setSpeed28(unsigned int address, char new_speed)
{
  DCCPacket p(address);
  byte dir = 1;
  byte speed_data_bytes[] = {0x40};
  unsigned int speed = new_speed;
  if(new_speed<0)
  {
    dir = 0;
    speed = new_speed * -1;
  }
  if(!new_speed) //estop!
    speed_data_bytes[0] |= 0x01; //estop
  else if(new_speed == 1) //regular stop!
    speed_data_bytes[0] |= 0x00; //stop
  else //movement
  {
    speed_data_bytes[0] |= map(speed, 2, 127, 2, 0X1F); //convert from [2-127] to [2-31]  
    //most least significant bit has to be shufled around
    speed_data_bytes[0] = (speed_data_bytes[0]&0xE0) | ((speed_data_bytes[0]&0x1F) >> 1) | ((speed_data_bytes[0]&0x01) << 4);
  }
  speed_data_bytes[0] |= (0x20*dir); //flip bit 3 to indicate direction;
  //Serial.println(speed_data_bytes[0],BIN);
  p.addData(speed_data_bytes,1);
  
  p.setRepeat(SPEED_REPEAT);
  
  p.setKind(speed_packet_kind);
    
  //speed packets get refreshed indefinitely, and so the repeat doesn't need to be set.
  //speed packets go to the high proirity queue
  //return(high_priority_queue.insertPacket(&p));
  return(high_priority_queue.insertPacket(&p));
}

bool DCCPacketScheduler::setSpeed128(unsigned int address, char new_speed)
{
  //why do we get things like this?
  // 03 3F 16 15 3F (speed packet addressed to loco 03)
  // 03 3F 11 82 AF  (speed packet addressed to loco 03, speed hex 0x11);
  DCCPacket p(address);
  byte dir = 1;
  unsigned int speed = new_speed;
  byte speed_data_bytes[] = {0x3F,0x00};
  if(new_speed<0)
  {
    dir = 0;
    speed = new_speed * -1;
  }
  if(!new_speed) //estop!
    speed_data_bytes[1] = 0x01; //estop
  else if(new_speed == 1) //regular stop!
    speed_data_bytes[1] = 0x00; //stop
  else //movement
    speed_data_bytes[1] = speed; //no conversion necessary.

  speed_data_bytes[1] |= (0x80*dir); //flip bit 7 to indicate direction;
  p.addData(speed_data_bytes,2);
  //Serial.print(speed_data_bytes[0],BIN);
  //Serial.print(" ");
  //Serial.println(speed_data_bytes[1],BIN);
  
  p.setRepeat(SPEED_REPEAT);
  
  p.setKind(speed_packet_kind);
  
  //speed packets get refreshed indefinitely, and so the repeat doesn't need to be set.
  //speed packets go to the high proirity queue
  return(high_priority_queue.insertPacket(&p));
}

bool DCCPacketScheduler::setFunctions(unsigned int address, uint16_t functions)
{
//  Serial.println(functions,HEX);
  if(setFunctions0to4(address, functions&0x1F))
    if(setFunctions5to8(address, (functions>>5)&0x0F))
      if(setFunctions9to12(address, (functions>>9)&0x0F))
        return true;
  return false;
}

bool DCCPacketScheduler::setFunctions(unsigned int address, byte F0to4, byte F5to8, byte F9to12)
{
  if(setFunctions0to4(address, F0to4))
    if(setFunctions5to8(address, F5to8))
      if(setFunctions9to12(address, F9to12))
        return true;
  return false;
}

bool DCCPacketScheduler::setFunctions0to4(unsigned int address, byte functions)
{
//  Serial.println("setFunctions0to4");
//  Serial.println(functions,HEX);
  DCCPacket p(address);
  byte data[] = {0x80};
  
  //Obnoxiously, the headlights (F0, AKA FL) are not controlled
  //by bit 0, but by bit 4. Really?
  
  //get functions 1,2,3,4
  data[0] |= (functions>>1) & 0x0F;
  //get functions 0
  data[0] |= (functions&0x01) << 4;

  p.addData(data,1);
  p.setKind(function_packet_1_kind);
  p.setRepeat(FUNCTION_REPEAT);
  return(low_priority_queue.insertPacket(&p));
}


bool DCCPacketScheduler::setFunctions5to8(unsigned int address, byte functions)
{
//  Serial.println("setFunctions5to8");
//  Serial.println(functions,HEX);
  DCCPacket p(address);
  byte data[] = {0xB0};
  
  data[0] |= functions & 0x0F;
  
  p.addData(data,1);
  p.setKind(function_packet_2_kind);
  p.setRepeat(FUNCTION_REPEAT);
  return(low_priority_queue.insertPacket(&p));
}

bool DCCPacketScheduler::setFunctions9to12(unsigned int address, byte functions)
{
//  Serial.println("setFunctions9to12");
//  Serial.println(functions,HEX);
  DCCPacket p(address);
  byte data[] = {0xA0};
  
  //least significant four functions (F5--F8)
  data[0] |= functions & 0x0F;
  
  p.addData(data,1);
  p.setKind(function_packet_3_kind);
  p.setRepeat(FUNCTION_REPEAT);
  return(low_priority_queue.insertPacket(&p));
}


//other cool functions to follow. Just get these working first, I think.

//bool DCCPacketScheduler::setTurnout(unsigned int address)
//bool DCCPacketScheduler::unsetTurnout(unsigned int address)

bool DCCPacketScheduler::opsProgramCV(unsigned int address, unsigned int CV, byte CV_data)
{
  //format of packet:
  // {preamble} 0 [ AAAAAAAA ] 0 111011VV 0 VVVVVVVV 0 DDDDDDDD 0 EEEEEEEE 1 (write)
  // {preamble} 0 [ AAAAAAAA ] 0 111001VV 0 VVVVVVVV 0 DDDDDDDD 0 EEEEEEEE 1 (verify)
  // {preamble} 0 [ AAAAAAAA ] 0 111010VV 0 VVVVVVVV 0 DDDDDDDD 0 EEEEEEEE 1 (bit manipulation)
  // only concerned with "write" form here.
  
  DCCPacket p(address);
  byte data[] = {0xEC, 0x00, 0x00};
  
  // split the CV address up among data bytes 0 and 1
  data[0] |= ((CV-1) & 0x3FF) >> 8;
  data[1] = (CV-1) & 0xFF;
  data[2] = CV_data;
  
  p.addData(data,3);
  p.setKind(ops_mode_programming_kind);
  p.setRepeat(OPS_MODE_PROGRAMMING_REPEAT);
  
  return(low_priority_queue.insertPacket(&p));
}
    
//more specific functions

//broadcast e-stop command
bool DCCPacketScheduler::eStop(void)
{
    // 111111111111 0 00000000 0 01DC0001 0 EEEEEEEE 1
    DCCPacket e_stop_packet = DCCPacket(); //address 0
    byte data[] = {0x71}; //01110001
    e_stop_packet.addData(data,1);
    e_stop_packet.setKind(e_stop_packet_kind);
    e_stop_packet.setRepeat(10);
    e_stop_queue.insertPacket(&e_stop_packet);
}
    
bool DCCPacketScheduler::eStop(unsigned int address)
{
    // 111111111111 0	0AAAAAAA 0 01001001 0 EEEEEEEE 1
    // or
    // 111111111111 0	0AAAAAAA 0 01000001 0 EEEEEEEE 1
    DCCPacket e_stop_packet = DCCPacket(address);
    byte data[] = {0x41}; //01000001
    e_stop_packet.addData(data,1);
    e_stop_packet.setKind(e_stop_packet_kind);
    e_stop_packet.setRepeat(10);
    e_stop_queue.insertPacket(&e_stop_packet);
}

//to be called periodically within loop()
void DCCPacketScheduler::update(void) //checks queues, puts whatever's pending on the rails via global current_packet. easy-peasy
{
  DCC_waveform_generation_hasshin();

  //TODO ADD POM QUEUE?
  if(!current_byte_counter) //if the ISR needs a packet:
  {
    DCCPacket p = DCCPacket();
    //Take from e_stop queue first, then high priority queue.
    //every fifth packet will come from low priority queue.
    //every 20th packet will come from periodic refresh queue. (Why 20? because. TODO reasoning)
    //if there's a packet ready, and the counter is not divisible by 5
    //first, we need to know which queues have packets ready, and the state of the this->packet_counter.
    if( !e_stop_queue.isEmpty() ) //if there's an e_stop packet, send it now!
    {
      //e_stop
      e_stop_queue.readPacket(&p); //nothing more to do. e_stop_queue is a repeat_queue, so automatically repeats where necessary.
    }
    else
    {
      bool doHigh = high_priority_queue.notEmpty() && high_priority_queue.notRepeat(last_packet_address);
      bool doLow = low_priority_queue.notEmpty() && low_priority_queue.notRepeat(last_packet_address) &&
                  !((packet_counter % LOW_PRIORITY_INTERVAL) && doHigh);
      bool doRepeat = repeat_queue.notEmpty() && repeat_queue.notRepeat(last_packet_address) &&
                  !((packet_counter % REPEAT_INTERVAL) && (doHigh || doLow));
      //bool doRefresh = periodic_refresh_queue.notEmpty() && periodic_refresh_queue.notRepeat(last_packet_address) &&
      //            !((packet_counter % PERIODIC_REFRESH_INTERVAL) && (doHigh || doLow || doRepeat));
      //examine queues in order from lowest priority to highest.
      //if(doRefresh)
      //{
      //  periodic_refresh_queue.readPacket(&p);
      //  ++packet_counter;
      //}
      //else if(doRepeat)
      if(doRepeat)
      {
        repeat_queue.readPacket(&p);
        ++packet_counter;
      }
      else if(doLow)
      {
        low_priority_queue.readPacket(&p);
        ++packet_counter;
      }
      else if(doHigh)
      {
        high_priority_queue.readPacket(&p);
        ++packet_counter;
      }
      //if none of these conditions hold, DCCPackets initialize to the idle packet, so that's what'll get sent.
      //++packet_counter; //it's a byte; let it overflow, that's OK.
      //enqueue the packet for repitition, if necessary:
      repeatPacket(&p);
    }
    last_packet_address = p.getAddress(); //remember the address to compare with the next packet
    current_packet_size = p.getBitstream(current_packet); //feed to the starving ISR.
    //output the packet, for checking:
    //for(byte i = 0; i < current_packet_size; ++i)
    //{
    //  Serial.print(current_packet[i],BIN);
    //  Serial.print(" ");
    //}
    //Serial.println("");
    current_byte_counter = current_packet_size;
  }
}
