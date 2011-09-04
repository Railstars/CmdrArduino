#include "DCCPacket.h"


DCCPacket::DCCPacket(unsigned int decoder_address) : _address(decoder_address), _kind(idle_packet_kind), _size_repeat(0x40) //size(1), repeat(0)
{
  _data[0] = 0x00; //default to idle packet
  _data[1] = 0x00;
  _data[2] = 0x00;
}

byte DCCPacket::getBitstream(byte rawbytes[]) //returns size of array.
{
  int total_size = 1; //minimum size
  
  if(_kind == idle_packet_kind)  //idle packets work a bit differently:
  // since the "address" field is 0xFF, the logic below will produce C0 FF 00 3F instead of FF 00 FF
  {
    rawbytes[0] = 0xFF;
  }
  else if(_address <= 127) //addresses of 127 or higher are 14-bit "extended" addresses
  {
    rawbytes[0] = (byte)_address;
  }
  else //we have a 14-bit extended ("4-digit") address
  {
    rawbytes[0] = (byte)((_address >> 8)|0xC0);
    rawbytes[1] = (byte)(_address & 0xFF);
    ++total_size;
  }
  
  byte i;
  for(i = 0; i < (_size_repeat>>6); ++i,++total_size)
  {
    rawbytes[total_size] = _data[i];
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
  return (_size_repeat>>6);
}

void DCCPacket::addData(byte new_data[], byte new_size) //insert freeform data.
{
  for(int i = 0; i < new_size; ++i)
    _data[i] = new_data[i];
  _size_repeat = (_size_repeat & 0x3F) | (new_size<<6);
}
