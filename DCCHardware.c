#include "DCCHardware.h"

/// An enumerated type for keeping track of the state machine used in the timer1 ISR
/** Given the structure of a DCC packet, the ISR can be in one of 5 states.
      *dos_idle: there is nothing to put on the rails. In this case, the only legal thing
                 to do is to put a '1' on the rails.  The ISR should almost never be in this state.
      *dos_send_premable: A packet has been made available, and so we should broadcast the preamble: 14 '1's in a row
      *dos_send_bstart: Each data uint8_t is preceded by a '0'
      *dos_send_byte: Sending the current data uint8_t
      *dos_end_bit: After the final uint8_t is sent, send a '0'.
*/                 
typedef enum  {
  dos_idle,
  dos_send_preamble,
  dos_send_bstart,
  dos_send_byte,
  dos_end_bit
} DCC_output_state_t;

DCC_output_state_t DCC_state = dos_idle; //just to start out

/// The currently queued packet to be put on the rails. Default is a reset packet.
uint8_t current_packet[6] = {0,0,0,0,0,0};
/// How many data uint8_ts in the queued packet?
volatile uint8_t current_packet_size = 0;
/// How many uint8_ts remain to be put on the rails?
volatile uint8_t current_byte_counter = 0;
/// How many bits remain in the current data uint8_t/preamble before changing states?
volatile uint8_t current_bit_counter = 14; //init to 14 1's for the preamble
/// A fixed-content packet to send when idle
//uint8_t DCC_Idle_Packet[3] = {255,0,255};
/// A fixed-content packet to send to reset all decoders on layout
//uint8_t DCC_Reset_Packet[3] = {0,0,0};


// using Arduino
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

/// Using LPC1769 MCPWM
/*******************
 * We use MCPWM channel 3. TC is the timer regsiter. Match is the register that controls period/frequency. Limit controls duty cycle.
 * see 25.8.1
 * Interrupt on LIM0
 * TODO stretched-zero.
 *
 * S 9.1 A specifies that '1's are represented by a square wave with a half-period of 58us (valid range: 55-61us)
 *   and '0's with a half-period of >100us (valid range: 95-9900us)
 *   Because '0's are stretched to provide DC power to non-DCC locos, we need two zero counters,
 *    one for the top half, and one for the bottom half.
 *
 */

/// Using LPC1343 TIMER32
/*******************
 * We use TIMER32_1 match 0 and 1.
 * Interrupt on match0. Timer prescaler = 0
 */

//OLD
//uint16_t one_count=115; //58us
//uint16_t zero_high_count=199; //100us
//uint16_t zero_low_count=199; //100us

//NEW
static volatile  uint32_t clock;
static uint32_t one_count; //=115; //58us
static volatile uint32_t zero_high_count; //=199; //100us
static volatile uint32_t zero_low_count; //=199; //100us
uint8_t pin_flag;
#if defined(LPC1769)
MCPWM_CHANNEL_CFG_Type chan2_setup;
#endif

