#pragma once

#include "Structs.h"

class OutDevice{

    public:
    DpdkDevice* device;
    mutex blocker;
    uint64_t* count;
    uint64_t* requests;
    uint8_t counterCount;
    

    OutDevice(DpdkDevice* device, uint8_t counterCount);
    ~OutDevice();

    void sendPackets(pcpp::MBufRawPacket **rawPacketsArr, uint16_t arrLength, uint16_t queue);
};