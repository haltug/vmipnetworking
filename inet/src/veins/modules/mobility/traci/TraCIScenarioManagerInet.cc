
#include "inet/common/geometry/common/Coord.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"

//#define WITH_TRACI
//
//void ifInetTraCIMobilityCallPreInitialize(cModule* mod, const std::string& nodeId, const inet::Coord& position, const std::string& road_id, double speed, double angle) {
//	inet::TraCIMobility* inetmm = dynamic_cast< inet::TraCIMobility*>(mod);
//	if (!inetmm) return;
//	inetmm->preInitialize(nodeId, ::inet::Coord(position.x, position.y), road_id, speed, angle);
//}
//
//
//void ifInetTraCIMobilityCallNextPosition(cModule* mod, const inet::Coord& p, const std::string& edge, double speed, double angle) {
//	inet::TraCIMobility *inetmm = dynamic_cast< inet::TraCIMobility*>(mod);
//	if (!inetmm) return;
//	inetmm->nextPosition(::inet::Coord(p.x, p.y), edge, speed, angle);
//}

//#else // not WITH_INET
//
void ifInetTraCIMobilityCallPreInitialize(cModule* mod, const std::string& nodeId, const inet::Coord& position, const std::string& road_id, double speed, double angle) {
	return;
}
//
//
void ifInetTraCIMobilityCallNextPosition(cModule* mod, const inet::Coord& p, const std::string& edge, double speed, double angle) {
	return;
}
//
//#endif // WITH_INET
