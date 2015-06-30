//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 
#include <ctype.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include "inet/networklayer/ipv6mev/VehicleIdentification.h"

namespace inet {

static const int ID_SIZE = 16;

const VehicleIdentification VehicleIdentification::UNSPECIFIED_ADDRESS;

VehicleIdentification::VehicleIdentification(int rnd, int seed)
{
    if(rnd > 0)
    {
        srand(seed);
        id = (uint64) rand();
    }
    else
    {
        id = (uint64) rand();
    }
}

bool VehicleIdentification::tryParse(const char *addr)
{
    if (!addr)
        return false;

    int numHexDigits = 0;
    for (const char *s = addr; *s; s++) // works if input string is null-terminated
    {
        if (isxdigit(*s) && numHexDigits <= ID_SIZE)
        {
            numHexDigits++;
        }
        else
        {
            if(numHexDigits == (ID_SIZE/2) && *s == ':')
                numHexDigits++;
            else
                return false;
        }
    }
    return true;
}

void VehicleIdentification::setId(const char* hexstr)
{
    if(hexstr != nullptr)
    {
        if(*(hexstr+8) == ':')
        {
//            const char* cpy = hexstr;  // an alias to iterate through s without moving s
//            const char* temp = hexstr;
//            while (*cpy)
//            {
//                if (*cpy != ':')
//                    *temp++ = *cpy;
//                cpy++;
//            }
//            *temp = 0;
//            id = strtoull(hexstr, NULL, 16);
            std::string str(hexstr);
            std::string str2(hexstr);
            str.replace(8,1,str2,9,8);
            id = std::stoull(str,NULL,16);
        }
        else
        {
            id = strtoull(hexstr, NULL, 16);
        }
    }
}

std::string VehicleIdentification::str() const
{
    if(isUnspecified())
        return std::string("DEADAFFE:DEADAFFE");

    char buf[ID_SIZE+1];
    sprintf(buf, "%016llX", id);
    std::string str = std::string(buf);
    str.insert(ID_SIZE/2, ":");
    str.insert(str.end(),'\0');
    return str;
}

std::string VehicleIdentification::get_str() const
{
    std::ostringstream s;
    s << id;
    return s.str();
}

} /* namespace inet */
