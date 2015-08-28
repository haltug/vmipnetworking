//
// Copyright (C) 2006-2012 Christoph Sommer <christoph.sommer@uibk.ac.at>
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
//

#ifndef VEINS_MOBILITY_TRACI_TRACIMOBILITY_H
#define VEINS_MOBILITY_TRACI_TRACIMOBILITY_H

#include <string>
#include <fstream>
#include <list>
#include <stdexcept>

#include "inet/common/geometry/common/Coord.h"
#include "inet/environment/contract/IPhysicalEnvironment.h"
#include "inet/mobility/base/MovingMobilityBase.h"
#include "inet/mobility/traci/TraCIScenarioManager.h"

//#include "inet/common/INETMath.h"


/**
 * @brief
 * Used in modules created by the TraCIScenarioManager.
 *
 * This module relies on the TraCIScenarioManager for state updates
 * and can not be used on its own.
 *
 * See the Veins website <a href="http://veins.car2x.org/"> for a tutorial, documentation, and publications </a>.
 *
 * @author Christoph Sommer, David Eckhoff, Luca Bedogni, Bastian Halmos, Stefan Joerer
 * @author Halis Altug - adapted to INET 3.0; 17.8.2015
 *
 * @see TraCIScenarioManager
 * @see TraCIScenarioManagerLaunchd
 *
 * @ingroup mobility
 */
namespace inet {
using namespace inet::physicalenvironment;

class INET_API TraCIMobility : public MovingMobilityBase {
    protected:
        IPhysicalEnvironment *environment;
        TraCIScenarioManager *traciManager;
        static simsignal_t parkingStateChangedSignal;
        static simsignal_t accidentStateChangedSignal;

        TraCIScenarioManager::VehicleSignal vehicleState; /**<updated by nextPosition() */
        bool isParking;
        bool hasAccident;
        // current state
        std::string vehicleId;
        std::string lastRoadId;
        double currentSpeed;
        double currentAngle;
        Coord currentOrientation;
        // next state
        Coord nextPosition;
        EulerAngles nextOrientation;
        std::string nextRoadId;
        double nextSpeed;
        double nextAngle;

	public:
		virtual ~TraCIMobility() {};
	    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
	    virtual void initialize(int stage) override;
		virtual void finish();

	    virtual void move() override;
		virtual void setStartPosition(std::string vehicle_id, const Coord& position, std::string road_id, double speed, double angle, simtime_t nextTime, double updateTime, TraCIScenarioManager::VehicleSignal state);
		virtual void setNextPosition(const Coord& position, std::string road_id, double speed, double angle, simtime_t nextTime, TraCIScenarioManager::VehicleSignal state);
//		virtual Coord getNextPosition(const simtime_t& actualTime = simTime());

		virtual void setParkingState(bool state);
		virtual bool hasParkingState();
		virtual void setAccidentState(bool state);
		virtual bool hasAccidentState();
		virtual std::string getCurrentRoadId();
		virtual TraCIScenarioManager::VehicleSignal getVehicleState();
		virtual double getMaxSpeed() const override { return currentSpeed; }
		double calculateCO2emission(double v, double a);
// =================================================================================================================================
        class Statistics {
            public:
                double firstRoadNumber; /**< for statistics: number of first road we encountered (if road id can be expressed as a number) */
                simtime_t startTime; /**< for statistics: start time */
                simtime_t totalTime; /**< for statistics: total time travelled */
                simtime_t stopTime; /**< for statistics: stop time */
                double minSpeed; /**< for statistics: minimum value of currentSpeed */
                double maxSpeed; /**< for statistics: maximum value of currentSpeed */
                double totalDistance; /**< for statistics: total distance travelled */
                double totalCO2Emission; /**< for statistics: total CO2 emission */
                const double MY_INFINITY = (std::numeric_limits<double>::has_infinity ? std::numeric_limits<double>::infinity() : std::numeric_limits<double>::max());
                void initialize();
                void watch(cSimpleModule& module);
                void recordScalars(cSimpleModule& module);
        };

        cOutVector currentPosXVec; /**< vector plotting posx */
        cOutVector currentPosYVec; /**< vector plotting posy */
        cOutVector currentSpeedVec; /**< vector plotting speed */
        cOutVector currentAccelerationVec; /**< vector plotting acceleration */
        cOutVector currentCO2EmissionVec; /**< vector plotting current CO2 emission */
        Statistics statistics; /**< everything statistics-related */
};

class INET_API TraCIMobilityAccess
{
	public:
		TraCIMobility* get(cModule* host) {
			TraCIMobility* traci = TraCIModuleFinder<TraCIMobility*>::findSubModule(host);
			ASSERT(traci);
			return traci;
		};
};
}

#endif

