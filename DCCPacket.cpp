#include "DCCPacket.h"


DCCPacket::DCCPacket(uint16_t new_address, uint8_t new_address_kind) : kind(idle_packet_kind), size_repeat(0x40) //size(1), repeat(0)
{
  address = new_address;
  address_kind = new_address_kind;
  data[0] = 0x00; //default to idle packet
  data[1] = 0x00;
  data[2] = 0x00;
}

uint8_t DCCPacket::getBitstream(uint8_t rawuint8_ts[]) //returns size of array.
{
  int total_size = 1; //minimum size
  
  if(kind == idle_packet_kind)  //idle packets work a bit differently:
  // since the "address" field is 0xFF, the logic below will produce C0 FF 00 3F instead of FF 00 FF
  {
    rawuint8_ts[0] = 0xFF;
  }
  else if(!address_kind) // short address
  {
    rawuint8_ts[0] = (uint8_t)address;
  }
  else //we have a 14-bit extended ("4-digit") address
  {
    rawuint8_ts[0] = (uint8_t)((address >> 8)|0xC0);
    rawuint8_ts[1] = (uint8_t)(address & 0xFF);
    ++total_size;
  }
  
  uint8_t i;
  for(i = 0; i < (size_repeat>>6); ++i,++total_size)
  {
    rawuint8_ts[total_size] = data[i];
  }

  uint8_t XOR = 0;
  for(i = 0; i < total_size; ++i)
  {
    XOR ^= rawuint8_ts[i];
  }
  rawuint8_ts[total_size] = XOR;

  return total_size+1;  
}

uint8_t DCCPacket::getSize(void)
{
  return (size_repeat>>6);
}

uint8_t DCCPacket::addData(uint8_t new_data[], uint8_t new_size) //insert freeform data.
{
  for(int i = 0; i < new_size; ++i)
    data[i] = new_data[i];
  size_repeat = (size_repeat & 0x3F) | (new_size<<6);
}