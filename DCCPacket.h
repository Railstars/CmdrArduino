#ifndef __DCCPACKET_H__
#define __DCCPACKET_H__

#include "WProgram.h"

typedef unsigned char byte;

//Packet kinds
enum packet_kind_t {
  idle_packet_kind,
  e_stop_packet_kind,
  speed_packet_kind,
  function_packet_kind,
  accessory_packet_kind,
  reset_packet_kind,
  ops_mode_programming_kind,
  other_packet_kind
};

class DCCPacket
{
  public:
   //A DCC packet is at most 6 bytes: 2 of address, three of data, one of XOR
    unsigned int address;// = 0;
    byte data[3];// = {0,0,0};
    byte size;// = 3; //of data only
//    byte XOR;// = 0;
    byte repeat;
    packet_kind_t kind;
    
//    void computeXOR(void);
  public:
    DCCPacket(unsigned int address=0);
    
    byte getBitstream(byte rawbytes[]); //returns size of array.
    byte getSize(void);
    inline unsigned int getAddress(void) { return address; }
    inline void setAddress(unsigned int new_address) { address = new_address; }
    byte addData(byte new_data[], byte new_size); //insert freeform data.
    inline void setKind(packet_kind_t new_kind) { kind = new_kind; }
    inline packet_kind_t getKind(void) { return kind; }
    inline void setRepeat(byte new_repeat) { repeat = new_repeat; }
    inline byte getRepeat(void) { return repeat; }
};

#endif //__DCCPACKET_H__
