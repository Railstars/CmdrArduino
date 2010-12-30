#include "DCCPacket.h"


DCCPacket::DCCPacket(unsigned int decoder_address) : address(decoder_address), size(1), kind(idle_packet_kind), repeat(0)
{
  data[0] = 0xFF; //default to idle packet
}

byte DCCPacket::getBitstream(byte rawbytes[]) //returns size of array.
{
  int total_size = 1; //minimum size
  if(address <= 127) //addresses of 127 or higher are 14-bit "extended" addresses
  {
    rawbytes[0] = (byte)address;
  }
  else //we have a 14-bit extended ("4-digit") address
  {
    rawbytes[0] = (byte)((address >> 8)|0xC0);
    rawbytes[1] = (byte)(address & 0xFF);
    ++total_size;
  }
  
  byte i;
  for(i = 0; i < size; ++i,++total_size)
  {
    rawbytes[total_size] = data[i];
  }

  byte XOR = 0;
  for(i = 0; i < total_size; ++i)
  {
    XOR ^= rawbytes[i];
  }
  rawbytes[total_size] = XOR;

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