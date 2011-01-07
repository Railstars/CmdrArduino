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
 

#include "DCCThrottle.h"

DCCThrottle::DCCThrottle(unsigned int address) : DCC_address(address)
{
}

DCCThrottle::~DCCThrottle()
{
}

bool DCCThrottle::setSpeed(int new_speed)
{
  speed = new_speed;
  //speed is an int; DCCPacketScheduler::setSpeed expects a char. so just slice off the 8 LS bits.
  //it's a hack, but hey, it works, and it's reasonably fast.
  return(packet_scheduler->setSpeed(DCC_address,speed>>8));
}

bool DCCThrottle::functionHelper(byte function_number)
{
  //depending on the value of function_number, there are different methods to call…
  if(function_number <= 5)
    return(packet_scheduler->setFunctions0to4(DCC_address, functions & 0x1F));
  else if (function_number <= 8)
    return(packet_scheduler->setFunctions5to8(DCC_address, (functions >> 5)&0x0F));
  else if(function_number <= 12)
    return(packet_scheduler->setFunctions9to12(DCC_address, (functions >> 9)&0x0F));
  //TODO more function methods, please!
}

bool DCCThrottle::setFunction(byte function_number)
{
  if(function_number <= 32)
  {
    functions |= (1 << function_number);
    return(functionHelper(function_number));
  }
  return false;
}

bool DCCThrottle::unsetFunction(byte function_number)
{
  if(function_number <= 32)
  {
    functions &= (0 << function_number);
    return(functionHelper(function_number));
  }
  return false;
}

bool DCCThrottle::opsModeProgramCV(unsigned int CV, byte CV_data)
{
  return(packet_scheduler->opsProgramCV(DCC_address, CV, CV_data));
  //TODO should update address if address is programmed.
}

bool DCCThrottle::setAddress(unsigned int new_address)
{
  if(new_address == DCC_address)
    return true;
    
  DCC_address = new_address;
  //TODO should call opsModeProgramCV to update address at loco! maybe switching CV29 to reflect 2- or 4-digit addressing
}

bool DCCThrottle::eStop(void)
{
  return(packet_scheduler->eStop(DCC_address));
}
