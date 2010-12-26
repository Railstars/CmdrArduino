#include "DCCPacket.h"


DCCPacket::DCCPacket(unsigned int decoder_address) : address(decoder_address), size(1), kind(idle_packet_kind), repeat(0)
{
  data[0] = 0xFF; //default to idle packet
}

byte DCCPacket::getBitstream(byte rawbytes[]) //returns size of array.
{
  int total_size = 1; //minimum size
  if(address < 127) //TODO IS THIS RIGHT?
  {
    rawbytes[0] = (byte)address;
    //Serial.println(rawbytes[0],BIN);
  }
  else //ELSE WHAT?
  {
    rawbytes[0] = (byte)address >> 8;
    rawbytes[1] = (byte)(address & 0xFF);
    //Serial.print(rawbytes[0],BIN);
    //Serial.print(" ");
    //Serial.println(rawbytes[1],BIN);
    ++total_size;
  }
  int i;
  //Serial.print("Payload:");
  for(i = 0; i < size; ++i,++total_size)
  {
    rawbytes[total_size] = data[i];
    //Serial.print(" ");
    //Serial.print(rawbytes[total_size],BIN);
  }
  //Serial.println("");
  //Serial.print("XOR: ");
  //Serial.print(total_size);
  //Serial.print(" ");
  byte XOR = 0;
  for(i = 0; i < total_size; ++i)
  {
    XOR ^= rawbytes[i];
  }
  rawbytes[total_size] = XOR;
  //Serial.println(rawbytes[total_size],BIN);

  return total_size+1;  
}

byte DCCPacket::getSize(void)
{
  return size;
}

byte DCCPacket::addData(byte new_data[], byte new_size) //insert freeform data.
{
  for(int i = 0; i < new_size; ++i)
    data[i] = new_data[i];
  size = new_size;
}