
#include "inet/common/geometry/common/Coord.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"

void ifInetTraCIMobilityCallPreInitialize(cModule* mod, const std::string& nodeId, const inet::Coord& position, const std::string& road_id, double speed, double angle) {
	return;
}

void ifInetTraCIMobilityCallNextPosition(cModule* mod, const inet::Coord& p, const std::string& edge, double speed, double angle) {
	return;
}
