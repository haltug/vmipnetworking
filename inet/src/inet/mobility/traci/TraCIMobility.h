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

#include "inet/mobility/traci/BaseMobility.h"
#include "inet/mobility/traci/FindModule.h"
#include "inet/mobility/traci/TraCIScenarioManager.h"
#include "inet/mobility/traci/TraCICommandInterface.h"
#include "inet/common/geometry/common/Coord.h"

#include "inet/mobility/traci/MiXiMDefs.h"
#include "inet/mobility/traci/Move.h"
#include "inet/mobility/traci/BaseWorldUtility.h"
#include "inet/mobility/traci/HostState.h"


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
 *
 * @see TraCIScenarioManager
 * @see TraCIScenarioManagerLaunchd
 *
 * @ingroup mobility
 */
#define TRACI_SIGNAL_PARKING_CHANGE_NAME "parkingStateChanged"

namespace inet {

class INET_API TraCIMobility : public cSimpleModule, public cListener
{
	public:
    enum BorderPolicy {
            REFLECT,       ///< reflect off the wall
            WRAP,          ///< reappear at the opposite edge (torus)
            PLACERANDOMLY, ///< placed at a randomly chosen position on the playground
            RAISEERROR     ///< stop the simulation with error
        };

        /**
         * @brief The kind field of messages
         *
         * that are used internally by this class have one of these values
         */
        enum BaseMobilityMsgKinds {
            MOVE_HOST = 21311,
            MOVE_TO_BORDER,
            /** Stores the id on which classes extending BaseMobility should
             * continue their own kinds.*/
            LAST_BASE_MOBILITY_KIND,
        };

        /**
         * @brief Specifies which border actually has been reached
         */
        enum BorderHandling {
            NOWHERE,   ///< not outside the playground
            X_SMALLER, ///< x smaller than 0
            X_BIGGER,  ///< x bigger or equal than playground size
            Y_SMALLER, ///< y smaller than 0
            Y_BIGGER,  ///< y bigger or equal than playground size
            Z_SMALLER, ///< z smaller than 0
            Z_BIGGER   ///< z bigger or equal than playground size
        };

        bool notAffectedByHostState;
        /** @brief Pointer to BaseWorldUtility -- these two must know each other */
        BaseWorldUtility *world;

        /** @brief Stores the current position and move pattern of the host*/
        Move move;

        /** @brief Store the category of HostMove */
        const static simsignalwrap_t mobilityStateChangedSignal;

        /** @brief Time interval (in seconds) to update the hosts position*/
        simtime_t updateInterval;

        /** @brief Self message to trigger movement */
        cMessage* moveMsg;

        /** @brief debug this core module? */
        bool coreDebug;

        /** @brief Enable depth dependent scaling of nodes when 3d and tkenv is
         * used. */
        bool scaleNodeByDepth;

        /** @brief Scaling of the playground in X direction.*/
        double playgroundScaleX;
        /** @brief Scaling of the playground in Y direction.*/
        double playgroundScaleY;

        /** @brief The original width the node is displayed width.*/
        double origDisplayWidth;
        /** @brief The original height the node is displayed width.*/
        double origDisplayHeight;

        /** @brief The original size of the icon of the node.*/
        double origIconSize;

        virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj);
        virtual void handleHostState(const HostState& state);

		TraCIMobility() : isPreInitialized(false), manager(0), commandInterface(0), vehicleCommandInterface(0) {}
		~TraCIMobility() {
            cancelAndDelete(moveMsg);
			delete vehicleCommandInterface;
            delete world;
		}
	    virtual void handleMessage(cMessage *msg);
		virtual void initialize(int);
		virtual void finish();

		virtual void handleSelfMsg(cMessage *msg);
		virtual void preInitialize(std::string external_id, const inet::Coord& position, std::string road_id = "", double speed = -1, double angle = -1);
		virtual void nextPosition(const inet::Coord& position, std::string road_id = "", double speed = -1, double angle = -1, TraCIScenarioManager::VehicleSignal signals = TraCIScenarioManager::VEH_SIGNAL_UNDEF);
		virtual void changePosition();
		virtual void changeParkingState(bool);
		virtual void updateDisplayString();
		virtual void setExternalId(std::string external_id) {
			this->external_id = external_id;
		}
		virtual std::string getExternalId() const {
			if (external_id == "") throw cRuntimeError("TraCIMobility::getExternalId called with no external_id set yet");
			return external_id;
		}
		virtual double getAntennaPositionOffset() const {
			return antennaPositionOffset;
		}
		virtual inet::Coord getPositionAt(const simtime_t& t) const {
			return move.getPositionAt(t) ;
		}
		virtual bool getParkingState() const {
			return isParking;
		}
		virtual std::string getRoadId() const {
			if (road_id == "") throw cRuntimeError("TraCIMobility::getRoadId called with no road_id set yet");
			return road_id;
		}
		virtual double getSpeed() const {
			if (speed == -1) throw cRuntimeError("TraCIMobility::getSpeed called with no speed set yet");
			return speed;
		}
		virtual TraCIScenarioManager::VehicleSignal getSignals() const {
			if (signals == -1) throw cRuntimeError("TraCIMobility::getSignals called with no signals set yet");
			return signals;
		}

