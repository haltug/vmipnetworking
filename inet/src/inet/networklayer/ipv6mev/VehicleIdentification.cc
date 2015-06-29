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
#include "inet/networklayer/ipv6mev/VehicleIdentification.h"

namespace inet {

bool VehicleIdentification::tryParse(const char *addr)
{
    if (!addr)
        return false;

    int numHexDigits = 0;
    for (const char *s = addr; *s; s++) // works if input string is null-terminated
    {
        if (isxdigit(*s) && numHexDigits <= ID_SIZE)
            numHexDigits++;
        else
            if(numHexDigits == (ID_SIZE/2) && *s == ':')
                numHexDigits++;
            else
                return false;
    }
    return true;
}



} /* namespace inet */
