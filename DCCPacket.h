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
    inline unsigned int getAddress(void) { return address; }
    inline void setAddress(unsigned int new_address) { address = new_address; }
    uint8_t addData(uint8_t new_data[], uint8_t new_size); //insert freeform data.
    inline void setKind(uint8_t new_kind) { kind = new_kind; }
    inline uint8_t getKind(void) { return kind; }
    inline void setRepeat(uint8_t new_repeat) { size_repeat = ((size_repeat&0xC0) | (new_repeat&0x3F)) ;}
    inline uint8_t getRepeat(void) { return size_repeat & 0x3F; }//return repeat; }
};

#endif //__DCCPACKET_H__
