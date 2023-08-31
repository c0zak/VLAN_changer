#pragma once

#include "Structs.h"
#include "InDevice.h"
#include "OutDevice.h"
#include "RoutingTable.h"
#include "Receiver.h"

class Helper : public DpdkWorkerThread
{

private:
    uint32_t m_CoreId;
    vlan getVlan(uint8_t firstByte, uint8_t secondByte);
    vlan getVlan(uint16_t vlanID);
    Receiver *receiver;

    uint8_t countOutDevices;
    RoutingTable *rTable;
    uint16_t queue;

public:
    bool m_Stop;
    Helper(Receiver *receiver, OutDevice **outDevices, uint8_t outDevicesCount,
           RoutingTable *rTable, uint16_t *lastSent, mutex *readBlocker,
           mutex *sentBlocker, bool *readyToStart, uint16_t queue);
    ~Helper();

    MBufRawPacket ***sentBuffer;
    uint8_t *posForWrite;
    OutDevice **outDevices;

    uint16_t *lastSent;
    mutex *readBlocker;
    mutex *sentBlocker;
    bool *readyToStart;
    uint64_t count;
    uint64_t droppedByGRVP;
    uint64_t droppedByInvalidHeaders;

    bool run(uint32_t coreId);
    void stop();
    uint32_t getCoreId() const;
};