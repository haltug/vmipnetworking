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

#include "inet/mobility/traci/TraCIMobility.h"

#include <limits>
#include <iostream>
#include <sstream>

#include "inet/mobility/traci/BorderMsg_m.h"
#include "inet/mobility/traci/BaseWorldUtility.h"
#include "inet/environment/contract/IPhysicalEnvironment.h"

namespace inet {

Define_Module(TraCIMobility);

const simsignalwrap_t TraCIMobility::parkingStateChangedSignal = simsignalwrap_t(TRACI_SIGNAL_PARKING_CHANGE_NAME);
const simsignalwrap_t TraCIMobility::catHostStateSignal = simsignalwrap_t(MIXIM_SIGNAL_HOSTSTATE_NAME);
const simsignalwrap_t TraCIMobility::mobilityStateChangedSignal = simsignalwrap_t(MIXIM_SIGNAL_MOBILITY_CHANGE_NAME);

void TraCIMobility::initialize(int stage)
{
	if (stage == 0)
	{
//        move();
        playgroundScaleX = 1;
        playgroundScaleY = 1;
        origDisplayWidth = 0;
        origDisplayHeight = 0;
        origIconSize = 0;

        TraCIMobility::findHost()->subscribe(catHostStateSignal, this);
        notAffectedByHostState =    hasPar("notAffectedByHostState") && par("notAffectedByHostState").boolValue();
        hasPar("coreDebug") ? coreDebug = par("coreDebug").boolValue() : coreDebug = false;
        EV_DETAIL << "initializing BaseMobility stage " << stage << endl;
        hasPar("scaleNodeByDepth") ? scaleNodeByDepth = par("scaleNodeByDepth").boolValue() : scaleNodeByDepth = true;
//        world = FindModule<BaseWorldUtility*>::findGlobalModule();
//        if (world == NULL)
//            error("Could not find BaseWorldUtility module");
        EV_DETAIL << "initializing BaseUtility stage " << stage << endl; // for node position
        if (hasPar("updateInterval")) {
            updateInterval = par("updateInterval");
        } else {
            updateInterval = 0;
        }
        if (hasPar("dim2d")) {
            dim2d = par("dim2d").boolValue();
        } else {
            dim2d = false;
        }
//        environment: PhysicalEnvironment;
//        PhysicalEnvironment *environment = dynamic_cast<PhysicalEnvironment *>(findContainingNode(this)->getSubmodule("environmentModule"));
//        isOperational = (!environment) || environment->getState() == NodeStatus::UP;
//        physicalenvironment::IPhysicalEnvironment *ipe = findModuleFromPar<physicalenvironment::IPhysicalEnvironment> (par("environmentModule"), this);
        environment = check_and_cast<physicalenvironment::IPhysicalEnvironment *>(getModuleByPath(par("environmentModule")));

        // initialize Move parameter
//        bool use2D = world->use2D();
        //initalize position with random values
//        Coord pos = world->getRandomPosition();
        Coord pos(
                uniform(environment->getSpaceMin().x, environment->getSpaceMax().x),
                uniform(environment->getSpaceMin().y, environment->getSpaceMax().y),
                uniform(environment->getSpaceMin().z, environment->getSpaceMax().z));
        //read coordinates from parameters if available
        double x = hasPar("x") ? par("x").doubleValue() : -1;
        double y = hasPar("y") ? par("y").doubleValue() : -1;
        double z = hasPar("z") ? par("z").doubleValue() : -1;
        //set position with values from parameters if available
        if(x > -1) pos.x = x;
        if(y > -1) pos.y = y;
        if(z > -1) pos.z = z;
        // set start-position and start-time (i.e. current simulation-time) of the Move
        move.setStart(pos);
        EV_DETAIL << "start pos: " << move.getStartPos().info() << endl;
        //check whether position is within the playground
        if (!isInBoundary(move.getStartPos(), Coord::ZERO, environment->getSpaceMax())) {
            error("node position specified in omnetpp.ini exceeds playgroundsize");
        }
        // set speed and direction of the Move
        move.setSpeed(0);
        move.setDirectionByVector(inet::Coord::ZERO);


		debug = par("debug");
		antennaPositionOffset = par("antennaPositionOffset");
		accidentCount = par("accidentCount");

		currentPosXVec.setName("posx");
		currentPosYVec.setName("posy");
		currentSpeedVec.setName("speed");
		currentAccelerationVec.setName("acceleration");
		currentCO2EmissionVec.setName("co2emission");

		statistics.initialize();
		statistics.watch(*this);

		ASSERT(isPreInitialized);
		isPreInitialized = false;

		inet::Coord nextPos = calculateAntennaPosition(roadPosition);
		nextPos.z = move.getCurrentPosition().z;

		move.setStart(nextPos);
		move.setDirectionByVector(inet::Coord(cos(angle), -sin(angle)));
		move.setSpeed(speed);

		WATCH(road_id);
		WATCH(speed);
		WATCH(angle);

		isParking = false;

		startAccidentMsg = 0;
		stopAccidentMsg = 0;
		manager = 0;
		last_speed = -1;

		if (accidentCount > 0) {
			simtime_t accidentStart = par("accidentStart");
			startAccidentMsg = new cMessage("scheduledAccident");
			stopAccidentMsg = new cMessage("scheduledAccidentResolved");
			scheduleAt(simTime() + accidentStart, startAccidentMsg);
		}
	}
	else if (stage == 1)
	{
        EV_DETAIL << "initializing BaseMobility stage " << stage << endl;
        //get playground scaling
//        if (world->getParentModule() != NULL )
//        {
//            const cDisplayString& dispWorldOwner
//                    = world->getParentModule()->getDisplayString();
//
//            if( dispWorldOwner.containsTag("bgb") )
//            {
//                double origPGWidth = 0.0;
//                double origPGHeight= 0.0;
//                // normally this should be equal to playground size
//                std::istringstream(dispWorldOwner.getTagArg("bgb", 0))
//                        >> origPGWidth;
//                std::istringstream(dispWorldOwner.getTagArg("bgb", 1))
//                        >> origPGHeight;
//
//                //bgb of zero means size isn't set manually
//                if(origPGWidth > 0) {
//                    playgroundScaleX = origPGWidth / playgroundSizeX();
//                }
//                if(origPGHeight > 0) {
//                    playgroundScaleY = origPGHeight / playgroundSizeY();
//                }
//            }
//        }
        //get original display of host
        cDisplayString& disp = findHost()->getDisplayString();
        //get host width and height
        if (disp.containsTag("b")) {
            std::istringstream(disp.getTagArg("b", 0)) >> origDisplayWidth;
            std::istringstream(disp.getTagArg("b", 1)) >> origDisplayHeight;
        }
        //get hosts icon size
        if (disp.containsTag("i")) {
            // choose a appropriate icon size (only if a icon is specified)
            origIconSize = iconSizeTagToSize(disp.getTagArg("is", 0));
        }
        // print new host position on the screen and update bb info
        updatePosition();
        if (move.getSpeed() > 0 && updateInterval > 0)
        {
            EV_DETAIL << "Host is moving, speed=" << move.getSpeed() << " updateInterval=" << updateInterval << endl;
            moveMsg = new cMessage("move", MOVE_HOST);
            //host moves the first time after some random delay to avoid synchronized movements
            scheduleAt(simTime() + uniform(0, updateInterval), moveMsg);
        }
	}
}

void TraCIMobility::finish()
{
	statistics.stopTime = simTime();

	statistics.recordScalars(*this);

	cancelAndDelete(startAccidentMsg);
	cancelAndDelete(stopAccidentMsg);

	isPreInitialized = false;
}

int TraCIMobility::iconSizeTagToSize(const char* tag)
{
    if(strcmp(tag, "vs") == 0) {
        return 16;
    } else if(strcmp(tag, "s") == 0) {
        return 24;
    } else if(strcmp(tag, "n") == 0 || strcmp(tag, "") == 0) {
        return 40;
    } else if(strcmp(tag, "l") == 0) {
        return 60;
    } else if(strcmp(tag, "vl") == 0) {
        return 100;
    }

    return -1;
}

const char* TraCIMobility::iconSizeToTag(double size)
{
    //returns the biggest icon smaller than the passed size (except sizes
    //smaller than the smallest icon
    if(size < 24) {
        return "vs";
    } else if(size < 40) {
        return "s";
    } else if(size < 60) {
        return "n";
    } else if(size < 100) {
        return "l";
    } else {
        return "vl";
    }
}

void TraCIMobility::handleMessage(cMessage * msg)
{
    if (!msg->isSelfMessage())
        error("mobility modules can only receive self messages");

    if(msg->getKind() == MOVE_TO_BORDER){
        handleBorderMsg(msg);
    } else {
        handleSelfMsg(msg);
    }
}


void TraCIMobility::handleSelfMsg(cMessage *msg)
{
    updatePosition();

    if( !moveMsg->isScheduled() && move.getSpeed() > 0) {
        scheduleAt(simTime() + updateInterval, msg);
    } else {
        delete msg;
        moveMsg = NULL;
    }
    if (msg == startAccidentMsg) {
		getVehicleCommandInterface()->setSpeed(0);
		simtime_t accidentDuration = par("accidentDuration");
		scheduleAt(simTime() + accidentDuration, stopAccidentMsg);
		accidentCount--;
	}
	else if (msg == stopAccidentMsg) {
		getVehicleCommandInterface()->setSpeed(-1);
		if (accidentCount > 0) {
			simtime_t accidentInterval = par("accidentInterval");
			scheduleAt(simTime() + accidentInterval, startAccidentMsg);
		}
	}
}

void TraCIMobility::handleBorderMsg(cMessage * msg)
{
    EV_DETAIL << "start MOVE_TO_BORDER:" << move.info() << endl;

    BorderMsg* bMsg = static_cast<BorderMsg*>(msg);

    switch( bMsg->getPolicy() ){
    case REFLECT:
        move.setStart(bMsg->getStartPos());
        move.setDirectionByVector(bMsg->getDirection());
        break;

    case WRAP:
        move.setStart(bMsg->getStartPos());
        break;

    case PLACERANDOMLY:
        move.setStart(bMsg->getStartPos());
        EV_DETAIL << "new random position: " << move.getStartPos().info() << endl;
        break;

    case RAISEERROR:
        error("node moved outside the playground");
        break;

    default:
        error("Unknown BorderPolicy!");
        break;
    }

    fixIfHostGetsOutside();

    updatePosition();

    delete bMsg;

    EV_DETAIL << "end MOVE_TO_BORDER:" << move.info() << endl;
}

void TraCIMobility::updatePosition() {
    EV << "updatePosition: " << move.info() << endl;

    //publish the the new move
    emit(mobilityStateChangedSignal, this);

    if(ev.isGUI())
    {
        std::ostringstream osDisplayTag;
#ifdef __APPLE__
        const int          iPrecis        = 0;
#else
        const int          iPrecis        = 5;
#endif
        cDisplayString&    disp           = findHost()->getDisplayString();

        // setup output stream
        osDisplayTag << std::fixed; osDisplayTag.precision(iPrecis);

        if (playgroundScaleX != 1.0) {
            osDisplayTag << (move.getStartPos().x * playgroundScaleX);
        }
        else {
            osDisplayTag << (move.getStartPos().x);
        }
            disp.setTagArg("p", 0, osDisplayTag.str().data());

        osDisplayTag.str(""); // reset
        if (playgroundScaleY != 1.0) {
            osDisplayTag << (move.getStartPos().y * playgroundScaleY);
        }
        else {
            osDisplayTag << (move.getStartPos().y);
        }
            disp.setTagArg("p", 1, osDisplayTag.str().data());

//            if(!world->use2D() && scaleNodeByDepth)
        if(scaleNodeByDepth)
        {
            const double minScale = 0.25;
            const double maxScale = 1.0;

            //scale host dependent on their z coordinate to simulate a depth
            //effect
            //z-coordinate of zero maps to a scale of maxScale (very close)
            //z-coordinate of playground size z maps to size of minScale (far away)
            double depthScale = minScale
                                + (maxScale - minScale)
                                  * (1.0 - move.getStartPos().z
                                           / playgroundSizeZ());

            if (origDisplayWidth > 0.0 && origDisplayHeight > 0.0) {
                    osDisplayTag.str(""); // reset
                osDisplayTag << (origDisplayWidth * depthScale);
                    disp.setTagArg("b", 0, osDisplayTag.str().data());

                    osDisplayTag.str(""); // reset
                osDisplayTag << (origDisplayHeight * depthScale);
                    disp.setTagArg("b", 1, osDisplayTag.str().data());
                }

            if (origIconSize > 0) {
                // choose a appropriate icon size (only if a icon is specified)
                disp.setTagArg("is", 0,
                               iconSizeToTag(origIconSize * depthScale));
            }
        }
    }
}

void TraCIMobility::reflectCoordinate(BorderHandling border, inet::Coord& c)
{
    switch( border ) {
    case X_SMALLER:
        c.x = (-c.x);
        break;
    case X_BIGGER:
        c.x = (2 * playgroundSizeX() - c.x);
        break;

    case Y_SMALLER:
        c.y = (-c.y);
        break;
    case Y_BIGGER:
        c.y = (2 * playgroundSizeY() - c.y);
        break;

    case Z_SMALLER:
        c.z = (-c.z);
        break;
    case Z_BIGGER:
        c.z = (2 * playgroundSizeZ() - c.z);
        break;

    case NOWHERE:
    default:
        error("wrong border handling case!");
        break;
    }
}

void TraCIMobility::reflectIfOutside(BorderHandling wo, inet::Coord& stepTarget,
                                    inet::Coord& targetPos, inet::Coord& step,
                                    double& angle) {

    reflectCoordinate(wo, targetPos);
    reflectCoordinate(wo, stepTarget);

    switch( wo ) {
    case X_SMALLER:
    case X_BIGGER:
        step.x = (-step.x);
        angle = 180 - angle;
        break;

    case Y_SMALLER:
    case Y_BIGGER:
        step.y = (-step.y);
        angle = -angle;
        break;

    case Z_SMALLER:
    case Z_BIGGER:
        step.z = (-step.z);
        angle = -angle;
        break;

    case NOWHERE:
    default:
        error("wrong border handling case!");
        break;
    }
}

void TraCIMobility::wrapIfOutside(BorderHandling wo,
                                 inet::Coord& stepTarget, inet::Coord& targetPos) {
    switch( wo ) {
    case X_SMALLER:
    case X_BIGGER:
        targetPos.x = (math::modulo(targetPos.x, playgroundSizeX()));
        stepTarget.x = (math::modulo(stepTarget.x, playgroundSizeX()));
        break;

    case Y_SMALLER:
    case Y_BIGGER:
        targetPos.y = (math::modulo(targetPos.y, playgroundSizeY()));
        stepTarget.y = (math::modulo(stepTarget.y, playgroundSizeY()));
        break;

    case Z_SMALLER:
    case Z_BIGGER:
        targetPos.z = (math::modulo(targetPos.z, playgroundSizeZ()));
        stepTarget.z = (math::modulo(stepTarget.z, playgroundSizeZ()));
        break;

    case NOWHERE:
    default:
        error("wrong border handling case!");
        break;
    }
}

TraCIMobility::BorderHandling TraCIMobility::checkIfOutside( inet::Coord targetPos,
                                                           inet::Coord& borderStep )
{
    BorderHandling outside = NOWHERE;

    // Testing x-value
    if (targetPos.x < 0){
        borderStep.x = (-move.getStartPos().x);
        outside = X_SMALLER;
    }
    else if (targetPos.x >= playgroundSizeX()){
        borderStep.x = (playgroundSizeX() - move.getStartPos().x);
        outside = X_BIGGER;
    }

    // Testing y-value
    if (targetPos.y < 0){
        borderStep.y = (-move.getStartPos().y);

        if( outside == NOWHERE
            || fabs(borderStep.x/move.getDirection().x)
               > fabs(borderStep.y/move.getDirection().y) )
        {
            outside = Y_SMALLER;
        }
    }
    else if (targetPos.y >= playgroundSizeY()){
        borderStep.y = (playgroundSizeY() - move.getStartPos().y);

        if( outside == NOWHERE
            || fabs(borderStep.x/move.getDirection().x)
               > fabs(borderStep.y/move.getDirection().y) )
        {
            outside = Y_BIGGER;
        }
    }

    // Testing z-value
//    if (!world->use2D())
    if (!dim2d)
    {
        // going to reach the lower z-border
        if (targetPos.z < 0)
        {
            borderStep.z = (-move.getStartPos().z);

            // no border reached so far
            if( outside==NOWHERE )
            {
                outside = Z_SMALLER;
            }
            // an y-border is reached earliest so far, test whether z-border
            // is reached even earlier
            else if( (outside == Y_SMALLER || outside == Y_BIGGER)
                     && fabs(borderStep.y/move.getDirection().y)
                        > fabs(borderStep.z/move.getDirection().z) )
            {
                outside = Z_SMALLER;
            }
            // an x-border is reached earliest so far, test whether z-border
            // is reached even earlier
            else if( (outside == X_SMALLER || outside == X_BIGGER)
                     && fabs(borderStep.x/move.getDirection().x)
                        > fabs(borderStep.z/move.getDirection().z) )
            {
                outside = Z_SMALLER;
            }

        }
        // going to reach the upper z-border
        else if (targetPos.z >= playgroundSizeZ())
        {
            borderStep.z = (playgroundSizeZ() - move.getStartPos().z);

            // no border reached so far
            if( outside==NOWHERE )
            {
                outside = Z_BIGGER;
            }
            // an y-border is reached earliest so far, test whether z-border
            // is reached even earlier
            else if( (outside==Y_SMALLER || outside==Y_BIGGER)
                     && fabs(borderStep.y/move.getDirection().y)
                        > fabs(borderStep.z/move.getDirection().z) )
            {
                outside = Z_BIGGER;
            }
            // an x-border is reached earliest so far, test whether z-border
            // is reached even earlier
            else if( (outside==X_SMALLER || outside==X_BIGGER)
                      && fabs(borderStep.x/move.getDirection().x)
                         > fabs(borderStep.z/move.getDirection().z) )
            {
                outside = Z_BIGGER;
            }


        }
    }

    EV_DETAIL << "checkIfOutside, outside="<<outside<<" borderStep: " << borderStep.info() << endl;

    return outside;
}

void TraCIMobility::goToBorder(BorderPolicy policy, BorderHandling wo,
                              inet::Coord& borderStep, inet::Coord& borderStart)
{
    double factor;

    EV_DETAIL << "goToBorder: startPos: " << move.getStartPos().info()
           << " borderStep: " << borderStep.info()
           << " BorderPolicy: " << policy
           << " BorderHandling: " << wo << endl;

    switch( wo ) {
    case X_SMALLER:
        factor = borderStep.x / move.getDirection().x;
        borderStep.y = (factor * move.getDirection().y);
        if (!dim2d)
        {
            borderStep.z = (factor * move.getDirection().z); // 3D case
        }

        if( policy == WRAP )
        {
            borderStart.x = (playgroundSizeX());
            borderStart.y = (move.getStartPos().y + borderStep.y);
            if (!dim2d)
            {
                borderStart.z = (move.getStartPos().z
                                 + borderStep.z); // 3D case
            }
        }
        break;

    case X_BIGGER:
        factor = borderStep.x / move.getDirection().x;
        borderStep.y = (factor * move.getDirection().y);
        if (!dim2d)
        {
            borderStep.z = (factor * move.getDirection().z); // 3D case
        }

        if( policy == WRAP )
        {
            borderStart.x = (0);
            borderStart.y = (move.getStartPos().y + borderStep.y);
            if (!dim2d)
            {
                borderStart.z = (move.getStartPos().z
                                 + borderStep.z); // 3D case
            }
        }
        break;

    case Y_SMALLER:
        factor = borderStep.y / move.getDirection().y;
        borderStep.x = (factor * move.getDirection().x);
        if (!dim2d)
        {
            borderStep.z = (factor * move.getDirection().z); // 3D case
        }

        if( policy == WRAP )
        {
            borderStart.y = (playgroundSizeY());
            borderStart.x = (move.getStartPos().x + borderStep.x);
            if (!dim2d)
            {
                borderStart.z = (move.getStartPos().z
                                 + borderStep.z); // 3D case
            }
        }
        break;

    case Y_BIGGER:
        factor = borderStep.y / move.getDirection().y;
        borderStep.x = (factor * move.getDirection().x);
        if (!dim2d)
        {
            borderStep.z = (factor * move.getDirection().z); // 3D case
        }

        if( policy == WRAP )
        {
            borderStart.y = (0);
            borderStart.x = (move.getStartPos().x + borderStep.x);
            if (!dim2d)
            {
                borderStart.z = (move.getStartPos().z
                                 + borderStep.z); // 3D case
            }
        }
        break;

    case Z_SMALLER: // here we are definitely in 3D
        factor = borderStep.z / move.getDirection().z;
        borderStep.x = (factor * move.getDirection().x);
        borderStep.y = (factor * move.getDirection().y);

        if( policy == WRAP )
        {
            borderStart.z = (playgroundSizeZ());
            borderStart.x = (move.getStartPos().x + borderStep.x);
            borderStart.y = (move.getStartPos().y + borderStep.y);
        }
        break;

    case Z_BIGGER: // here we are definitely in 3D
        factor = borderStep.z / move.getDirection().z;
        borderStep.x = (factor * move.getDirection().x);
        borderStep.y = (factor * move.getDirection().y);

        if( policy == WRAP )
        {
            borderStart.z = (0);
            borderStart.x = (move.getStartPos().x + borderStep.x);
            borderStart.y = (move.getStartPos().y + borderStep.y);
        }
        break;

    default:
        factor = 0;
        error("invalid state in goToBorder switch!");
        break;
    }

    EV_DETAIL << "goToBorder: startPos: " << move.getStartPos().info()
           << " borderStep: " << borderStep.info()
           << " borderStart: " << borderStart.info()
           << " factor: " << factor << endl;
}

bool TraCIMobility::handleIfOutside(BorderPolicy policy, inet::Coord& stepTarget,
                                   inet::Coord& targetPos, inet::Coord& step,
                                   double& angle)
{
    // where did the host leave the playground?
    BorderHandling wo;

    // step to reach the border
    inet::Coord borderStep;

    wo = checkIfOutside(stepTarget, borderStep);

    // just return if the next step is not outside the playground
    if( wo == NOWHERE )
    return false;

    EV_DETAIL << "handleIfOutside:stepTarget = " << stepTarget.info() << endl;

    // new start position after the host reaches the border
    inet::Coord borderStart;
    // new direction the host has to move to
    inet::Coord borderDirection;
    // time to reach the border
    simtime_t borderInterval;

    EV_DETAIL << "old values: stepTarget: " << stepTarget.info()
           << " step: " << step.info()
           << " targetPos: " << targetPos.info()
           << " angle: " << angle << endl;

    // which border policy is to be followed
    switch (policy){
    case REFLECT:
        reflectIfOutside( wo, stepTarget, targetPos, step, angle );
        break;
    case WRAP:
        wrapIfOutside( wo, stepTarget, targetPos );
        break;
    case PLACERANDOMLY:
        placeRandomlyIfOutside( targetPos );
        break;
    case RAISEERROR:
        break;
    }

    EV_DETAIL << "new values: stepTarget: " << stepTarget.info()
           << " step: " << step.info()
           << " angle: " << angle << endl;

    // calculate the step to reach the border
    goToBorder(policy, wo, borderStep, borderStart);

    // calculate the time to reach the border
    borderInterval = (borderStep.length()) / move.getSpeed();

    // calculate new start position
    // NOTE: for WRAP this is done in goToBorder
    switch( policy ){
    case REFLECT:
    {
        borderStart = move.getStartPos() + borderStep;
        double d = stepTarget.distance( borderStart );
        borderDirection = (stepTarget - borderStart) / d;
        break;
    }
    case PLACERANDOMLY:
        borderStart = targetPos;
        stepTarget = targetPos;
        break;

    case WRAP:
    case RAISEERROR:
        break;

    default:
        error("unknown BorderPolicy");
        break;
    }

    EV_DETAIL << "border handled, borderStep: "<< borderStep.info()
           << "borderStart: " << borderStart.info()
           << " stepTarget " << stepTarget.info() << endl;

    // create a border self message and schedule it appropriately
    BorderMsg *bMsg = new BorderMsg("borderMove", MOVE_TO_BORDER);
    bMsg->setPolicy(policy);
    bMsg->setStartPos(borderStart);
    bMsg->setDirection(borderDirection);

    scheduleAt(simTime() + borderInterval, bMsg);

    return true;
}

void TraCIMobility::placeRandomlyIfOutside( inet::Coord& targetPos )
{
    targetPos = Coord(
        uniform(environment->getSpaceMin().x, environment->getSpaceMax().x),
        uniform(environment->getSpaceMin().y, environment->getSpaceMax().y),
        uniform(environment->getSpaceMin().z, environment->getSpaceMax().z));
}

cModule *const TraCIMobility::findHost(void)
{
    return FindModule<>::findHost(this);
}

const cModule *const TraCIMobility::findHost(void) const
{
    return FindModule<>::findHost(this);
}

void TraCIMobility::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj) {
    Enter_Method_Silent();
    if (signalID == catHostStateSignal) {
        const HostState *const pHostState = dynamic_cast<const HostState *const>(obj);
        if (pHostState) {
            handleHostState(*pHostState);
        }
        else {
            opp_warning("Got catHostStateSignal but obj was not a HostState pointer?");
        }
    }
}

