#include "DCCPacket.h"


DCCPacket::DCCPacket(unsigned int decoder_address) : address(decoder_address), size(1), kind(idle_packet_kind), repeat(0)
{
  data[0] = 0xFF; //default to idle packet
}

byte DCCPacket::getBitstream(byte rawbytes[]) //returns size of array.
{
  byte total_size = 1; //minimum size
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

void DCCPacket::addData(byte new_data[], byte new_size) //insert freeform data.
{
  for(int i = 0; i < new_size; ++i)
    data[i] = new_data[i];
  size = new_size;
}


/////////////////////////////////////

DCCAccessoryPacket::DCCAccessoryPacket(unsigned int address) : DCCPacket(address)
{
  kind = accessory_packet_kind;
}

byte DCCAccessoryPacket::getBitstream(byte rawbytes[])
{

  // Basic Accessory Packet looks like this:
  // {preamble} 0 10AAAAAA 0 1AAACDDD 0 EEEEEEEE 1
  // or this:
  // {preamble} 0 10AAAAAA 0 1AAACDDD 0 (1110CCVV 0 VVVVVVVV 0 DDDDDDDD) 0 EEEEEEEE 1 (if programming)

  rawbytes[0] = 0x80; //set up address byte 0
  rawbytes[1] = 0x80; //set up address byte 1

  //first, whittle address down to 9bits, and split address up among the first two bytes
  rawbytes[0] |= (address & 0x1FF) >> 3;
  rawbytes[1] |= (address & 0x07) << 4;

  //next, construct the function instruction byte. C=1; DDD = function
  rawbytes[1] |= (data[0] & 0x0F);
  
  //now, add any programming bytes (skipping first data byte, of course)
  byte i;
  byte total_size = 2;
  for(i = 1; i < size; ++i,++total_size)
  {
    rawbytes[total_size] = data[i];
  }
  
  //and, finally, the XOR
  byte XOR = 0;
  for(i = 0; i < total_size; ++i)
  {
    XOR ^= rawbytes[i];
  }
  rawbytes[total_size] = XOR;

  return total_size+1;
}

void DCCAccessoryPacket::setData(byte new_data)
{
  data[0] = new_data;
  size = 1;
}

/////////////////////////////////

DCCExtendedAccessoryPacket::DCCExtendedAccessoryPacket(unsigned int address) : DCCPacket(address)
{
  kind = accessory_packet_kind;
}

byte DCCExtendedAccessoryPacket::getBitstream(byte rawbytes[])
{
  // Extended Accessory Packet looks like this:
  // {preamble} 0 10AAAAAA 0 0AAA0AA1 0 000XXXXX 0 EEEEEEEE 1
  // and if programming, like this:
  // {preamble} 0 10AAAAAA 0 0AAA0AA1 0 (1110CCVV	0	VVVVVVVV	0	DDDDDDDD) 0 EEEEEEEE 1

  rawbytes[0] = 0x80;
  rawbytes[1] = 0x01;

  //first, whittle address down to 11bits, and split address up among the first two bytes
  rawbytes[0] |= (address & 0x7FF) >> 5;
  rawbytes[1] |= ( (address & 0x1C) << 2 ) | ( (address & 0x03) << 1 );
  
  //now, add any programming bytes (skipping first data byte, of course)
  byte i;
  byte total_size = 2;
  for(i = 0; i < size; ++i,++total_size)
  {
    rawbytes[total_size] = data[i];
  }
  
  //and, finally, the XOR
  byte XOR = 0;
  for(i = 0; i < total_size; ++i)
  {
    XOR ^= rawbytes[i];
  }
  rawbytes[total_size] = XOR;

  return total_size+1;
}

void DCCExtendedAccessoryPacket::setData(byte new_data)
{
  data[0] = new_data;
  size = 1;
}
