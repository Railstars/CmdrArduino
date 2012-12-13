#ifndef __DCCPACKETQUEUE_H__
#define __DCCPACKETQUEUE_H__

#include "Arduino.h"

/**
 * A FIFO queue for holding DCC packets, implemented as a circular buffer.
 * Copyright 2010 D.E. Goodman-Wilson
 * TODO
**/

#include "DCCPacket.h"

class DCCPacketQueue
{
  public: //protected:
    DCCPacket *queue;
    byte read_pos;
    byte write_pos;
    byte size;
    byte written; //how many cells have valid data? used for determining full status.  
  public:
    DCCPacketQueue(void);
    
    virtual void setup(byte);
    
    ~DCCPacketQueue(void)
    {
      free(queue);
    }
    
    virtual inline bool isFull(void)
    {
      return (written == size);
    }
    virtual inline bool isEmpty(void)
    {
      return (written == 0);
    }
    virtual inline bool notEmpty(void)
    {
      return (written > 0);
    }
    
    virtual inline bool notRepeat(unsigned int address)
    {
      return (address != queue[read_pos].getAddress());
    }
    
    //void printQueue(void);
    
    virtual bool insertPacket(DCCPacket *packet); //makes a local copy, does not take over memory management!
    virtual bool readPacket(DCCPacket *packet); //does not hand off memory management of packet. used immediately.
    
    bool forget(uint16_t address, uint8_t address_kind);
    void clear(void);
};

//A queue that, when a packet is read, puts that packet back in the queue if it requires repeating.
class DCCRepeatQueue: public DCCPacketQueue
{
  public:
    DCCRepeatQueue(void);
    //void setup(byte length);
    bool insertPacket(DCCPacket *packet);
    bool readPacket(DCCPacket *packet);
};

//A queue that repeats the topmost packet as many times as is indicated by the packet before moving on
class DCCEmergencyQueue: public DCCPacketQueue
{
  public:
    DCCEmergencyQueue(void);
    bool readPacket(DCCPacket *packet);
};

#endif //__DCCPACKETQUEUE_H__
