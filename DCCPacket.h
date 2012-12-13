#ifndef __DCCPACKET_H__
#define __DCCPACKET_H__

#include "Arduino.h"

typedef unsigned char uint8_t;

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

#define MULTIFUNCTION_PACKET_KIND_MASK 0x10
#define idle_packet_kind            0x10
#define e_stop_packet_kind          0x11
#define speed_packet_kind           0x12
#define function_packet_1_kind      0x13
#define function_packet_2_kind      0x14
#define function_packet_3_kind      0x15
#define accessory_packet_kind       0x16
#define reset_packet_kind           0x17
#define ops_mode_programming_kind   0x18


#define ACCESSORY_PACKET_KIND_MASK 0x40
#define basic_accessory_packet_kind 0x40
#define extended_accessory_packet_kind 0x41

#define other_packet_kind           0x00

#define DCC_SHORT_ADDRESS           0x00
#define DCC_LONG_ADDRESS            0x01

class DCCPacket
{
  private:
   //A DCC packet is at most 6 uint8_ts: 2 of address, three of data, one of XOR
    uint16_t address;
    uint8_t address_kind;
    uint8_t data[3];
    uint8_t size_repeat;  //a bit field! 0b11000000 = 0xC0 = size; 0x00111111 = 0x3F = repeat
    uint8_t kind;
    
  public:
    DCCPacket(uint16_t decoder_address=0xFF, uint8_t decoder_address_kind=0x00);
    
    uint8_t getBitstream(uint8_t rawuint8_ts[]); //returns size of array.
    uint8_t getSize(void);
    inline uint16_t getAddress(void) { return address; }
    inline uint8_t getAddressKind(void) { return address_kind; }
    inline void setAddress(uint16_t new_address) { address = new_address; }
    inline void setAddress(uint16_t new_address, uint8_t new_address_kind) { address = new_address; address_kind = new_address_kind; }
    void addData(uint8_t new_data[], uint8_t new_size); //insert freeform data.
    inline void setKind(uint8_t new_kind) { kind = new_kind; }
    inline uint8_t getKind(void) { return kind; }
    inline void setRepeat(uint8_t new_repeat) { size_repeat = ((size_repeat&0xC0) | (new_repeat&0x3F)) ;}
    inline uint8_t getRepeat(void) { return size_repeat & 0x3F; }//return repeat; }
};

#endif //__DCCPACKET_H__
