#pragma once

#include "Structs.h"
#include "Receiver.h"
#include "Helper.h"
#include "Router.h"

class Controller {
    private:
    MYSQL* connector;

    string host;
    string username;
    string password;
    string database;
    int receiversCount;
    int helpersCount;
    uint16_t queueForHelper;

    vector<string> interfaces;
    vector<string> interfaceType;

    

    uint8_t inDevicesCount;
    uint8_t outDevicesCount;
    InDevice* inDevices[256];
    OutDevice* outDevices[256];

    bool startUp;

    public:
    vector<Router *> routers;
    vector<DpdkWorkerThread *> workers;

    Controller(string host, string username, string password, string database, int receiversCount, int helpersCount);
    ~Controller();
    bool connect();
    MYSQL_RES* executeQuery(string query);
    bool createInterfacesVector();
    bool initDevices(vector<DpdkDevice *> deviceList);
    bool initRouters();
    bool updateRoutesAndGRVP();
    void printStats();
};