#include "Router.h"

Router::Router(InDevice *inDevice, OutDevice **outDevices, uint8_t outDevicesCount,
               uint8_t receiversCount, uint8_t helpersCount, uint16_t *queueForHelper)
{
    this->inDevice = inDevice;
    this->outDevices = outDevices;
    this->outDevicesCount = outDevicesCount;
    this->receiversCount = receiversCount;
    this->helpersCount = helpersCount;
    this->queueForHelper = queueForHelper;

    this->rt = new RoutingTable(inDevice, outDevices, outDevicesCount);

    lastSent = 0;
    this->readyToStart = new bool[receiversCount];

    this->readBlocker = new mutex[receiversCount];

    for (uint8_t i = 0; i < receiversCount; i++)
    {
        Receiver *receiver = new Receiver(inDevice, i);
        readyToStart[i] = false;
        for (uint8_t j = 0; j < helpersCount; j++)
        {
            Helper *helper = new Helper(receiver, outDevices, outDevicesCount, rt,
                                        &lastSent, &readBlocker[i], &sentBlocker, &readyToStart[i], *queueForHelper);
            (*queueForHelper)++;
            helpers.push_back(helper);
        }
        receivers.push_back(receiver);
    }
}