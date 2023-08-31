#include "Receiver.h"

Receiver::Receiver(InDevice *device, int instanceNum)
{
	this->device = device;
	m_Stop = true;
	m_CoreId = MAX_NUM_OF_CORES + 1;

	cout << "Start memory allocating for incoming interface with PCI address " << device->device->getPciAddress()
		 << " on instance " << instanceNum << "." << endl;

	buf = new uint8_t[4294967295];
	memset(buf, 0, 4294967295);
	lengths = new uint16_t[67108863];
	memset(lengths, 0, sizeof(uint16_t) * 67108863);
	lengthOffset = new uint32_t[67108863];
	memset(lengthOffset, 0, sizeof(uint16_t) * 67108863);
	globalPositions = new uint32_t[67108863];
	memset(globalPositions, 0, sizeof(uint32_t) * 67108863);
	packLengths = new uint16_t[67108863];
	memset(packLengths, 0, sizeof(uint16_t) * 67108863);
	readyChecker = new uint16_t[67108863];
	memset(readyChecker, 0, sizeof(uint16_t) * 67108863);
	packsBuf = new RawPacket *[67108863];

	for (uint32_t i = 0; i < 67108863; i++)
		packsBuf[i] = new RawPacket();

	cout << "Memory for incoming interface with PCI address " << device->device->getPciAddress()
		 << " on instance " << instanceNum << " allocated successfuly." << endl
		 << endl;
}

Receiver::~Receiver() {}

uint32_t Receiver::moveCircleIndex()
{

	uint32_t ePos = 0;
	while (ePos == 0 && !m_Stop)
		ePos = readyChecker[indexOfFirstUnreadedPosition];
	ePos = indexOfFirstUnreadedPosition + packLengths[indexOfFirstUnreadedPosition];
	uint32_t nPos = ePos;

	if (nPos >= 67108863)
		nPos = 0;

	uint32_t check = 0;
	while (check == 0 && !m_Stop)
		check = readyChecker[nPos];

	for (uint32_t i = indexOfFirstUnreadedPosition; i < ePos; i++)
		readyChecker[i] = 0;


	if (packLengths[nPos] == 0)
	{
		readyChecker[nPos] = 0;
		nPos = 0;
		check = 0;
		while (check == 0 && !m_Stop)
			check = readyChecker[nPos];
	}

	firstUnreadedPosition = globalPositions[nPos];
	indexOfFirstUnreadedPosition = nPos;

	return ePos;
}

bool Receiver::run(uint32_t coreId)
{
	m_CoreId = coreId;
	m_Stop = false;

	count = 0;

	bPos = 0;
	sPos = 0;

	firstUnreadedPosition = 0;
	indexOfFirstUnreadedPosition = 0;

	MBufRawPacket *mbufArr[MBUFARRSIZE] = {};

	for (;;)
	{
		if (m_Stop)
			break;

		uint8_t numOfPackets = 0;
		uint32_t globalPosition = 0;
		device->receivePackets(mbufArr, numOfPackets, globalPosition);

		if (numOfPackets)
		{
			count += numOfPackets;
			for (uint8_t i = 0; i < numOfPackets; ++i)
			{
				uint16_t length = mbufArr[i]->getRawDataLen();
				memcpy(buf + bPos, mbufArr[i]->getRawData(), length);
				bPos += length;
				lengths[sPos] = length;
				globalPositions[sPos] = globalPosition;
				packLengths[sPos] = numOfPackets;
				readyChecker[sPos] = numOfPackets;
				// packsBuf[sPos]->clear();
				packsBuf[sPos] = mbufArr[i];
				// packsBuf[sPos]->setRawData(mbufArr[i]->getRawData(), length, mbufArr[i]->getPacketTimeStamp());
				if (!sPos)
					lengthOffset[sPos] = 0;
				else
					lengthOffset[sPos] = lengthOffset[sPos - 1] + lengths[sPos - 1];
				++sPos;
			}
		}

		if (bPos >= 4294935295)
		{
			bPos = 0;
			readyChecker[sPos] = 0x5555;
			sPos = 0;
		}
	}

	return true;
}

void Receiver::stop()
{
	m_Stop = true;
}

uint32_t Receiver::getCoreId() const
{
	return m_CoreId;
}
