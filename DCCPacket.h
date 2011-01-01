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
  other_packet_kind
};

class DCCPacket
{
  public: //protected:
   //A DCC packet is at most 6 bytes: 2 of address, three of data, one of XOR
    unsigned int address;
    byte data[3];
    byte size; //of data only, not entire packet
    byte repeat; //number of times to repeat
    packet_kind_t kind; //kind of packet
    
  public:
    DCCPacket(unsigned int address=0);
    
    virtual byte getBitstream(byte rawbytes[]); //returns size of array.
    inline byte getSize(void) { return size; }
    inline unsigned int getAddress(void) { return address; }
    inline void setAddress(unsigned int new_address) { address = new_address; }
    void addData(byte new_data[], byte new_size); //insert freeform data.
    inline void setKind(packet_kind_t new_kind) { kind = new_kind; }
    inline packet_kind_t getKind(void) { return kind; }
    inline void setRepeat(byte new_repeat) { repeat = new_repeat; }
    inline byte getRepeat(void) { return repeat; }
};

class DCCAccessoryPacket : public DCCPacket
{
  public:
    DCCAccessoryPacket(unsigned int address=0);
    byte getBitstream(byte rawbytes[]);
    void addData(byte new_data);
};

class DCCExtendedAccessoryPacket : public DCCPacket
{
  public:
    DCCExtendedAccessoryPacket(unsigned int address=0);
    byte getBitstream(byte rawbytes[]);
    void addData(byte new_data);
};

#endif //__DCCPACKET_H__
