#pragma once

#include "DpdkDevice.h"
#include "DpdkDeviceList.h"
#include <unistd.h>
#include <string>
#include <mutex>
#include <sys/sysinfo.h>
#include "mysql/mysql.h"
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

#define MBUFARRSIZE 32

using namespace std;
using namespace pcpp;

struct vlan
{
	uint8_t firstByte;
	uint8_t secondByte;
	uint16_t vlan;
};

