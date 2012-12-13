#include "DCCPacketQueue.h"

DCCPacketQueue::DCCPacketQueue(void) : read_pos(0), write_pos(0), size(10), written(0)
{
  return;
}

void DCCPacketQueue::setup(byte length)
{
  size = length;
  queue = (DCCPacket *)malloc(sizeof(DCCPacket) *size);
  for(int i = 0; i<size; ++i)
  {
    queue[i] = DCCPacket();
  }
}

bool DCCPacketQueue::insertPacket(DCCPacket *packet)
{
//  Serial.print("Enqueueing a packet of kind: ");
//  Serial.println(packet->getKind(), DEC);
   //First: Overwrite any packet with the same address and kind; if no such packet THEN hitup the packet at write_pos
  byte i = read_pos;
  while(i != (read_pos+written)%(size) )//(size+1) ) //size+1 so we can check the last slot, too…
  {
    if( (queue[i].getAddress() == packet->getAddress()) && (queue[i].getKind() == packet->getKind()))
    {
//      Serial.print("Overwriting existing packet at index ");
//      Serial.println(i, DEC);
      memcpy(&queue[i],packet,sizeof(DCCPacket));
      //do not increment written or modify write_pos
      return true;
    }
    i = (i+1)%size;
  }
  
  //else, tack it on to the end
  if(!isFull())
  {
    //else, just write it at the end of the queue.
    memcpy(&queue[write_pos],packet,sizeof(DCCPacket));
//    Serial.print("Write packet to index ");
//    Serial.println(write_pos, DEC);
    write_pos = (write_pos + 1) % size;
    ++written;
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
    memcpy(packet,&queue[read_pos],sizeof(DCCPacket));
    read_pos = (read_pos + 1) % size;
    --written;
    return true;
  }
  return false;
}

bool DCCPacketQueue::forget(uint16_t address, uint8_t address_kind)
{
  bool found = false;
  for(int i = 0; i < size; ++i)
  {
    if( (queue[i].getAddress() == address) && (queue[i].getAddressKind() == address_kind) )
    {
      found = true;
      queue[i] = DCCPacket(); //revert to default value
    }
  }
  --written;
  return found;
}

void DCCPacketQueue::clear(void)
{
  read_pos = 0;
  write_pos = 0;
  written = 0;
  for(int i = 0; i<size; ++i)
  {
    queue[i] = DCCPacket();
  }
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
    memcpy(packet,&queue[read_pos],sizeof(DCCPacket));
    read_pos = (read_pos + 1) % size;
    --written;

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
    queue[read_pos].setRepeat(queue[read_pos].getRepeat()-1); //decrement the current packet's repeat count
    if(queue[read_pos].getRepeat()) //if the topmost packet needs repeating
    {
      memcpy(packet,&queue[read_pos],sizeof(DCCPacket));
      return true;
    }
    else //the topmost packet is ready to be discarded; use the DCCPacketQueue mechanism
    {
      return(DCCPacketQueue::readPacket(packet));
    }
  }
  return false;
}
