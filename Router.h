#pragma once

#include "Structs.h"
#include "InDevice.h"
#include "OutDevice.h"
#include "Receiver.h"
#include "Helper.h"
#include "RoutingTable.h"

class Router
{
public:
    string PciID;
    InDevice *inDevice;
    OutDevice **outDevices;
    uint8_t outDevicesCount;
    RoutingTable *rt;
    uint8_t receiversCount;
    uint8_t helpersCount;

    uint16_t lastSent;
    mutex *readBlocker;
    mutex sentBlocker;
    bool *readyToStart;
    uint16_t *queueForHelper;

    vector<Receiver *> receivers;
    vector<Helper *> helpers;

    Router(InDevice *inDevice, OutDevice **outDevices, uint8_t outDevicesCount,
           uint8_t receiversCount, uint8_t helpersCount, uint16_t *queueForHelper);
};