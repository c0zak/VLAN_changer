#include "Controller.h"

Controller::Controller(string host, string username, string password, string database, int receiversCount, int helpersCount)
{

    this->host = host;
    this->username = username;
    this->password = password;
    this->database = database;
    this->receiversCount = receiversCount;
    this->helpersCount = helpersCount;

    inDevicesCount = 0;
    outDevicesCount = 0;
    startUp = true;
    queueForHelper = 0;
}

Controller::~Controller()
{
    mysql_close(connector);
}

bool Controller::connect()
{
    connector = mysql_init(NULL);

    if (mysql_real_connect(connector, host.c_str(), username.c_str(), password.c_str(), database.c_str(), 3306, NULL, 0) == NULL)
    {
        cerr << "Failed to connect to database: Error: " << mysql_error(connector) << endl;
        mysql_close(connector);
        return false;
    }

    return true;
}

MYSQL_RES *Controller::executeQuery(string query)
{

    if (mysql_query(connector, query.c_str()))
    {
        cerr << "Problem with query: " << query << endl
             << "Error: " << mysql_error(connector) << endl;
    }

    MYSQL_RES *result = mysql_store_result(connector);

    return result;
}

bool Controller::createInterfacesVector()
{
    MYSQL_RES *iFacesBase = executeQuery("select * from Interfaces");
    MYSQL_ROW row;

    bool haveIn = false;
    bool haveOut = false;

    while ((row = mysql_fetch_row(iFacesBase)))
    {
        interfaces.push_back(row[0]);
        interfaceType.push_back(row[1]);
        if (interfaceType.back().compare("Incoming") == 0)
            haveIn = true;
        if (interfaceType.back().compare("Outcoming") == 0)
            haveOut = true;
    }

    mysql_free_result(iFacesBase);

    if (interfaces.size() < 2)
    {
        cerr << "Less then 2 interfaces in DB!" << endl;
        return false;
    }

    if (!(haveIn && haveOut))
    {
        cerr << "Have not all types of interfaces in DB!" << endl;
        return false;
    }

    return true;
}

bool Controller::initDevices(vector<DpdkDevice *> deviceList)
{

    DpdkDevice::DpdkDeviceConfiguration devConf;
    devConf.receiveDescriptorsNumber = 2048;
    devConf.transmitDescriptorsNumber = 2048;
    devConf.flushTxBufferTimeout = 100;
    devConf.rssHashFunction = 0;
    devConf.rssKey = NULL;
    devConf.rssKeyLength = 0;

    for (size_t i = 0; i < deviceList.size(); i++)
    {

        string pciID = deviceList[i]->getPciAddress();

        for (size_t j = 0; j < interfaces.size(); j++)
        {

            if (pciID.compare(interfaces.at(j)) == 0)
            {

                if (interfaceType.at(j).compare("Incoming") == 0)
                {
                    inDevices[inDevicesCount] = new InDevice(deviceList[i]);
                    if (!inDevices[inDevicesCount]->device->openMultiQueues(1, 1, devConf))
                    {
                        cerr << "Cannot open interface with pciID '" << pciID << "'" << endl;
                        return false;
                    }
                    cout << "Device " << pciID << " opened as incoming" << endl;
                    inDevicesCount++;
                }
            }
        }
    }

    uint8_t counterCount = inDevicesCount * receiversCount * helpersCount;

    for (size_t i = 0; i < deviceList.size(); i++)
    {

        string pciID = deviceList[i]->getPciAddress();

        for (size_t j = 0; j < interfaces.size(); j++)
        {

            if (pciID.compare(interfaces.at(j)) == 0)
            {

                if (interfaceType.at(j).compare("Outcoming") == 0)
                {
                    outDevices[outDevicesCount] = new OutDevice(deviceList[i], counterCount);
                    if (!outDevices[outDevicesCount]->device->openMultiQueues(1, counterCount, devConf))
                    {
                        cerr << "Cannot open interface with pciID '" << pciID << "'" << endl;
                        return false;
                    }
                    cout << "Device " << pciID << " opened as outcoming" << endl;
                    outDevicesCount++;
                }
            }
        }
    }

    cout << endl
         << "---------------------" << endl;

    return true;
}