/// Setup phase: configure and enable timer1 CTC interrupt, set OC1A and OC1B to toggle on CTC
void setup_DCC_waveform_generator() {
#if defined(__AVR__)
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

  #elif defined(LPC1769)
	//determine timings.
	clock = CLKPWR_GetPCLK(CLKPWR_PCLKSEL_MC);
	//convert this from Hz to MHz to make following calculation not have to be floating point.
	clock = clock / 1000000;

	//now that we know the clock speed, we know how long each clock tick lasts.
	one_count = 58 * clock;
	zero_high_count = 100 * clock;
	zero_low_count = 100 * clock;

	clock = one_count;

	MCPWM_Init(LPC_MCPWM);

	//configure output channel 2
	chan2_setup.channelType = MCPWM_CHANNEL_EDGE_MODE;
	chan2_setup.channelPolarity = MCPWM_CHANNEL_PASSIVE_LO;
	chan2_setup.channelDeadtimeEnable = DISABLE;
	chan2_setup.channelUpdateEnable = ENABLE;
	chan2_setup.channelTimercounterValue = 0;
	chan2_setup.channelPeriodValue = one_count << 1; /**< MCPWM Period value */
	chan2_setup.channelPulsewidthValue = clock; /**< MCPWM Pulse Width value */
	MCPWM_ConfigChannel(LPC_MCPWM, 2, &chan2_setup);

	//configure pins
	PINSEL_CFG_Type pin_cfg;
	//MCOA0
	pin_cfg.Portnum = PINSEL_PORT_1;
	pin_cfg.Pinnum = PINSEL_PIN_28;
	pin_cfg.Funcnum = PINSEL_FUNC_1;
	PINSEL_ConfigPin(&pin_cfg);
	//MCOA1
	pin_cfg.Pinnum = PINSEL_PIN_29;
	PINSEL_ConfigPin(&pin_cfg);

#elif defined(LPC1343)
	//initialize timer peripheral
	TIM_TIMERCFG_Type timer_config;
	timer_config.PrescaleOption = TIM_PRESCALE_TICKVAL;
	timer_config.PrescaleValue = 1;
	TIM_Init(LPC_TMR32B1, TIM_TIMER_MODE, &timer_config);

	//determine timings.
	clock = SYSCON_GetAHBClock();
	//convert this from Hz to MHz to make following calculation not have to be floating point.
	clock = clock / 1000000;

	//now that we know the clock speed, we know how long each clock tick lasts.
	one_count = 58 * clock;
	zero_count = 100 * clock;
	zero_high_count = zero_count;
	zero_low_count = zero_count;

	clock = one_count;

	//configure pin MAT1 to be in opposite phase from MAT0
	pin_flag = 1; //start on high cycle.
	IOCON_SetPinFunc(IOCON_PIO1_2, PIO1_2_FUN_PIO);
	GPIO_SetDir(PORT1, GPIO_Pin_2, 1);
	GPIO_ResetBits(PORT1, GPIO_Pin_2);

	IOCON_SetPinFunc(IOCON_PIO1_1, PIO1_1_FUN_PIO);
	GPIO_SetDir(PORT1, GPIO_Pin_1, 1);
	GPIO_SetBits(PORT1, GPIO_Pin_1);

	//configure match channels 0
	timer32_1_chan0_setup.MatchChannel = 0;
	timer32_1_chan0_setup.IntOnMatch = ENABLE;
	timer32_1_chan0_setup.StopOnMatch = DISABLE;
	timer32_1_chan0_setup.ResetOnMatch = ENABLE;
	timer32_1_chan0_setup.ExtMatchOutputType = 0; // DO NOTHING TODO Toggle external output pin if match
	timer32_1_chan0_setup.MatchValue = clock;
	TIM_ConfigMatch(LPC_TMR32B1, &timer32_1_chan0_setup);
	//TIM_MatchPins(LPC_TMR32B1, TIM_PINS_MAT0);

	//setup flags and enable pins
	IOCON_SetPinFunc(IOCON_PIO3_2, PIO3_2_FUN_PIO); //enable pin, ACTIVE LOW!
	GPIO_SetDir(PORT3, GPIO_Pin_2, 1); //output
	GPIO_ResetBits(PORT3, GPIO_Pin_2); //take low to disable outputs
#endif
}

void DCC_waveform_generation_start()
{
#if defined(__AVR__)
  //enable the compare match interrupt
  TIMSK1 |= (1<<OCIE1A);
#elif defined(LPC1769)
	//enable interrupt;
	MCPWM_IntConfig(LPC_MCPWM, MCPWM_INTFLAG_LIM2, ENABLE); //interrupt when timer reaches Limit.
	ENABLE_INTERRUPTS;
	//start activity
	//TODO this will cause problems later!
	MCPWM_Start(LPC_MCPWM, ENABLE, ENABLE, ENABLE);
#elif defined(LPC1343)
	ENABLE_INTERRUPTS;
	TIM_Cmd(LPC_TMR32B1, ENABLE);
#endif
}

#if defined(__AVR__)
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
      /// About to send a data uint8_t, but have to peceed the data with a '0'. Send that '0', then move to dos_send_byte
      case dos_send_bstart:
        OCR1A = OCR1B = zero_high_count;
        DCC_state = dos_send_byte;
        current_bit_counter = 8;
