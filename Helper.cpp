#include "Helper.h"

Helper::Helper(Receiver *receiver, OutDevice **outDevices, uint8_t outDevicesCount,
               RoutingTable *rTable, uint16_t *lastSent, mutex *readBlocker,
               mutex *sentBlocker, bool *readyToStart, uint16_t queue)
{
    this->receiver = receiver;
    this->outDevices = outDevices;
    this->countOutDevices = outDevicesCount;
    this->rTable = rTable;
    this->lastSent = lastSent;
    this->readBlocker = readBlocker;
    this->sentBlocker = sentBlocker;
    this->readyToStart = readyToStart;
    this->queue = queue;
    count = 0;
    droppedByGRVP = 0;
    droppedByInvalidHeaders = 0;

    posForWrite = new uint8_t[outDevicesCount];

    m_Stop = true;
    m_CoreId = MAX_NUM_OF_CORES + 1;

    // sentBuffer = new RawPacket **[outDevicesCount];
    sentBuffer = new MBufRawPacket **[outDevicesCount];

    for (uint8_t i = 0; i < outDevicesCount; i++)
    {
        posForWrite[i] = 0;
        // sentBuffer[i] = new RawPacket *[MBUFARRSIZE];
        sentBuffer[i] = new MBufRawPacket *[MBUFARRSIZE];
        for (uint8_t j = 0; j < MBUFARRSIZE; j++)
        {
            // sentBuffer[i][j] = new RawPacket();
            sentBuffer[i][j] = new MBufRawPacket();
            sentBuffer[i][j]->init(outDevices[i]->device);
            // sentBuffer[i][j]->reallocateData(2048);
        }
    }
}

Helper::~Helper() {}

vlan Helper::getVlan(uint8_t firstByte, uint8_t secondByte)
{
    uint16_t vlanID = (static_cast<uint16_t>(firstByte) << 8) | static_cast<uint16_t>(secondByte);
    return {firstByte, secondByte, vlanID};
}

vlan Helper::getVlan(uint16_t vlanID)
{
    uint8_t firstByte = static_cast<uint8_t>((vlanID >> 8) & 0xFF);
    uint8_t secondByte = static_cast<uint8_t>(vlanID & 0xFF);
    return {firstByte, secondByte, vlanID};
}

