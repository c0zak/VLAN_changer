#pragma once

#include "Structs.h"

class InDevice {

    public:
        DpdkDevice* device;
        mutex blocker;
        uint16_t globalPosition;
        

        InDevice(DpdkDevice* device);
        ~InDevice();

        void receivePackets(MBufRawPacket** mbufArr, uint8_t &numOfPackets, uint32_t &globalPosition);
};