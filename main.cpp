#include "DpdkDeviceList.h"
#include "Structs.h"
#include "Controller.h"

#define MBUF_POOL_SIZE 512 * 1024 - 1

bool keepRunning = true;
Controller *controller;

void onApplicationInterrupted(void *cookie)
{
    cout << endl
         << "Shutting down..." << endl;
    for (uint8_t i = 0; i < controller->routers.size(); i++)
    {
        for (uint8_t j = 0; j < controller->routers.at(i)->helpers.size(); j++)
        {
            controller->routers.at(i)->helpers.at(j)->m_Stop = true;
        }

        for (uint8_t j = 0; j < controller->routers.at(i)->receivers.size(); j++)
        {
            controller->routers.at(i)->receivers.at(j)->m_Stop = true;
        }
    }
    DpdkDeviceList::getInstance().stopDpdkWorkerThreads();
    controller->~Controller();
    exit(0);
}

int main(int argc, char *argv[])
{

    ApplicationEventHandler::getInstance().onApplicationInterrupted(onApplicationInterrupted, NULL);

    CoreMask coreMaskToUse = getCoreMaskForAllMachineCores();
    DpdkDeviceList::initDpdk(coreMaskToUse, MBUF_POOL_SIZE);

    vector<DpdkDevice *> deviceList = DpdkDeviceList::getInstance().getDpdkDeviceList();

    cout << endl
         << "Existing devices" << endl
         << "---------------------" << endl;

    for (vector<DpdkDevice *>::iterator iter = deviceList.begin(); iter != deviceList.end(); iter++)
    {
        DpdkDevice *dev = *iter;
        cout << "   "
             << " Port #" << dev->getDeviceId() << ":"
             << " MAC address='" << dev->getMacAddress() << "';"
             << " PCI address='" << dev->getPciAddress() << "';"
             << " PMD='" << dev->getPMDName() << "'"
             << endl;
    }

    cout << endl
         << endl
         << "---------------------" << endl;

    map<string, string> config;

    ifstream file("config.txt");
    string line;

    while (getline(file, line))
    {
        istringstream iss(line);
        string key, value;
        iss >> key >> value;
        config[key] = value;
    }

    file.close();

    if (config["host"].empty())
    {
        cerr << "Host can not be empty!" << endl;
        return 1;
    }
    if (config["username"].empty())
    {
        cerr << "Username can not be empty!" << endl;
        return 1;
    }
    if (config["password"].empty())
    {
        cerr << "Password can not be empty!" << endl;
        return 1;
    }
    if (config["dbName"].empty())
    {
        cerr << "Database name can not be empty!" << endl;
        return 1;
    }
    if (config["receiversCount"].empty())
    {
        cerr << "Receivers count can not be empty!" << endl;
        return 1;
    }
    if (config["helpersCount"].empty())
    {
        cerr << "Helpers count can not be empty!" << endl;
        return 1;
    }

    int receiversCount = atoi(config["receiversCount"].c_str());
    int helpersCount = atoi(config["helpersCount"].c_str());

    controller = new Controller(config["host"], config["username"], config["password"], config["dbName"], receiversCount, helpersCount);

    if (!controller->connect())
    {
        cerr << "Fail with connection to database!" << endl;
        return 1;
    }

    if (!controller->createInterfacesVector())
    {
        cerr << "Fail with create interfaces list!" << endl;
        controller->~Controller();
        return 1;
    }

    if (!controller->initDevices(deviceList))
    {
        cerr << "Fail with initialize interfaces!" << endl;
        controller->~Controller();
        return 1;
    }

    if (!controller->initRouters())
    {
        cerr << "Fail with initialize routers!" << endl;
        controller->~Controller();
        return 1;
    }

    int workersCoreMask = 0;
    for (size_t i = 1; i <= controller->workers.size(); i++)
    {
        workersCoreMask = workersCoreMask | (1 << i);
    }

    if (!DpdkDeviceList::getInstance().startDpdkWorkerThreads(workersCoreMask, controller->workers))
    {
        std::cerr << "Couldn't start worker threads" << std::endl;
        return 1;
    }

    while (keepRunning)
    {
        sleep(1);

        cout << "\033[2J\033[1;1H";
        controller->printStats();
        controller->updateRoutesAndGRVP();
    }

    return 0;
}