#ifndef __DCCPACKET_H__
#define __DCCPACKET_H__

#include "WProgram.h"

typedef unsigned char byte;

//Packet kinds
// enum packet_kind_t {
//   idle_packet_kind,
//   e_stop_packet_kind,
//   speed_packet_kind,
//   function_packet_kind,
//   accessory_packet_kind,
//   reset_packet_kind,
//   ops_mode_programming_kind,
//   other_packet_kind
// };

#define idle_packet_kind            0
#define e_stop_packet_kind          1
#define speed_packet_kind           2
#define function_packet_1_kind        3
#define function_packet_2_kind        4
#define function_packet_3_kind        5
#define accessory_packet_kind       6
#define reset_packet_kind           7
#define ops_mode_programming_kind   8
#define other_packet_kind           9

#define BASIC_ACCESSORY_BROADCAST 0x1F8
#define EXTENDED_ACCESSORY_BROADCAST 0x7E3

class DCCPacket
{
  private:
   //A DCC packet is at most 6 bytes: 2 of address, three of data, one of XOR
    unsigned int address;
    byte data[3];
    byte size_repeat;  //a bit field! 0b11000000 = 0xC0 = size; 0x00111111 = 0x3F = repeat
    byte kind;
    
  public:
    DCCPacket(unsigned int decoder_address=0xFF);
    
    virtual byte getBitstream(byte rawbytes[]); //returns size of array.
    inline byte getSize(void) { return size; }
    inline unsigned int getAddress(void) { return address; }
    inline void setAddress(unsigned int new_address) { address = new_address; }
    byte addData(byte new_data[], byte new_size); //insert freeform data.
    inline void setKind(byte new_kind) { kind = new_kind; }
    inline byte getKind(void) { return kind; }
    inline void setRepeat(byte new_repeat) { size_repeat = (size_repeat&0xC0 | new_repeat&0x3F) ;}
    inline byte getRepeat(void) { return size_repeat & 0x3F; }//return repeat; }
};

class DCCAccessoryPacket : public DCCPacket
{
  public:
    DCCAccessoryPacket(unsigned int address=BASIC_ACCESSORY_BROADCAST);
    byte getBitstream(byte rawbytes[]);
    void setData(byte new_data);
};

class DCCExtendedAccessoryPacket : public DCCPacket
{
  public:
    DCCExtendedAccessoryPacket(unsigned int address=EXTENDED_ACCESSORY_BROADCAST);
    byte getBitstream(byte rawbytes[]);
    void setData(byte new_data);
};

#endif //__DCCPACKET_H__