void TraCIMobility::handleHostState(const HostState& state)
{
    if(notAffectedByHostState)
        return;

    if(state.get() != HostState::ACTIVE) {
        error("Hosts state changed to something else than active which"
              " is not handled by this module. Either handle this state"
              " correctly or if this module really isn't affected by the"
              " hosts state set the parameter \"notAffectedByHostState\""
              " of this module to true.");
    }
}

void TraCIMobility::preInitialize(std::string external_id, const inet::Coord& position, std::string road_id, double speed, double angle)
{
	this->external_id = external_id;
	this->lastUpdate = 0;
	this->roadPosition = position;
	this->road_id = road_id;
	this->speed = speed;
	this->angle = angle;
	this->antennaPositionOffset = par("antennaPositionOffset");

	inet::Coord nextPos = calculateAntennaPosition(roadPosition);
	nextPos.z = move.getCurrentPosition().z;

	move.setStart(nextPos);
	move.setDirectionByVector(inet::Coord(cos(angle), -sin(angle)));
	move.setSpeed(speed);

	isPreInitialized = true;
}

void TraCIMobility::nextPosition(const inet::Coord& position, std::string road_id, double speed, double angle, TraCIScenarioManager::VehicleSignal signals)
{
	if (debug) EV << "nextPosition " << position.x << " " << position.y << " " << road_id << " " << speed << " " << angle << std::endl;
	isPreInitialized = false;
	this->roadPosition = position;
	this->road_id = road_id;
	this->speed = speed;
	this->angle = angle;
	this->signals = signals;
	changePosition();
}