bool Helper::run(uint32_t coreId)
{

    m_CoreId = coreId;
    m_Stop = false;

    // uint32_t currentPos = 0;
    uint32_t endPos = 0;
    bool getted = false;
    uint8_t tempBuffer[10000];
    uint8_t debugBuffer[1600];
    memset(debugBuffer, 0, 1600);

    uint32_t indexForReadingBegin = 0;
    uint16_t currentGlobalPosition = 0;
    uint32_t *firstUnreadedPosition = &receiver->firstUnreadedPosition;
    uint32_t *indexOfFirstUnreadedPosition = &receiver->indexOfFirstUnreadedPosition;

    readBlocker->lock();
    while (!(*readyToStart) && !m_Stop)
    {
        if (receiver->packLengths[0] != 0)
        {
            *readyToStart = true;
            *firstUnreadedPosition = receiver->globalPositions[0];
        }
    }
    readBlocker->unlock();

    while (!m_Stop)
    {
        readBlocker->lock();

        if (currentGlobalPosition > *firstUnreadedPosition)
            currentGlobalPosition = *firstUnreadedPosition - 1;

        if (currentGlobalPosition < *firstUnreadedPosition)
        {
            indexForReadingBegin = *indexOfFirstUnreadedPosition;
            endPos = receiver->moveCircleIndex();
            currentGlobalPosition = receiver->globalPositions[endPos - 1];
            getted = true;
        }

        readBlocker->unlock();

        if (!getted)
            int p = 0;

        if (getted)
        {
            // uint32_t globalPosition = receiverForWork->globalPositions[currentPos];
            // uint8_t *pointToRing = receiver->buf;
            // timespec time;
            // clock_gettime(CLOCK_REALTIME, &time);
            for (uint32_t i = indexForReadingBegin; i < endPos; i++)
            {
                // uint32_t offset = receiver->lengthOffset[i];

                // if (pointToRing[offset + 12] == 0x81 &&
                //     pointToRing[offset + 13] == 0x00 &&
                //     pointToRing[offset + 16] == 0x81 &&
                //     pointToRing[offset + 17] == 0x00)
                RawPacket* pack = receiver->packsBuf[i];
                uint16_t length = pack->getRawDataLen();
                const uint8_t* pointToRing = pack->getRawData();

                if (pointToRing[12] == 0x81 &&
                    pointToRing[13] == 0x00 &&
                    pointToRing[16] == 0x81 &&
                    pointToRing[17] == 0x00)
                {
                    // uint16_t length = receiver->lengths[i];
                    // uint16_t vlanIn0 = getVlan(pointToRing[offset + 14], pointToRing[offset + 15]).vlan;
                    // uint16_t vlanIn1 = getVlan(pointToRing[offset + 18], pointToRing[offset + 19]).vlan;
                    // memcpy(tempBuffer, receiver->buf + offset, 16);
                    // memcpy(tempBuffer + 16, receiver->buf + offset + 20, length - 16);
                    // memset(receiver->buf + offset, 0, length);

                    uint16_t vlanIn0 = getVlan(pointToRing[14], pointToRing[15]).vlan;
                    uint16_t vlanIn1 = getVlan(pointToRing[18], pointToRing[19]).vlan;
                    memcpy(tempBuffer, pointToRing, 16);
                    memcpy(tempBuffer + 16, pointToRing + 20, length - 16);
                    
                    length -= 4;

                    routeResult route = rTable->getRoute(vlanIn0, vlanIn1);
                    if (route.outDevice == NULL)
                    {
                        ++droppedByGRVP;
                        continue;
                    }

                    uint8_t devId = 255;
                    for (uint8_t j = 0; j < countOutDevices; j++)
                    {
                        if (route.outDevice == outDevices[j])
                        {
                            devId = j;
                            break;
                        }
                    }

                    if (devId == 255)
                        continue;

                    vlan vlanOutByte = getVlan(route.vlanOut);
                    tempBuffer[14] = vlanOutByte.firstByte;
                    tempBuffer[15] = vlanOutByte.secondByte;
                    sentBuffer[devId][posForWrite[devId]]->setFreeMbuf(true);
                    sentBuffer[devId][posForWrite[devId]]->clear();
                    // sentBuffer[devId][posForWrite[devId]]->setRawDataNoDel(tempBuffer, length, time);
                    sentBuffer[devId][posForWrite[devId]]->setRawDataNoDel(tempBuffer, length, pack->getPacketTimeStamp());

                    posForWrite[devId]++;
                    count++;
                }
                else
                {
                    memcpy(debugBuffer, pointToRing, receiver->lengths[i]);
                    ++droppedByInvalidHeaders;
                    continue;
                }

                receiver->lengthOffset[i] = 0;
                receiver->lengths[i] = 0;
                receiver->packLengths[i] = 0;
                receiver->globalPositions[i] = 0;
            }

            // uint16_t check = currentGlobalPosition - *lastSent;

            while (currentGlobalPosition - *lastSent != 1 && !m_Stop)
            {
            }

            sentBlocker->lock();
            for (uint8_t i = 0; i < countOutDevices; i++)
            {
                if (posForWrite[i] != 0)
                {
                    outDevices[i]->sendPackets(sentBuffer[i], posForWrite[i], queue);
                    posForWrite[i] = 0;
                }
            }
            (*lastSent)++;
            if (*lastSent == 65535)
                *lastSent = 0;
            sentBlocker->unlock();
            getted = false;
        }
    }

    return true;
}

void Helper::stop()
{
    m_Stop = true;
}

uint32_t Helper::getCoreId() const
{
    return m_CoreId;
}
