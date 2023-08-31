#include "InDevice.h"

InDevice::InDevice(DpdkDevice *device)
{
    this->device = device;
    globalPosition = 0;
}

InDevice::~InDevice() {}

void InDevice::receivePackets(MBufRawPacket **mbufArr, uint8_t &numOfPackets, uint32_t &globalPosition)
{
    blocker.lock();
    numOfPackets = device->receivePackets(mbufArr, MBUFARRSIZE, 0);
    if (numOfPackets)
    {
        this->globalPosition++;
        if (this->globalPosition == 0)
            this->globalPosition++;
        globalPosition = this->globalPosition;
    }
    blocker.unlock();
}