void TraCIMobility::changePosition()
{
	// ensure we're not called twice in one time step
	ASSERT(lastUpdate != simTime());

	// keep statistics (for current step)
	currentPosXVec.record(move.getStartPos().x);
	currentPosYVec.record(move.getStartPos().y);

	inet::Coord nextPos = calculateAntennaPosition(roadPosition);
	nextPos.z = move.getCurrentPosition().z;

	// keep statistics (relative to last step)
	if (statistics.startTime != simTime()) {
		simtime_t updateInterval = simTime() - this->lastUpdate;

		double distance = move.getStartPos().distance(nextPos);
		statistics.totalDistance += distance;
		statistics.totalTime += updateInterval;
		if (speed != -1) {
			statistics.minSpeed = std::min(statistics.minSpeed, speed);
			statistics.maxSpeed = std::max(statistics.maxSpeed, speed);
			currentSpeedVec.record(speed);
			if (last_speed != -1) {
				double acceleration = (speed - last_speed) / updateInterval;
				double co2emission = calculateCO2emission(speed, acceleration);
				currentAccelerationVec.record(acceleration);
				currentCO2EmissionVec.record(co2emission);
				statistics.totalCO2Emission+=co2emission * updateInterval.dbl();
			}
			last_speed = speed;
		} else {
			last_speed = -1;
			speed = -1;
		}
	}
	this->lastUpdate = simTime();

	move.setStart(inet::Coord(nextPos.x, nextPos.y, move.getCurrentPosition().z)); // keep z position
	move.setDirectionByVector(inet::Coord(cos(angle), -sin(angle)));
	move.setSpeed(speed);
	if (ev.isGUI()) updateDisplayString();
	fixIfHostGetsOutside();
	updatePosition();
}

