package veins.base.modules;

// Interface for application layer modules
moduleinterface IBaseApplLayer
{
    parameters:
        int headerLength @unit("bit"); // length of the application message header (in bits)
    gates:
        input lowerLayerIn @loose; // from network layer
        output lowerLayerOut @loose; // to network layer
        input lowerControlIn; // control from network layer
        output lowerControlOut; // control to network layer
}