	    virtual Coord getCurrentPosition() const {
	        return move.getStartPos();
	    }
	    virtual Coord getCurrentSpeed() const {
	        return move.getDirection() * move.getSpeed();
	    }

	    virtual int iconSizeTagToSize(const char* tag);
	    virtual const char* iconSizeToTag(double size);
	    virtual void handleBorderMsg( cMessage* );
	    virtual void updatePosition();
	    double playgroundSizeX() const  {return world->getPgs()->x;}
	    double playgroundSizeY() const  {return world->getPgs()->y;}
	    double playgroundSizeZ() const  {return world->getPgs()->z;}
	    Coord getRandomPosition() { return world->getRandomPosition();}
	    bool handleIfOutside(BorderPolicy, inet::Coord&, inet::Coord&, inet::Coord&, double&);
	    BorderHandling checkIfOutside(Coord,Coord& );
	    void goToBorder( BorderPolicy, BorderHandling, inet::Coord&, inet::Coord& );
	    void reflectCoordinate(BorderHandling border, inet::Coord& c);
	    void reflectIfOutside(BorderHandling, inet::Coord&, inet::Coord&, inet::Coord&, double&);
	    void wrapIfOutside(BorderHandling, inet::Coord&, inet::Coord&);
	    void placeRandomlyIfOutside( inet::Coord& );
	    const static simsignalwrap_t catHostStateSignal;
	    cModule *const findHost(void);
	    const cModule *const findHost(void) const;
	    const cModule *const getNode() const {
	        return findHost();
	    };
	    bool isInBoundary(inet::Coord c, inet::Coord lowerBound, inet::Coord upperBound) {
	        return  lowerBound.x <= c.x && c.x <= upperBound.x &&
	                lowerBound.y <= c.y && c.y <= upperBound.y &&
	                lowerBound.z <= c.z && c.z <= upperBound.z;
	    }

	    virtual void fixIfHostGetsOutside(); /**< called after each read to check for (and handle) invalid positions */

		/**
		 * returns angle in rads, 0 being east, with -M_PI <= angle < M_PI.
		 */
		virtual double getAngleRad() const {
			if (angle == M_PI) throw cRuntimeError("TraCIMobility::getAngleRad called with no angle set yet");
			return angle;
		}
		virtual TraCIScenarioManager* getManager() const {
			if (!manager) manager = TraCIScenarioManagerAccess().get();
			return manager;
		}
		virtual TraCICommandInterface* getCommandInterface() const {
			if (!commandInterface) commandInterface = getManager()->getCommandInterface();
			return commandInterface;
		}
		virtual TraCICommandInterface::Vehicle* getVehicleCommandInterface() const {
			if (!vehicleCommandInterface) vehicleCommandInterface = new TraCICommandInterface::Vehicle(getCommandInterface()->vehicle(getExternalId()));
			return vehicleCommandInterface;
		}


	protected:
		bool debug; /**< whether to emit debug messages */
		int accidentCount; /**< number of accidents */


		bool isPreInitialized; /**< true if preInitialize() has been called immediately before initialize() */

		std::string external_id; /**< updated by setExternalId() */
		double antennaPositionOffset; /**< front offset for the antenna on this car */

		simtime_t lastUpdate; /**< updated by nextPosition() */
		inet::Coord roadPosition; /**< position of front bumper, updated by nextPosition() */
		std::string road_id; /**< updated by nextPosition() */
		double speed; /**< updated by nextPosition() */
		double angle; /**< updated by nextPosition() */
		TraCIScenarioManager::VehicleSignal signals; /**<updated by nextPosition() */

		cMessage* startAccidentMsg;
		cMessage* stopAccidentMsg;
		mutable TraCIScenarioManager* manager;
		mutable TraCICommandInterface* commandInterface;
		mutable TraCICommandInterface::Vehicle* vehicleCommandInterface;
		double last_speed;

		const static simsignalwrap_t parkingStateChangedSignal;

		bool isParking;




		/**
		 * Returns the amount of CO2 emissions in grams/second, calculated for an average Car
		 * @param v speed in m/s
		 * @param a acceleration in m/s^2
		 * @returns emission in g/s
		 */
		double calculateCO2emission(double v, double a) const;

		/**
		 * Calculates where the antenna of this car is, given its front bumper position
		 */
		inet::Coord calculateAntennaPosition(const inet::Coord& vehiclePos) const;




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
			TraCIMobility* traci = FindModule<TraCIMobility*>::findSubModule(host);
			ASSERT(traci);
			return traci;
		};
};
}

#endif

