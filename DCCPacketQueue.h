/**
 * @file    DCCPacketQueue.h
 * @author  D.E. Goodman-Wilson <dgoodman@railstars.com>
 * @version 1.0alpha
 * @copyright ©2010-2011 D.E. Goodman-Wilson <dgoodman@railstars.com>
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This file implements a few different FIFO queues for holding DCC packets,
 * implemented as circular buffers.
 *
 * None of these classes need to be dealt with directly in application code; these
 * are only used by the scheduler.
 */

#ifndef __DCCPACKETQUEUE_H__
#define __DCCPACKETQUEUE_H__

#include "WProgram.h"
#include "DCCPacket.h"

/**
 * @class DCCPacketQueue
 * An abstract base class for implementing packet queues.
 */
class DCCPacketQueue
{
  public:
    /**
     * The constructor. Does nothing really.
     */
    DCCPacketQueue(void);
    
    /**
     * This should be the constructor, but it's not. Allocates the DCC queue
     * @param size the size of the queue in packets
     */
    virtual void setup(byte size);
    
    /**
     * The destructor. Should never be called. Frees the queue.
     */
    ~DCCPacketQueue(void)
    {
      free(_queue);
    }
    
    /**
     * Is the queue full?
     * @return true = full; false = not full
     */
    virtual inline bool isFull(void)
    {
      return (_written == _size);
    }

    /**
     * Is the queue empty?
     * @return true = empty; false = not empty
     */
    virtual inline bool isEmpty(void)
    {
      return (_written == 0);
    }

    /**
     * Is the queue not empty? Why do I have this method, really?
     * @return true = not empty; false = empty
     */
    virtual inline bool notEmpty(void)
    {
      return (_written > 0);
    }
    
    /**
     * Required by the scheduler. Returns true if the next queued packet DOES NOT have the specified address
     * @param address the address to compare against
     * @return true = next packet has not the address; false = the next packet does have the address
     */
    virtual inline bool notRepeat(unsigned int address)
    {
      return (address != _queue[_read_pos].getAddress());
    }
    
    /**
     * Insert a new packet into the queue. Makes a local copy, does not take over memory management of the packet!
     * @param packet the packet to insert; is copied in full, can be safely deleted after this call
     * @return true = success; false = failure (queue full, most likely)
     */
    virtual bool insertPacket(DCCPacket *packet);

    /**
     * Pops the topmost packet out of the queue, copying it into the pointer provided.
     * @param packet a pointer to have the packet copied into
     * @return true = success; false = failure (empty queue)
     */
    virtual bool readPacket(DCCPacket *packet);

  protected:
    DCCPacket *_queue;
    byte _read_pos;
    byte _write_pos;
    byte _written; //how many cells have valid data? used for determining full status.
    byte _size;
};

/**
 * @class DCCTemporalQueue
 * A queue that, instead of writing new packets to the end of the queue, simply overwrites the oldest packet in the queue.
 */

class DCCTemporalQueue: public DCCPacketQueue
{
  public:
    DCCTemporalQueue(void) : DCCPacketQueue() {};
    void setup(byte length);
    /**
     * DCCTemporalQueues are never full! There is always an oldest packet to overwrite
     * @return false, always
     */
    inline bool isFull(void) { return false; }
    bool insertPacket(DCCPacket *packet);
    bool readPacket(DCCPacket *packet);
    /**
     * Forget all packets associated with a particular address, drop them from the queue.
     * @param address the address associated with the packets to drop
     * @return true = success; false = failure (no such packets to drop or queue empty)
     */
    bool forget(unsigned int address);
  protected:
    byte *_age;
};

/**
 * @class DCCRepeatQueue
 * A queue that, when a packet is read, puts that packet back in the queue if it requires repeating.
 */
class DCCRepeatQueue: public DCCPacketQueue
{
  public:
    DCCRepeatQueue(void);
    //void setup(byte length);
    bool insertPacket(DCCPacket *packet);
    bool readPacket(DCCPacket *packet);
    bool forget(unsigned int address);
};

/**
 * @class DCCEmergencyQueue
 * A queue that repeats the topmost packet as many times as is indicated by the packet before moving on
 */
class DCCEmergencyQueue: public DCCPacketQueue
{
  public:
    DCCEmergencyQueue(void);
    bool readPacket(DCCPacket *packet);
};

#endif //__DCCPACKETQUEUE_H__
