#pragma once

#include "Structs.h"
#include "InDevice.h"
#include "OutDevice.h"

struct routeResult {
    uint16_t vlanOut;
    OutDevice* outDevice;
};

class RoutingTable {
    public:
    string pciID;
    InDevice* inDevice;
    OutDevice** outDevices;
    uint8_t countOutDevices;

    uint16_t routes[64][4095][4095];
    bool grvpTable[4095][4095];

    RoutingTable(InDevice* inDevice, OutDevice** outDevices, uint8_t countOutDevices);
    ~RoutingTable();

    bool addRoute(string inPciID, string outPciID, uint16_t vlanIn0, uint16_t vlanIn1, uint16_t vlanOut);
    bool delRoute(string inPciID, string outPciID, uint16_t vlanIn0, uint16_t vlanIn1, uint16_t vlanOut);
    routeResult getRoute(uint16_t vlanIn0, uint16_t vlanIn1);
};