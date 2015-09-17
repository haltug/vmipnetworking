//
// Halis Altug - mail@halisaltug.de - TU Darmstadt - Multimedia Kommunikation - 2015
// Christoph Sommer <christoph.sommer@uibk.ac.at>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "inet/mobility/traci/TraCIMobility.h"

#include <limits>
#include <iostream>
#include <sstream>

#include "inet/environment/contract/IPhysicalEnvironment.h"

namespace inet {
using namespace inet::physicalenvironment;


Define_Module(TraCIMobility);

simsignal_t TraCIMobility::parkingStateChangedSignal = cComponent::registerSignal("parkingStateChangedSignal");
simsignal_t TraCIMobility::accidentStateChangedSignal = cComponent::registerSignal("accidentStateChangedSignal");
// Statistics
simsignal_t TraCIMobility::currentPosXVec = cComponent::registerSignal("currentPosXVec");
simsignal_t TraCIMobility::currentPosYVec = cComponent::registerSignal("currentPosYVec");
simsignal_t TraCIMobility::currentSpeedVec = cComponent::registerSignal("currentSpeedVec");
simsignal_t TraCIMobility::currentAccelerationVec = cComponent::registerSignal("currentAccelerationVec");
simsignal_t TraCIMobility::currentCO2EmissionVec = cComponent::registerSignal("currentCO2EmissionVec");

void TraCIMobility::initialize(int stage) {
    MovingMobilityBase::initialize(stage);
	if (stage == INITSTAGE_LOCAL) {
        EV_DETAIL << "Initializing TraCIMobility." << " Start position: " << lastPosition << endl;
        environment = check_and_cast<IPhysicalEnvironment *>(getModuleByPath(par("environmentModule")));
        traciManager = check_and_cast<TraCIScenarioManager *>(getModuleByPath(par("traciManager")));
        if(!traciManager)
            cRuntimeError("Initialization of TraCIMobility aborted. No TraCIScenarioManager exists. Check your .ned file.");
        // Set statistics configuration
        firstRoadNumber = MY_INFINITY;
        startTime = simTime();
        totalTime = 0;
        stopTime = 0;
        minSpeed = MY_INFINITY;
        maxSpeed = -MY_INFINITY;
        totalDistance = 0;
        totalCO2Emission = 0;
        WATCH(totalTime);
        WATCH(minSpeed);
        WATCH(maxSpeed);
        WATCH(totalDistance);

		// set base class configuration
		nextChange = -1;
		stationary = false;
	}
}

void TraCIMobility::finish()
{
}

void TraCIMobility::setStartPosition(std::string vehicle_id, const Coord& position, std::string road_id, double speed, double angle, simtime_t nextTime, double updateTime, TraCIScenarioManager::VehicleSignal state) {
    EV_DEBUG << "Setting start position of vehicle to " << position << endl;
    Enter_Method("Setting first vehicle position.");
    this->vehicleId = vehicle_id;
    this->lastPosition.x = nextPosition.x = position.x;
    this->lastPosition.y = nextPosition.y = position.y;
//    this->lastPosition.z = position.z; // do we need this?
    this->currentSpeed = nextSpeed = speed;
    this->currentAngle = nextAngle = angle;
    this->lastRoadId = nextRoadId = road_id;
    this->updateInterval = updateTime;
//    this->nextChange = nextTime + this->updateInterval;
    this->vehicleState = state;
//    EV << "Checking position." << endl;
    checkPosition();
//    EV << "Scheduling" << endl;
    scheduleUpdate();
}

void TraCIMobility::setNextPosition(const Coord& position, std::string road_id, double speed, double angle, simtime_t nextTime, TraCIScenarioManager::VehicleSignal state) {
//    if(this->vehicleId != vehicle_id)
//        cRuntimeError("VehicleId is wrong. Cannot set new parameters for wrong vehicle. Check calling function.");
    EV_DEBUG << "Setting next position of vehicle to " << position << endl;
    Enter_Method("Setting next vehicle position.");
    this->nextPosition.x = position.x;
    this->nextPosition.y = position.y;
//    this->lastPosition.z = position.z; // do we need this?
    this->nextSpeed = speed;
    this->nextAngle = angle;
    this->nextRoadId = road_id;
//    this->nextChange = nextTime;
    this->vehicleState = state;
    scheduleUpdate();
}

void TraCIMobility::move() {
    // FIXME check statistics variables
    if(lastUpdate == simTime())
        cRuntimeError("Move function called twice in one time step.");

    if (startTime != simTime()) {
        simtime_t updateInterval = simTime() - this->lastUpdate;
        double distance = lastPosition.distance(nextPosition);
        totalDistance += distance;
        totalTime += updateInterval;
        if (nextSpeed != -1) {
            minSpeed = std::min(minSpeed, nextSpeed);
            maxSpeed = std::max(maxSpeed, nextSpeed);
            emit(currentSpeedVec, nextSpeed);
            if (currentSpeed != -1) {
                double acceleration = (nextSpeed - currentSpeed) / this->updateInterval;
                double co2emission = calculateCO2emission(nextSpeed, acceleration);
//                emit(currentAccelerationVec,acceleration);
//                emit(currentCO2EmissionVec,co2emission);
                totalCO2Emission += co2emission * this->updateInterval.dbl();
            }
        }
    }
    handleIfOutside(RAISEERROR, nextPosition, lastSpeed, nextAngle);
    lastPosition.x = nextPosition.x;
    lastPosition.y = nextPosition.y;
//    lastPosition.z = nextPosition.z;
//    emit(currentPosXVec,lastPosition.x);
//    emit(currentPosYVec,lastPosition.y);
    currentSpeed = nextSpeed;
    currentAngle = nextAngle;
    currentOrientation = Coord(cos(currentAngle), -sin(currentAngle));
    lastRoadId = nextRoadId;
}

void TraCIMobility::setParkingState(bool state) {
    this->isParking = state;
    emit(parkingStateChangedSignal, this);
}
bool TraCIMobility::hasParkingState() {
    return this->isParking;
}
void TraCIMobility::setAccidentState(bool state) {
    hasAccident = state;
    emit(accidentStateChangedSignal, this);
}
bool TraCIMobility::hasAccidentState() {
    return hasAccident;
}

std::string TraCIMobility::getCurrentRoadId() {
    return this->lastRoadId;
}
TraCIScenarioManager::VehicleSignal TraCIMobility::getVehicleState() {
    return this->vehicleState;
}

double TraCIMobility::calculateCO2emission(double v, double a) {
    // Calculate CO2 emission parameters according to:
    // Cappiello, A. and Chabini, I. and Nam, E.K. and Lue, A. and Abou Zeid, M., "A statistical model of vehicle emissions and fuel consumption," IEEE 5th International Conference on Intelligent Transportation Systems (IEEE ITSC), pp. 801-809, 2002

    double A = 1000 * 0.1326; // W/m/s
    double B = 1000 * 2.7384e-03; // W/(m/s)^2
    double C = 1000 * 1.0843e-03; // W/(m/s)^3
    double M = 1325.0; // kg

    // power in W
    double P_tract = A*v + B*v*v + C*v*v*v + M*a*v; // for sloped roads: +M*g*sin_theta*v

    // "Category 9 vehicle" (e.g. a '94 Dodge Spirit)
    double alpha = 1.11;
    double beta = 0.0134;
    double delta = 1.98e-06;
    double zeta = 0.241;
    double alpha1 = 0.973;

    if (P_tract <= 0) return alpha1;
    return alpha + beta*v*3.6 + delta*v*v*v*(3.6*3.6*3.6) + zeta*a*v;
}

} // namespace inet