void TraCIMobility::changeParkingState(bool newState) {
	isParking = newState;
	emit(parkingStateChangedSignal, this);
}

void TraCIMobility::updateDisplayString() {
	ASSERT(-M_PI <= angle);
	ASSERT(angle < M_PI);

	getParentModule()->getDisplayString().setTagArg("b", 2, "rect");
	getParentModule()->getDisplayString().setTagArg("b", 3, "red");
	getParentModule()->getDisplayString().setTagArg("b", 4, "red");
	getParentModule()->getDisplayString().setTagArg("b", 5, "0");

	if (angle < -M_PI + 0.5 * M_PI_4 * 1) {
		getParentModule()->getDisplayString().setTagArg("t", 0, "\u2190");
		getParentModule()->getDisplayString().setTagArg("b", 0, "4");
		getParentModule()->getDisplayString().setTagArg("b", 1, "2");
	}
	else if (angle < -M_PI + 0.5 * M_PI_4 * 3) {
		getParentModule()->getDisplayString().setTagArg("t", 0, "\u2199");
		getParentModule()->getDisplayString().setTagArg("b", 0, "3");
		getParentModule()->getDisplayString().setTagArg("b", 1, "3");
	}
	else if (angle < -M_PI + 0.5 * M_PI_4 * 5) {
		getParentModule()->getDisplayString().setTagArg("t", 0, "\u2193");
		getParentModule()->getDisplayString().setTagArg("b", 0, "2");
		getParentModule()->getDisplayString().setTagArg("b", 1, "4");
	}
	else if (angle < -M_PI + 0.5 * M_PI_4 * 7) {
		getParentModule()->getDisplayString().setTagArg("t", 0, "\u2198");
		getParentModule()->getDisplayString().setTagArg("b", 0, "3");
		getParentModule()->getDisplayString().setTagArg("b", 1, "3");
	}
	else if (angle < -M_PI + 0.5 * M_PI_4 * 9) {
		getParentModule()->getDisplayString().setTagArg("t", 0, "\u2192");
		getParentModule()->getDisplayString().setTagArg("b", 0, "4");
		getParentModule()->getDisplayString().setTagArg("b", 1, "2");
	}
	else if (angle < -M_PI + 0.5 * M_PI_4 * 11) {
		getParentModule()->getDisplayString().setTagArg("t", 0, "\u2197");
		getParentModule()->getDisplayString().setTagArg("b", 0, "3");
		getParentModule()->getDisplayString().setTagArg("b", 1, "3");
	}
	else if (angle < -M_PI + 0.5 * M_PI_4 * 13) {
		getParentModule()->getDisplayString().setTagArg("t", 0, "\u2191");
		getParentModule()->getDisplayString().setTagArg("b", 0, "2");
		getParentModule()->getDisplayString().setTagArg("b", 1, "4");
	}
	else if (angle < -M_PI + 0.5 * M_PI_4 * 15) {
		getParentModule()->getDisplayString().setTagArg("t", 0, "\u2196");
		getParentModule()->getDisplayString().setTagArg("b", 0, "3");
		getParentModule()->getDisplayString().setTagArg("b", 1, "3");
	}
	else {
		getParentModule()->getDisplayString().setTagArg("t", 0, "\u2190");
		getParentModule()->getDisplayString().setTagArg("b", 0, "4");
		getParentModule()->getDisplayString().setTagArg("b", 1, "2");
	}
}

