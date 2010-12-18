#ifndef __PACKETQUEUE_H__
#define __PACKETQUEUE_H__

/**
 * A FIFO queue for holding DCC packets, implemented as a circular buffer.
 * Copyright 2010 D.E. Goodman-Wilson
 * TODO
**/

#include "DCCPacket.h"

class PacketQueue
{
  protected:
    DCCPacket *queue;
    byte read_pos;
    byte write_pos;
    byte size;
    byte written; //how many cells have valid data? used for determining full status.  
  public:
    PacketQueue(byte length=10);
    
    virtual inline bool isFull(void)
    {
//      Serial.print("isFull: written=");
//      Serial.print(written,DEC);
//      Serial.print(" size=");
//      Serial.println(size,DEC);
      return (written == size);
    }
    virtual inline bool isEmpty(void)
    {
      return (written == 0);
    }
    
    virtual bool insertPacket(DCCPacket *packet); //makes a local copy, does not take over memory management!
    virtual bool readPacket(DCCPacket *packet); //does not hand off memory management of packet. used immediately.
};

class TemporalQueue: public PacketQueue
{
  protected:
    byte *age;
  public:
    TemporalQueue(byte length=10);
    inline bool isFull(void) { return false; }
    bool insertPacket(DCCPacket *packet);
    bool readPacket(DCCPacket *packet);
    bool forget(unsigned int address);
};

class RepeatQueue: public PacketQueue
{
  public:
    RepeatQueue(byte length=10);
    bool readPacket(DCCPacket *packet);
    bool forget(unsigned int address);
};

#endif //__PACKETQUEUE_H__