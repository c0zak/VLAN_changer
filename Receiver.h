#pragma once

#include "Structs.h"
#include "InDevice.h"

class Receiver : public pcpp::DpdkWorkerThread
{
private:
	uint32_t m_CoreId;

public:
	bool m_Stop;
	Receiver(InDevice *device, int instanceNum);

	~Receiver();

	bool run(uint32_t coreId);

	void stop();
	uint64_t count;

	InDevice *device;
	uint8_t *buf;
	uint16_t *lengths;
	uint32_t *lengthOffset;
	uint32_t *globalPositions;
	uint16_t *packLengths;
	uint16_t *readyChecker;
	RawPacket** packsBuf;
	uint32_t bPos;
	uint32_t sPos;
	uint32_t firstUnreadedPosition;
	uint32_t indexOfFirstUnreadedPosition;
	uint32_t getCoreId() const;

	uint32_t moveCircleIndex();
};