void TraCIMobility::fixIfHostGetsOutside()
{
	inet::Coord pos = move.getStartPos();
	inet::Coord dummy = inet::Coord::ZERO;
	double dum;

	bool outsideX = (pos.x < 0) || (pos.x >= playgroundSizeX());
	bool outsideY = (pos.y < 0) || (pos.y >= playgroundSizeY());
	bool outsideZ = (!dim2d) && ((pos.z < 0) || (pos.z >= playgroundSizeZ()));
	if (outsideX || outsideY || outsideZ) {
		error("Tried moving host to (%f, %f) which is outside the playground", pos.x, pos.y);
	}

	handleIfOutside( RAISEERROR, pos, dummy, dummy, dum);
}

double TraCIMobility::calculateCO2emission(double v, double a) const {
	// Calculate CO2 emission parameters according to:
	// Cappiello, A. and Chabini, I. and Nam, E.K. and Lue, A. and Abou Zeid, M., "A statistical model of vehicle emissions and fuel consumption," IEEE 5th International Conference on Intelligent Transportation Systems (IEEE ITSC), pp. 801-809, 2002

	double A = 1000 * 0.1326; // W/m/s
	double B = 1000 * 2.7384e-03; // W/(m/s)^2
	double C = 1000 * 1.0843e-03; // W/(m/s)^3
	double M = 1325.0; // kg

	// power in W
	double P_tract = A*v + B*v*v + C*v*v*v + M*a*v; // for sloped roads: +M*g*sin_theta*v

	/*
	// "Category 7 vehicle" (e.g. a '92 Suzuki Swift)
	double alpha = 1.01;
	double beta = 0.0162;
	double delta = 1.90e-06;
	double zeta = 0.252;
	double alpha1 = 0.985;
	*/

	// "Category 9 vehicle" (e.g. a '94 Dodge Spirit)
	double alpha = 1.11;
	double beta = 0.0134;
	double delta = 1.98e-06;
	double zeta = 0.241;
	double alpha1 = 0.973;

	if (P_tract <= 0) return alpha1;
	return alpha + beta*v*3.6 + delta*v*v*v*(3.6*3.6*3.6) + zeta*a*v;
}


