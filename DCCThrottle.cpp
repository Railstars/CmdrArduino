/**
 *CmdrArduino
 *
 *Copyright © 2011 D.E. Goodman-Wilson
 *
 *CmdrArduino is a library for the Arduino platform for building DCC Command Stations.
 *
 *This file is part of CmdrArduino.
 *
 *CmdrArduino is free software: you can redistribute it and/or modify
 *it under the terms of the GNU Lesser General Public License as published by
 *the Free Software Foundation, either version 3 of the License, or
 *(at your option) any later version.
 *
 *CmdrArduino is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU Lesser General Public License for more details.
 *
 *You should have received a copy of the GNU Lesser General Public License
 *along with CmdrArduino.  If not, see <http://www.gnu.org/licenses/>.
 */
 

#include "DCCThrottle.h"


DCCThrottle::DCCThrottle(DCCPacketScheduler *scheduler, unsigned int address) : packet_scheduler(scheduler), DCC_address(address)
{
}

DCCThrottle::~DCCThrottle()
{
  //don't delete packet_scheduler; not up to us to maintain it!
}

bool DCCThrottle::setSpeed(int new_speed)
{
}

bool DCCThrottle::setFunction(byte function_number)
{
}

bool DCCThrottle::unsetFunction(byte function_number)
{
}

bool DCCThrottle::opsModeProgramCV(unsigned int CV, byte CV_data)
{
 //TODO should update address if address is programmed.
}

bool DCCThrottle::setAddress(unsigned int new_address)
{
}

bool DCCThrottle::eStop(void)
{
}