bool Controller::initRouters()
{
    struct sysinfo meminfo;
    sysinfo(&meminfo);

    uint64_t freeMemory = meminfo.freeram * meminfo.mem_unit;
    uint64_t memoryForEachReceiver = 4831838199;
    uint64_t GB = 1073741824;

    if (freeMemory < memoryForEachReceiver * receiversCount * inDevicesCount + 5 * GB)
    {
        cerr << "Not enough RAM for this configuration!" << endl;
        return false;
    }

    int coresNum = getNumOfCores();

    if (coresNum < inDevicesCount * (receiversCount * helpersCount) + 2)
    {
        cerr << "Not enough CPU cores for this configuration!" << endl;
        return false;
    }

    for (uint8_t i = 0; i < inDevicesCount; i++)
    {
        Router *router = new Router(inDevices[i], outDevices, outDevicesCount, receiversCount, helpersCount, &queueForHelper);
        routers.push_back(router);
        for (uint8_t j = 0; j < router->receivers.size(); j++)
        {
            workers.push_back(router->receivers[j]);
        }
        for (uint8_t j = 0; j < router->helpers.size(); j++)
        {
            workers.push_back(router->helpers[j]);
        }
    }

    return true;
}

bool Controller::updateRoutesAndGRVP()
{

    if (startUp)
    {
        MYSQL_RES *executeResult = executeQuery("update Routes set Status = 'Inactive' where Status = 'Active'");
        mysql_free_result(executeResult);
        startUp = false;
    }

    MYSQL_RES *executeResult = executeQuery("select * from Routes where Status = 'Delete'");
    MYSQL_ROW row;

    if (executeResult == NULL)
        return false;

    while ((row = mysql_fetch_row(executeResult)))
    {
        string pciIdIn = row[0];
        string pciIdOut = row[1];
        uint16_t vlanIn0 = atoi(row[2]);
        uint16_t vlanIn1 = atoi(row[3]);
        uint16_t vlanOut = atoi(row[4]);

        for (uint8_t i = 0; i < routers.size(); i++)
        {
            Router *router = routers[i];
            if (router->rt->pciID.compare(pciIdIn) == 0)
            {
                if (router->rt->delRoute(pciIdIn, pciIdOut, vlanIn0, vlanIn1, vlanOut))
                    cout << "Route: " << pciIdIn << ":" << pciIdOut << ":" << vlanIn0 << ":" << vlanIn1 << ":" << vlanOut << " successfuly deleted!" << endl;
                else
                    cout << "Route: " << pciIdIn << ":" << pciIdOut << ":" << vlanIn0 << ":" << vlanIn1 << ":" << vlanOut << " deleting failed!" << endl;
            }
        }
    }

    mysql_free_result(executeResult);

    executeResult = executeQuery("select eraseDeletedRoutes();");

    if (executeResult == NULL)
        return false;

    while ((row = mysql_fetch_row(executeResult)))
    {
        string res = row[0];
        if (res.compare("0") != 0)
            cout << res << " routes deleted from database" << endl;
    }

    mysql_free_result(executeResult);

    executeResult = executeQuery("select * from Routes where Status = 'Inactive'");

    if (executeResult == NULL)
        return false;

    while ((row = mysql_fetch_row(executeResult)))
    {
        string pciIdIn = row[0];
        string pciIdOut = row[1];
        uint16_t vlanIn0 = atoi(row[2]);
        uint16_t vlanIn1 = atoi(row[3]);
        uint16_t vlanOut = atoi(row[4]);

        for (uint8_t i = 0; i < routers.size(); i++)
        {
            Router *router = routers[i];
            if (router->rt->pciID.compare(pciIdIn) == 0)
            {
                if (router->rt->addRoute(pciIdIn, pciIdOut, vlanIn0, vlanIn1, vlanOut))
                    cout << "Route: " << pciIdIn << ":" << pciIdOut << ":" << vlanIn0 << ":" << vlanIn1 << ":" << vlanOut << " successfuly added!" << endl;
                else
                    cout << "Route: " << pciIdIn << ":" << pciIdOut << ":" << vlanIn0 << ":" << vlanIn1 << ":" << vlanOut << " adding failed!" << endl;
            }
        }
    }

    mysql_free_result(executeResult);

    executeResult = executeQuery("update Routes set Status = 'Active' where Status = 'Inactive'");
    mysql_free_result(executeResult);

    for (uint8_t i = 0; i < routers.size(); i++)
    {
        Router *router = routers[i];
        RoutingTable *rt = router->rt;
        for (uint16_t i = 0; i < 4095; i++)
        {
            for (uint16_t j = 0; j < 4095; j++)
            {
                if (rt->grvpTable[i][j])
                {
                    string query = "select addGRVP('" + rt->pciID + "', " + to_string(i) + ", " + to_string(j) + ");";
                    executeResult = executeQuery(query);
                    if (executeResult == NULL)
                        return false;
                    while ((row = mysql_fetch_row(executeResult)))
                    {
                        string res = row[0];
                        if (res.compare("Success") == 0)
                            cout << "Added new grvp: " << rt->pciID << ", " << i << ", " << j << endl;
                    }
                    mysql_free_result(executeResult);
                    rt->grvpTable[i][j] = false;
                }
            }
        }
    }

    return true;
}