inet::Coord TraCIMobility::calculateAntennaPosition(const inet::Coord& vehiclePos) const {
	inet::Coord corPos;
	if (antennaPositionOffset >= 0.001) {
		//calculate antenna position of vehicle according to antenna offset
		corPos = inet::Coord(vehiclePos.x - antennaPositionOffset*cos(angle), vehiclePos.y + antennaPositionOffset*sin(angle), vehiclePos.z);
	} else {
		corPos = inet::Coord(vehiclePos.x, vehiclePos.y, vehiclePos.z);
	}
	return corPos;
}

// ================================ STATISTICS ================================
void TraCIMobility::Statistics::initialize()
{
    firstRoadNumber = MY_INFINITY;
    startTime = simTime();
    totalTime = 0;
    stopTime = 0;
    minSpeed = MY_INFINITY;
    maxSpeed = -MY_INFINITY;
    totalDistance = 0;
    totalCO2Emission = 0;
}

void TraCIMobility::Statistics::watch(cSimpleModule& )
{
    WATCH(totalTime);
    WATCH(minSpeed);
    WATCH(maxSpeed);
    WATCH(totalDistance);
}

void TraCIMobility::Statistics::recordScalars(cSimpleModule& module)
{
    if (firstRoadNumber != MY_INFINITY) module.recordScalar("firstRoadNumber", firstRoadNumber);
    module.recordScalar("startTime", startTime);
    module.recordScalar("totalTime", totalTime);
    module.recordScalar("stopTime", stopTime);
    if (minSpeed != MY_INFINITY) module.recordScalar("minSpeed", minSpeed);
    if (maxSpeed != -MY_INFINITY) module.recordScalar("maxSpeed", maxSpeed);
    module.recordScalar("totalDistance", totalDistance);
    module.recordScalar("totalCO2Emission", totalCO2Emission);
}

} // namespace inet
