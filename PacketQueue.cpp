#include "PacketQueue.h"

//    DCCPacket *queue; //an array of DCCPackets
//    byte read_pos;
//    byte write_pos;
//    byte size;
//    byte written; //how many cells have valid data? used for determining full status.

PacketQueue::PacketQueue(byte length) : read_pos(0), write_pos(0), written(0), size(length)
{
  queue = (DCCPacket *)malloc(sizeof(DCCPacket) *size);
  for(int i = 0; i<size; ++i)
  {
    queue[i] = DCCPacket();
  }
}

bool PacketQueue::insertPacket(DCCPacket *packet)
{
  //First: Overwrite any packet with the same address and kind; if no such packet THEN hitup the packet at write_pos
  for(byte i = read_pos; i <= read_pos+written; ++i)
  {
    byte queue_address = i%size;
    if( (queue[queue_address].getAddress() == packet->getAddress()) && (queue[queue_address].getKind() == packet->getKind()))
    {
      memcpy(&queue[queue_address],packet,sizeof(DCCPacket));
      //do not increment written!
      return true;
    }
  }
  if(!isFull())
  {
    //else, just write it at the end of the queue.
    memcpy(&queue[write_pos],packet,sizeof(DCCPacket));
    write_pos = (write_pos + 1) % size;
    ++written;
    return true;
  }
  return false;
}

bool PacketQueue::readPacket(DCCPacket *packet)
{
  if(!isEmpty())
  {
    memcpy(packet,&queue[read_pos],sizeof(DCCPacket));
    read_pos = (read_pos + 1) % size;
    --written;
    return true;
  }
  return false;
}


/*****************************/


TemporalQueue::TemporalQueue(byte length) : PacketQueue(length)
{
  age = (byte *)malloc(sizeof(byte)*length);
  for(int i = 0; i<length; ++i)
  {
    age[i] = 255;
  }
}

//TODO NEEDS FIXING!
bool TemporalQueue::insertPacket(DCCPacket *packet)
{
  //first, see if there is a packet to overwrite
  //otherwise find the oldest packet, and write over it.
  byte eldest = 0;
  byte eldest_idx = 0;
  for(byte i = 0; i < size; ++i)
  {
    if( (queue[i].getAddress() == packet->getAddress()) && (queue[i].getKind() == packet->getKind()) )
    {
      eldest_idx = i;
      break; //short circuit this, we've found it;
    }
    else if(age[i] > eldest)
    {
      eldest = age[i];
      eldest_idx = i;
    }
  }
  
  memcpy(&queue[eldest_idx],packet,sizeof(DCCPacket));
  age[eldest_idx] = 0;
  //now, age the remaining packets in the queue.
  for(int i = 0; i < size; ++i)
  {
    if(i != eldest_idx)
    {
      if(age[i] < 255) //ceiling effect for age.
      {
        ++age[i];
      }
    }
  }
  ++written; //this is not being updated correctly; current updates if overwriting old, valid packet, which is incorrect.
  return true;
}

//TODO this function isn't quite right, because written isn't being updated correctly, and so isEmpty() isn't going to work right.
bool TemporalQueue::readPacket(DCCPacket *packet)
{
  if(!isEmpty()) //prevents the while loop below from running forever.
  {
    //fast-foward past any idle packets (which act as placeholders)
    while(!queue[read_pos].getAddress())
      read_pos = (read_pos + 1) % size;
    memcpy(packet,&queue[read_pos],sizeof(DCCPacket));
    read_pos = (read_pos + 1) % size;
    return true;
  }
  return false;
}

bool TemporalQueue::forget(unsigned int address)
{
  bool found = false;
  for(int i = 0; i < size; ++i)
  {
    if(queue[i].getAddress() == address)
    {
      queue[i] = DCCPacket(); //revert to default value
      age[i] = 255; //mark it as really old.
    }
  }
  --written;
  return found;
}


/*****************************/

RepeatQueue::RepeatQueue(byte length) : PacketQueue(length)
{
}

//TODO NEEDS FIXING!
bool RepeatQueue::insertPacket(DCCPacket *packet)
{
  if(packet->getRepeat())
    return(PacketQueue::insertPacket(packet));
  return false;
}

bool RepeatQueue::readPacket(DCCPacket *packet)
{
  if(!isEmpty())
  {
    Serial.print("Not Empty! ");
    Serial.println(written,DEC);
    memcpy(packet,&queue[read_pos],sizeof(DCCPacket));
    //DCCPacket p;
    //memcpy(&p,&queue[read_pos],sizeof(DCCPacket));
    
    read_pos = (read_pos + 1) % size;
    --written;

    if(packet->getRepeat()) //the packet needs to be sent out at least one more time
    {     
      Serial.print("Repeating ");
      Serial.print(packet->getAddress(),DEC);
      Serial.print(" packet ");
      Serial.print(packet->getRepeat()-1,DEC);
      Serial.println(" more times.");
      packet->setRepeat(packet->getRepeat()-1);
      insertPacket(packet);
    }
    else
    {
      Serial.print("NOT repeating ");
      Serial.print("Repeating ");
      Serial.print(packet->getAddress(),DEC);
      Serial.println(" packet.");
    }
    return true;
  }
  else Serial.println("Queue is empty!");
  return false;
}