void Controller::printStats()
{
    cout << "Incoming interfaces" << endl
         << "---------------------" << endl
         << endl;
    for (uint8_t i = 0; i < routers.size(); i++)
    {

        DpdkDevice::DpdkDeviceStats rxStats;
        routers[i]->inDevice->device->getStatistics(rxStats);
        uint64_t totalPacketsReceived = 0;
        for (uint8_t j = 0; j < routers[i]->receivers.size(); j++)
        {
            totalPacketsReceived += routers[i]->receivers[j]->count;
        }
        uint64_t totalPacketsCalculated = 0;
        uint64_t totalPacketsDroppedByGRVP = 0;
        uint64_t totalPacketsDroppedByInvalidHeaders = 0;

        for (uint8_t j = 0; j < routers[i]->helpers.size(); j++)
        {
            totalPacketsCalculated += routers[i]->helpers[j]->count;
            totalPacketsDroppedByGRVP += routers[i]->helpers[j]->droppedByGRVP;
            totalPacketsDroppedByInvalidHeaders += routers[i]->helpers[j]->droppedByInvalidHeaders;
        }

        uint32_t currentLag = totalPacketsReceived - totalPacketsCalculated -
                              totalPacketsDroppedByInvalidHeaders - totalPacketsDroppedByGRVP;        

        cout << routers[i]->rt->pciID << endl;
        cout << '\t' << "Current speed:                            " << rxStats.aggregatedRxStats.bytesPerSec * 8 / 1000000 << " MBit/s" << endl;
        cout << '\t' << "Total packets received:                   " << totalPacketsReceived << endl;
        cout << '\t' << "Total packets calculated:                 " << totalPacketsCalculated << endl;
        cout << '\t' << "Current lag:                              " << currentLag << endl;
        cout << '\t' << "Total packets dropped by invalid headers: " << totalPacketsDroppedByInvalidHeaders << endl;
        cout << '\t' << "Total packets dropped by GRVP:            " << totalPacketsDroppedByGRVP << endl;

        cout << "---------------------" << endl;
    }
    cout << "---------------------" << endl
         << endl;
    cout << "Outcoming interfaces" << endl
         << "---------------------" << endl
         << endl;

    for (uint8_t i = 0; i < outDevicesCount; i++)
    {
        DpdkDevice::DpdkDeviceStats txStats;
        outDevices[i]->device->getStatistics(txStats);

        uint64_t totalRequests = 0;
        uint64_t totalSent = 0;

        for (uint8_t j = 0; j < outDevices[i]->counterCount; j++)
        {
            totalRequests += outDevices[i]->requests[j];
            totalSent += outDevices[i]->count[j];
        }

        cout << outDevices[i]->device->getPciAddress() << endl;
        cout << '\t' << "Current speed:           " << txStats.aggregatedTxStats.bytesPerSec * 8 / 1000000 << " MBit/s" << endl;
        cout << '\t' << "Total requests for sent: " << totalRequests << endl;
        cout << '\t' << "Total packets sent:      " << totalSent << endl;
        cout << "---------------------" << endl;
    }
}