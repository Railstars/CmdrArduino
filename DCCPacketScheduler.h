#ifndef __DCCCOMMANDSTATION_H__
#define __DCCCOMMANDSTATION_H__
#include "DCCPacket.h"
#include "PacketQueue.h"
#include "WProgram.h"

#define E_STOP_QUEUE_SIZE           2
#define HIGH_PRIORITY_QUEUE_SIZE    10
#define LOW_PRIORITY_QUEUE_SIZE     10
#define REPEAT_QUEUE_SIZE           10
#define PERIODIC_REFRESH_QUEUE_SIZE 10

#define LOW_PRIORITY_INTERVAL     5
#define REPEAT_INTERVAL           11
#define PERIODIC_REFRESH_INTERVAL 23

class DCCPacketScheduler
{
  private:
    void stashAddress(DCCPacket *p); //remember the address to compare with the next packet
    void repeatPacket(DCCPacket *p); //insert into the appropriate repeat queue
    byte default_speed_steps;
    unsigned int last_packet_address;
  public:
  
    DCCPacketScheduler(void);
    
    //for configuration
    void setDefaultSpeedSteps(byte new_speed_steps);
    void setup(void); //for any post-constructor initialization
    
    //for enqueueing packets
    bool setSpeed(unsigned int address,  char new_speed, byte steps = 0); //new_speed: [-127,127]
    bool setSpeed14(unsigned int address, char new_speed); //new_speed: [-13,13]
    bool setSpeed28(unsigned int address, char new_speed); //new_speed: [-28,28]
    bool setSpeed128(unsigned int address, char new_speed); //new_speed: [-127,127]
    bool setFunction(unsigned int address, byte function);
    bool unsetFunction(unsigned int address, byte function);
    //other cool functions to follow. Just get these working first, I think.
    
    //bool setTurnout(unsigned int address);
    //bool unsetTurnout(unsigned int address);
    
    //void programCV(unsigned int address, byte CV, byte data);
        
    //more specific functions
    bool eStop(void); //all locos
    bool eStop(unsigned int address); //just one specific loco
    
    //to be called periodically within loop()
    void update(void); //checks queues, puts whatever's pending on the rails via global current_packet. easy-peasy

  private:
  
    byte packet_counter;
    
    PacketQueue e_stop_queue;
    PacketQueue high_priority_queue;
    PacketQueue low_priority_queue;
    RepeatQueue repeat_queue;
    TemporalQueue periodic_refresh_queue;
    
    //TODO to be completed later.
    //DCC_Packet ops_programming_queue[10];

    byte speed_steps; //=SPEED_STEPS_128
};

#endif //__DCC_COMMANDSTATION_H__