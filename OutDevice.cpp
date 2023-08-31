#include "OutDevice.h"

OutDevice::OutDevice(DpdkDevice *device, uint8_t counterCount)
{
    this->device = device;
    this->counterCount = counterCount;
    count = new uint64_t[counterCount];
    requests = new uint64_t[counterCount];

    for (uint8_t i = 0; i < counterCount; i++)
    {
        count[i] = 0;
        requests[i] = 0;
    }
}

OutDevice::~OutDevice() {}

void OutDevice::sendPackets(pcpp::MBufRawPacket **rawPacketsArr, uint16_t arrLength, uint16_t queue)
{
    // blocker.lock();
    requests[queue] += arrLength;

    uint16_t packetsSent = 0;

    while (packetsSent < arrLength)
        packetsSent = device->sendPackets(rawPacketsArr, arrLength, queue, false);
    
    count[queue] += packetsSent;

    // blocker.unlock();
}