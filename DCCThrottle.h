/**
 *    CmdrArduino
 *
 *    Copyright © 2011 D.E. Goodman-Wilson
 *    
 *    CmdrArduino is a library for the Arduino platform for building DCC Command Stations.
 *    
 *    This file is part of CmdrArduino.
 *
 *    CmdrArduino is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    CmdrArduino is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with CmdrArduino.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef __DCCTHROTTLE_H__
#define __DCCTHROTTLE_H__

#include "DCCPacketScheduler.h"

#define DIRECTION_FORWARD 1
#define DIRECTION_REVERSE -1


/**
 *    
 *    DCCThrottle is a class for abstracting the particulars of a locomotive, train, or consist.
 *    It provides state information on the item's address, speed, direction, functions, and any
 *    consist it may belong to. This class is used as an itermediary between the Layout Control
 *    But (LCB) and the DCC outputs. The handling of translating LCB requests and commands into
 *    DCCThrottle objects is the purview of classed derived from the DCCThrottleManager abstract
 *    base class.
 *
 */
 
 class DCCThrottle
 {
   private:
    //char[16] name; //Yes, a string. Not all railroads restrict their road numbers to digits...
    unsigned int DCC_address;
    int speed; //this had better be enough speed steps. negative value = reverse; positive = forward.
    unsigned long functions; //bitfield of functions, with F0 at LSB, and F32 at MSB. Is this enough? Too much?
    //DCCThrottle *consist; //the consist this unit is attached to.
    
    //a helper function
    bool functionHelper(byte function_number);
        
   public:
    DCCThrottle(unsigned int address=0); //default address is a patently invalid address to indicate invalidity
    ~DCCThrottle();
    
    static DCCPacketScheduler *packet_scheduler; //all throttles will share the same packet scheduler
    
    bool setSpeed(int new_speed);
    bool setFunction(byte function_number);
    bool unsetFunction(byte function_number);
    bool opsModeProgramCV(unsigned int CV, byte CV_data);
    bool setAddress(unsigned int new_address);
    bool eStop(void);
    
    //functions whose implementation may depend on layout feedback devices!
    int getSpeed(void) {return speed;} //in a future version, this function will somehow involve RailCom
    bool isSetFunction(byte function_number) {return functions & ( 1<<function_number );}
    unsigned int getAddress(void) {return DCC_address;}
 };
 
 #endif __DCC_THROTTLE_H__