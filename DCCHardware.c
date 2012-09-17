#include "Arduino.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "DCCHardware.h"

/// An enumerated type for keeping track of the state machine used in the timer1 ISR
/** Given the structure of a DCC packet, the ISR can be in one of 5 states.
      *dos_idle: there is nothing to put on the rails. In this case, the only legal thing
                 to do is to put a '1' on the rails.  The ISR should almost never be in this state.
      *dos_send_premable: A packet has been made available, and so we should broadcast the preamble: 14 '1's in a row
      *dos_send_bstart: Each data uint8_t is preceded by a '0'
      *dos_send_uint8_t: Sending the current data uint8_t
      *dos_end_bit: After the final uint8_t is sent, send a '0'.
*/                 
typedef enum  {
  dos_idle,
  dos_send_preamble,
  dos_send_bstart,
  dos_send_uint8_t,
  dos_end_bit
} DCC_output_state_t;

DCC_output_state_t DCC_state = dos_idle; //just to start out

/// The currently queued packet to be put on the rails. Default is a reset packet.
uint8_t current_packet[6] = {0,0,0,0,0,0};
/// How many data uint8_ts in the queued packet?
volatile uint8_t current_packet_size = 0;
/// How many uint8_ts remain to be put on the rails?
volatile uint8_t current_uint8_t_counter = 0;
/// How many bits remain in the current data uint8_t/preamble before changing states?
volatile uint8_t current_bit_counter = 14; //init to 14 1's for the preamble
/// A fixed-content packet to send when idle
//uint8_t DCC_Idle_Packet[3] = {255,0,255};
/// A fixed-content packet to send to reset all decoders on layout
//uint8_t DCC_Reset_Packet[3] = {0,0,0};


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

uint16_t one_count=115; //58us
uint16_t zero_high_count=199; //100us
uint16_t zero_low_count=199; //100us

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
  //first, check to see if we're in the second half of a uint8_t; only act on the first half of a uint8_t
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
        if(!current_uint8_t_counter) //if no new packet
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
//          for(uint8_t j = 0; j < current_packet_size; ++j)
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
      /// About to send a data uint8_t, but have to peceed the data with a '0'. Send that '0', then move to dos_send_uint8_t
      case dos_send_bstart:
        OCR1A = OCR1B = zero_high_count;
        DCC_state = dos_send_uint8_t;
        current_bit_counter = 8;
//        Serial.print(" 0 ");
        break;
      /// Sending a data uint8_t; current bit is tracked with current_bit_counter, and current uint8_t with current_uint8_t_counter
      case dos_send_uint8_t:
        if(((current_packet[current_packet_size-current_uint8_t_counter])>>(current_bit_counter-1)) & 1) //is current bit a '1'?
        {
          OCR1A = OCR1B = one_count;
//          Serial.print("1");
        }
        else //or is it a '0'
        {
          OCR1A = OCR1B = zero_high_count;
//          Serial.print("0");
        }
        if(!--current_bit_counter) //out of bits! time to either send a new uint8_t, or end the packet
        {
          if(!--current_uint8_t_counter) //if not more uint8_ts, move to dos_end_bit
          {
            DCC_state = dos_end_bit;
          }
          else //there are more uint8_ts…so, go back to dos_send_bstart
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