//        Serial.print(" 0 ");
        break;
      /// Sending a data uint8_t; current bit is tracked with current_bit_counter, and current byte with current_byte_counter
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
        if(!--current_bit_counter) //out of bits! time to either send a new uint8_t, or end the packet
        {
          if(!--current_byte_counter) //if not more uint8_ts, move to dos_end_bit
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

/// This is the Interrupt Service Routine all MCPWM interrupts.
#else

#if defined(LPC1769)
#define WRITE_ONE() do{chan2_setup.channelPeriodValue = one_count<<1; chan2_setup.channelPulsewidthValue = one_count;} while(0)
#define WRITE_ZERO_HIGH() do{chan2_setup.channelPeriodValue = zero_high_count<<1; chan2_setup.channelPulsewidthValue = zero_high_count;} while(0)
#define WRITE_ZERO_LOW() do{chan2_setup.channelPeriodValue = zero_low_count<<1; chan2_setup.channelPulsewidthValue = zero_low_count;} while(0)
#define COMMIT_WRITE() MCPWM_WriteToShadow(LPC_MCPWM, 2, &chan2_setup)
#define FINISH_IRQ() MCPWM_IntClear(LPC_MCPWM, MCPWM_INTFLAG_LIM2)
#elif defined(LPC1343)
#define WRITE_ONE() do{clock = one_count; timer32_1_chan0_setup.MatchValue = clock; TIM_ConfigMatch(LPC_TMR32B1, &timer32_1_chan0_setup);} while(0)
#define WRITE_ZERO_HIGH() do{clock = zero_high_count; timer32_1_chan0_setup.MatchValue = clock; TIM_ConfigMatch(LPC_TMR32B1, &timer32_1_chan0_setup);} while(0)
#define WRITE_ZERO_LOW() do{clock = zero_low_count; timer32_1_chan0_setup.MatchValue = clock; TIM_ConfigMatch(LPC_TMR32B1, &timer32_1_chan0_setup);} while(0)
#define COMMIT_WRITE() do{TIM_ConfigMatch(LPC_TMR32B1, &timer32_1_chan0_setup);} while(0)
#define FINISH_IRQ() TIM_ClearIntPending(LPC_TMR32B1, TIM_MR0_INT)
#endif

void TIMER32_1_IRQHandler(void) {
	//TODO handle zero-stretching

	DISABLE_INTERRUPTS;
#if defined(LPC1769)
	//first determine the nature of the interrupt.
	if(MCPWM_GetIntStatus(LPC_MCPWM, MCPWM_INTFLAG_LIM2) == SET)
	{
#elif defined(LPC1343)
	if(TIM_GetIntStatus(LPC_TMR32B1, TIM_MR0_INT) == SET)
	{
		TIM_ClearIntPending(LPC_TMR32B1, TIM_MR0_INT);
		//do stuff!
		LPC_GPIO1->DATA ^= 0x06; //toggle pins 1 and 2

		//TODO CHECK WHICH EDGE OF THE SIGNAL WE'RE ON!!!

#endif
		//if going LOW, switch out zero values.
		if (pin_flag) {
			if (clock == zero_high_count) {
				WRITE_ZERO_LOW();
			}
		}
		//if going HIGH, go on to the stuff below if LOW
		else {
			//First, toggle P1.2, which is under software control. P1.1 is under hardware control. Why can't we do both? TODO
			//GPIO_GetPointer(PORT1)->DATA ;
			//Timer has reached Limit. It will reset to 0 and start counting up again.
			// Our job is to determine the proper next value for Limit�and hence also for Match
			// This determination hinges on whether the next bit to be output is a 1 or a 0.
			switch (DCC_state) {
			/// Idle: Check if a new packet is ready. If it is, fall through to dos_send_premable. Otherwise just stick a '1' out there.
			case dos_idle:
				if (!current_byte_counter) //if no new packet
				{
					//          Serial.println("X");
					WRITE_ONE();
					//          OCR1A = OCR1B = one_count; //just send ones if we don't know what else to do. safe bet.
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
				WRITE_ONE();
				//OCR1A = OCR1B = one_count;
				//        Serial.print("P");
				if (!--current_bit_counter)
					DCC_state = dos_send_bstart;
				break;
				/// About to send a data uint8_t, but have to peceed the data with a '0'. Send that '0', then move to dos_send_byte
			case dos_send_bstart:
				WRITE_ZERO_HIGH();
				//OCR1A = OCR1B = zero_high_count;
				DCC_state = dos_send_byte;
				current_bit_counter = 8;
				//        Serial.print(" 0 ");
				break;
				/// Sending a data uint8_t; current bit is tracked with current_bit_counter, and current uint8_t with current_byte_counter
			case dos_send_byte:
				if (((current_packet[current_packet_size - current_byte_counter])
						>> (current_bit_counter - 1)) & 1) //is current bit a '1'?
				{
					WRITE_ONE();
					//OCR1A = OCR1B = one_count;
					//          Serial.print("1");
				} else //or is it a '0'
				{
					WRITE_ZERO_HIGH();
					//OCR1A = OCR1B = zero_high_count;
					//          Serial.print("0");
				}
				if (!--current_bit_counter) //out of bits! time to either send a new uint8_t, or end the packet
				{
					if (!--current_byte_counter) //if not more uint8_ts, move to dos_end_bit
					{
						DCC_state = dos_end_bit;
					} else //there are more uint8_ts�so, go back to dos_send_bstart
					{
						DCC_state = dos_send_bstart;
					}
				}
				break;
				/// Done with the packet. Send out a final '1', then head back to dos_idle to check for a new packet.
			case dos_end_bit:
				WRITE_ONE();
				//OCR1A = OCR1B = one_count;
				DCC_state = dos_idle;
				current_bit_counter = 14; //in preparation for a premable...
				//        Serial.println(" 1");
				break;
			}
			FINISH_IRQ(); //clear the interrupt first.
		}
	}

	pin_flag ^= 1; //flip bit so we know which edge we're on.
	ENABLE_INTERRUPTS;
}

#endif
