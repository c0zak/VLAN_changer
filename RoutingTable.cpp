#include "RoutingTable.h"

RoutingTable::RoutingTable(InDevice* inDevice, OutDevice** outDevices, uint8_t countOutDevices) {
    
    this->inDevice = inDevice;
    this->outDevices = outDevices;
    this->countOutDevices = countOutDevices;
    this->pciID = inDevice->device->getPciAddress();

    for (int i = 0; i < countOutDevices; i++) {
        for (int j = 0; j < 4095; j++) {
            for (int k = 0; k < 4095; k++) {
                routes[i][j][k] = 5000;
                grvpTable[j][k] = false;
            }
        }
    }
}

RoutingTable::~RoutingTable() {}

bool RoutingTable::addRoute(string inPciID, string outPciID, uint16_t vlanIn0, uint16_t vlanIn1, uint16_t vlanOut) {
    
    string currentDevicePciId = inDevice->device->getPciAddress();
    
    if (currentDevicePciId.compare(inPciID) != 0)
        return false;
    
    uint8_t outDeviceIndex = 255;
    for (uint8_t i = 0; i < countOutDevices; i++) {
        if (outDevices[i]->device->getPciAddress().compare(outPciID) == 0) {
            outDeviceIndex = i;
            break;
        }
    }
    
    if (outDeviceIndex == 255)
        return false;

    if (vlanIn0 < 0    || vlanIn1 < 0    || vlanOut < 0 ||
		vlanIn0 > 4094 || vlanIn1 > 4094 || vlanOut > 4094)
            return false;

    routes[outDeviceIndex][vlanIn0][vlanIn1] = vlanOut;

    return true;
}

bool RoutingTable::delRoute(string inPciID, string outPciID, uint16_t vlanIn0, uint16_t vlanIn1, uint16_t vlanOut) {
        
    if (pciID.compare(inPciID) != 0)
        return false;
    
    uint8_t outDeviceIndex = 255;
    for (uint8_t i = 0; i < countOutDevices; i++) {
        if (outDevices[i]->device->getPciAddress().compare(outPciID) == 0) {
            outDeviceIndex = i;
            break;
        }
    }
    
    if (outDeviceIndex == 255)
        return false;

    if (vlanIn0 < 0    || vlanIn1 < 0    || vlanOut < 0 ||
		vlanIn0 > 4094 || vlanIn1 > 4094 || vlanOut > 4094)
            return false;

    routes[outDeviceIndex][vlanIn0][vlanIn1] = 5000;

    return true;
}

routeResult RoutingTable::getRoute(uint16_t vlanIn0, uint16_t vlanIn1) {
    routeResult result = {5000, NULL};

    for (uint8_t i = 0; i < countOutDevices; i++) {
        if (routes[i][vlanIn0][vlanIn1] != 5000) {
            result = {routes[i][vlanIn0][vlanIn1], outDevices[i]};
            break;
        }
    }

    if (result.vlanOut == 5000) {
        grvpTable[vlanIn0][vlanIn1] = true;
    }

    return result;
}