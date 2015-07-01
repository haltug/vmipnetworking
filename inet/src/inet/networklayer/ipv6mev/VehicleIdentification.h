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
//#include <crng.h>

#ifndef VEHICLEIDENTIFICATION_H_
#define VEHICLEIDENTIFICATION_H_

#include <string>

#include "inet/common/INETDefs.h"

namespace inet {

#define NOT_ID  0xDEADAFFEDEADAFFE

class INET_API VehicleIdentification {

protected:
  uint64 id; // never 0

public:
  static const VehicleIdentification UNSPECIFIED_ADDRESS;
  static const int ID_SIZE = 16;

  VehicleIdentification() { id = 0xDEADAFFEDEADAFFE; }
  explicit VehicleIdentification(uint64 vid) { id = vid; }
//  explicit VehicleIdentification(int rnd, int seed) { rnd ? id = rng(seed) : id = rng();}
  explicit VehicleIdentification(int rnd, int seed);
  ~VehicleIdentification() {}

  uint64 getId() const { return id; }
  void setId(uint64 i) { id = i; }
  void setId(const char* hexstr);
  std::string str() const;
  std::string get_str() const;

  bool tryParse(const char *addr);
  bool isUnspecified() const { return id == NOT_ID; }
  bool isUnicast() const { return true; }
  bool isMulticast() const { return true; }
  bool isBroadcast() const { return true; }

  bool operator==(const VehicleIdentification& addr1) const { return id == addr1.id; }
  bool operator!=(const VehicleIdentification& addr1) const { return id != addr1.id; }
  bool operator<(const VehicleIdentification& addr1) const { return id < addr1.id; }
  bool operator>(const VehicleIdentification& addr1) const { return id > addr1.id; }

};

} /* namespace inet */

#endif /* VEHICLEIDENTIFICATION_H_ */
