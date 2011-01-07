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
 
#ifndef __OPENLCBTHROTTLEMANAGER_H__
#define __OPENLCBTHROTTLEMANAGER_H__

#include "DCCThrottle.h"

class OpenLCBThrottleManager
{
  private:
    DCCThrottle *throttles; //an array of instances is a large hunk of contiguous memory! (even if it uses less total memory than an array of pointers)
    //OlcbNodeID *NIDs
  public:
    OpenLCBThrottleManager(byte size=10)
    {
      DCCThrottle = (DCCThrottle *)malloc(sizeof(DCCThrottle)*size);
      //how to ensure the constructors get called? //how to ensure we aren't running out of memory?
      //NIDs = (OlcbNodeID *)malloc(sizeof(OlcbNodeID)*size);
    };
    
    ~OpenLCBThrottleManager()
    {
      delete DCCThrottle;
      //delete NIDs;
    }
    
    //I have no idea how the remainder of this class should be structured yet. Wait and see.
        
};

#endif //__OPENLCBTHROTTLEMANAGER_H__
