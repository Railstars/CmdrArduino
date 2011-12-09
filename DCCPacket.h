/**
 * @file    DCCPacket.h
 * @author  D.E. Goodman-Wilson <dgoodman@railstars.com>
 * @version 1.0alpha
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * The DCCPacket class represents a single DCC packet, which encodes a single DCC instruction.
 */

#ifndef __DCCPACKET_H__
#define __DCCPACKET_H__

#include "Arduino.h"

typedef uint8_t byte;

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
  public:
    /**
     * Constructor, taking a particular decoder address
     * @param decoder_address Decoder to address; 0xFF = broadcast
     */
    DCCPacket(unsigned int decoder_address=0xFF);
    
    /**
     * Converts the object into a sequence of bits formatted according to NMRA S 9.2, include the error-checking
     * @param rawbytes an array of bytes to write the bitstream into. Must be large enough to hold an entire packet, ideally TODO bytes
     * @return the size of the array actually used.
     */
    byte getBitstream(byte rawbytes[]); //returns size of array.

    /**
     * Determine the size of the data payload (exclusive of header and error-check byte) in bytes
     * @return
     */
    byte getSize(void);

    /**
     * Returns the decoder ID the packet is addressed to.
     * @return the packet address
     */
    inline unsigned int getAddress(void) { return _address; }

    /**
     * Sets the decoder ID the packet is addressed to.
     * @param new_address the new packet address
     */
    inline void setAddress(unsigned int new_address) { _address = new_address; }

    /**
     * Add unformatted data to the packet; overwrites existing data, if any.
     * @param new_data an array of bytes to put in the packet payload
     * @param new_size the size of the array in bytes
     */
    void addData(byte new_data[], byte new_size);

    /**
     * Set the packet kind
     * @param new_kind see the defines above; must be one of:
     *   -idle              #idle_packet_kind          (0)
     *   -emergency stop    #e_stop_packet_kind        (1)
     *   -speed update      #speed_packet_kind         (2)
     *   -function group 1  #function_packet_1_kind    (3)
     *   -function group 2  #function_packet_2_kind    (4)
     *   -function group 3  #function_packet_3_kind    (5)
     *   -accessory command #accessory_packet_kind     (6)
     *   -reset             #reset_packet_kind         (7)
     *   -ops programming   #ops_mode_programming_kind (8)
     *   -other             #other_packet_kind         (9)
     */
    inline void setKind(byte new_kind) { _kind = new_kind; }

    /**
     * Returns the packet kind
     * @return returns the packet kind
     * @see setKind(new_kind)
     */
    inline byte getKind(void) { return _kind; }

    /**
     * Set the number of time to repeat this packet
     * @param new_repeat the number of times to repeat; max is 63.
     */
    inline void setRepeat(byte new_repeat) { _size_repeat = ( (_size_repeat&0xC0) | (new_repeat&0x3F) ) ;}

    /**
     * Get the number of times this packet will repeat
     * @return the number of times the packet will repeat.
     */
    inline byte getRepeat(void) { return _size_repeat & 0x3F; }//return repeat; }
  private:
   //A DCC packet is at most 6 bytes: 2 of address, three of data, one of XOR
    unsigned int _address;
    byte _data[3];
    byte _kind;
    byte _size_repeat;  //a bit field! 0b11000000 = 0xC0 = size; 0x00111111 = 0x3F = repeat
};

#endif //__DCCPACKET_H__
