#include "DCCPacketQueue.h"

DCCPacketQueue::DCCPacketQueue(void) : _read_pos(0), _write_pos(0), _written(0), _size(10)
{
  return;
}

void DCCPacketQueue::setup(byte length)
{
  _size = length;
  _queue = (DCCPacket *)malloc(sizeof(DCCPacket) *_size);
  for(int i = 0; i<_size; ++i)
  {
    _queue[i] = DCCPacket();
  }
}

bool DCCPacketQueue::insertPacket(DCCPacket *packet)
{
//  Serial.print("Enqueueing a packet of kind: ");
//  Serial.println(packet->getKind(), DEC);
   //First: Overwrite any packet with the same address and kind; if no such packet THEN hitup the packet at write_pos
  byte i = _read_pos;
  while(i != (_read_pos+_written)%(_size) )//(size+1) ) //size+1 so we can check the last slot, too…
  {
    if( (_queue[i].getAddress() == packet->getAddress()) && (_queue[i].getKind() == packet->getKind()))
    {
//      Serial.print("Overwriting existing packet at index ");
//      Serial.println(i, DEC);
      memcpy(&_queue[i],packet,sizeof(DCCPacket));
      //do not increment written or modify write_pos
      return true;
    }
    i = (i+1)%_size;
  }
  
  //else, tack it on to the end
  if(!isFull())
  {
    //else, just write it at the end of the queue.
    memcpy(&_queue[_write_pos],packet,sizeof(DCCPacket));
//    Serial.print("Write packet to index ");
//    Serial.println(write_pos, DEC);
    _write_pos = (_write_pos + 1) % _size;
    ++_written;
    return true;
  }
//  Serial.println("Queue is full!");
  return false;
}

// void DCCPacketQueue::printQueue(void)
// {
//   byte i, j;
//   for(i = 0; i < size; ++i)
//   {
//     for(j = 0; j < (queue[i].size_repeat>>4); ++j)
//     {
//       Serial.print(queue[i].data[j],BIN);
//       Serial.print(" ");
//     }
//     if(i == read_pos) Serial.println("   r");
//     else if(i == write_pos) Serial.println("    w");
//     else Serial.println("");
//   }
// }

bool DCCPacketQueue::readPacket(DCCPacket *packet)
{
  if(!isEmpty())
  {
//    Serial.print("Reading a packet from index: ");
//    Serial.println(read_pos, DEC);
    memcpy(packet,&_queue[_read_pos],sizeof(DCCPacket));
    _read_pos = (_read_pos + 1) % _size;
    --_written;
    return true;
  }
  return false;
}


/*****************************/


void DCCTemporalQueue::setup(byte length)
{
  DCCPacketQueue::setup(length);

  _age = (byte *)malloc(sizeof(byte)*_size);
  for(int i = 0; i<length; ++i)
  {
    _age[i] = 255;
  }
}

bool DCCTemporalQueue::insertPacket(DCCPacket *packet)
{
  //first, see if there is a packet to overwrite
  //otherwise find the oldest packet, and write over it.
  byte eldest = 0;
  byte eldest_idx = 0;
  bool updating = false;
  for(byte i = 0; i < _size; ++i)
  {
    if( (_queue[i].getAddress() == packet->getAddress()) && (_queue[i].getKind() == packet->getKind()) )
    {
      eldest_idx = i;
      updating = true;
      break; //short circuit this, we've found it;
    }
    else if(_age[i] > eldest)
    {
      eldest = _age[i];
      eldest_idx = i;
    }
  }

  
  memcpy(&_queue[eldest_idx],packet,sizeof(DCCPacket));
  _age[eldest_idx] = 0;
  //now, age the remaining packets in the queue.
  for(int i = 0; i < _size; ++i)
  {
    if(i != eldest_idx)
    {
      if(_age[i] < 255) //ceiling effect for age.
      {
        ++_age[i];
      }
    }
  }
  if(!updating)
    ++_written;
  return true;
}

bool DCCTemporalQueue::readPacket(DCCPacket *packet)
{
  if(!isEmpty()) //prevents the while loop below from running forever.
  {
    //valid packets will be found in index range [0,written);
    memcpy(packet,&_queue[_read_pos],sizeof(DCCPacket));
    _read_pos = (_read_pos + 1) % (_written);
    return true;
  }
  return false;
}

bool DCCTemporalQueue::forget(unsigned int address)
{
  bool found = false;
  for(int i = 0; i < _size; ++i)
  {
    if(_queue[i].getAddress() == address)
    {
      _queue[i] = DCCPacket(); //revert to default value
      _age[i] = 255; //mark it as really old.
    }
  }
  --_written;
  return found;
}


/*****************************/

DCCRepeatQueue::DCCRepeatQueue(void) : DCCPacketQueue()
{
}

bool DCCRepeatQueue::insertPacket(DCCPacket *packet)
{
  if(packet->getRepeat())
  {
    return(DCCPacketQueue::insertPacket(packet));
  }
  return false;
}

bool DCCRepeatQueue::readPacket(DCCPacket *packet)
{
  if(!isEmpty())
  {
    memcpy(packet,&_queue[_read_pos],sizeof(DCCPacket));
    _read_pos = (_read_pos + 1) % _size;
    --_written;

    if(packet->getRepeat()) //the packet needs to be sent out at least one more time
    {     
      packet->setRepeat(packet->getRepeat()-1);
      insertPacket(packet);
    }
    return true;
  }
  return false;
}


/**************/

DCCEmergencyQueue::DCCEmergencyQueue(void) : DCCPacketQueue()
{
}

/* Goes through each packet in the queue, repeats it getRepeat() times, and discards it */
bool DCCEmergencyQueue::readPacket(DCCPacket *packet)
{
  if(!isEmpty()) //anything in the queue?
  {
    _queue[_read_pos].setRepeat(_queue[_read_pos].getRepeat()-1); //decrement the current packet's repeat count
    if(_queue[_read_pos].getRepeat()) //if the topmost packet needs repeating
    {
      memcpy(packet,&_queue[_read_pos],sizeof(DCCPacket));
      return true;
    }
    else //the topmost packet is ready to be discarded; use the DCCPacketQueue mechanism
    {
      return(DCCPacketQueue::readPacket(packet));
    }
  }
  return false;
}
