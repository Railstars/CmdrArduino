#ifndef __DCCCOMMANDSTATION_H__
#define __DCCCOMMANDSTATION_H__
#include "DCCPacket.h"
#include "PacketQueue.h"
#include "WProgram.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define E_STOP_QUEUE_SIZE           2
#define HIGH_PRIORITY_QUEUE_SIZE    10
#define LOW_PRIORITY_QUEUE_SIZE     10
#define REPEAT_QUEUE_SIZE           10
#define PERIODIC_REFRESH_QUEUE_SIZE 10

#define LOW_PRIORITY_INTERVAL     5
#define REPEAT_INTERVAL           11
#define PERIODIC_REFRESH_INTERVAL 23

#define SPEED_REPEAT      3
#define FUNCTION_REPEAT   3
#define E_STOP_REPEAT     5
#define OPS_MODE_PROGRAMMING_REPEAT 3
#define OTHER_REPEAT      2

class DCCPacketScheduler
{
  public:
  
    DCCPacketScheduler(void);
    
    //for configuration
    void setDefaultSpeedSteps(byte new_speed_steps);
    void setup(void); //for any post-constructor initialization
    
    //for enqueueing packets
    bool setSpeed(unsigned int address,  char new_speed, byte steps = 0); //new_speed: [-127,127]
    bool setSpeed14(unsigned int address, char new_speed, bool F0=true); //new_speed: [-13,13], and optionally F0 settings.
    bool setSpeed28(unsigned int address, char new_speed); //new_speed: [-28,28]
    bool setSpeed128(unsigned int address, char new_speed); //new_speed: [-127,127]
    bool setFunctionG1(unsigned int address, byte functions);
    bool setFunctionG2(unsigned int address, byte functions);
    
    //the function methods are NOT stateful; you must specify all functions each time you call one
    //keeping track of function state is the responsibility of the calling program.
    bool setFunctions(unsigned int address, byte F0to4, byte F5to9=0x00, byte F9to12=0x00);
    bool setFunctions0to4(unsigned int address, byte functions);
    bool setFunctions5to8(unsigned int address, byte functions);
    bool setFunctions9to12(unsigned int address, byte functions);
    //other cool functions to follow. Just get these working first, I think.
    
    bool setTurnout(unsigned int address);
    bool unsetTurnout(unsigned int address);
    
    bool opsProgramCV(unsigned int address, unsigned int CV, byte CV_data);

    //more specific functions
    bool eStop(void); //all locos
    bool eStop(unsigned int address); //just one specific loco
    
    //to be called periodically within loop()
    void update(void); //checks queues, puts whatever's pending on the rails via global current_packet. easy-peasy

  //private:
  
  //  void stashAddress(DCCPacket *p); //remember the address to compare with the next packet
    void repeatPacket(DCCPacket *p); //insert into the appropriate repeat queue
    byte default_speed_steps;
    unsigned int last_packet_address;
  
    byte packet_counter;
    
    EmergencyQueue e_stop_queue;
    PacketQueue high_priority_queue;
    PacketQueue low_priority_queue;
    RepeatQueue repeat_queue;
    TemporalQueue periodic_refresh_queue;
    
    //TODO to be completed later.
    //DCC_Packet ops_programming_queue[10];
    
    //some handy thingers
    DCCPacket idle_packet;
};

//DCCPacketScheduler packet_scheduler;

#endif //__DCC_COMMANDSTATION_H__
