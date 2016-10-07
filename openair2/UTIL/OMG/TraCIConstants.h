#ifndef TRACICONSTANTS_H
#define TRACICONSTANTS_H

// command: subscribe simulation variable
#define CMD_SUBSCRIBE_SIM_VARIABLE 0xdb

// departed vehicle ids (get: simulation)
#define VAR_DEPARTED_VEHICLES_IDS 0x74

// command: simulation step (new version)
#define CMD_SIMSTEP2 0x02

// command: close sumo
#define CMD_CLOSE 0x7F

// command: get vehicle variable
#define CMD_GET_VEHICLE_VARIABLE 0xa4

// command: Scenario
#define CMD_SCENARIO 0x73

// max count of vehicles
#define DOMVAR_MAXCOUNT 0x0A

// 32 bit integer
#define TYPE_INTEGER 0x09

// speed of a node
#define DOMVAR_SPEED 0x04

// position of a domain object
#define DOMVAR_POSITION 0x02

// position (2D) (get: vehicle, poi, set: poi)
#define VAR_POSITION 0x42

// speed (get: vehicle)
#define VAR_SPEED 0x40

// position of a domain object
#define DOMVAR_POSITION 0x02

// ids of arrived vehicles (get: simulation)
#define VAR_ARRIVED_VEHICLES_IDS 0x7a

// ****************************************
// RESULT TYPES
// ****************************************
// result type: Ok
#define RTYPE_OK 0x00

#endif