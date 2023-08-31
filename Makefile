include /usr/local/etc/PcapPlusPlus.mk

# All Target
all:
	g++ $(PCAPPP_BUILD_FLAGS) -c $(PCAPPP_INCLUDES) -c -o build/Receiver.o Receiver.cpp
	g++ $(PCAPPP_BUILD_FLAGS) -c $(PCAPPP_INCLUDES) -c -o build/Structs.o Structs.cpp
	g++ $(PCAPPP_BUILD_FLAGS) -c $(PCAPPP_INCLUDES) -I/usr/include/mysql -c -o build/Controller.o Controller.cpp
	g++ $(PCAPPP_BUILD_FLAGS) -c $(PCAPPP_INCLUDES) -c -o build/Router.o Router.cpp
	g++ $(PCAPPP_BUILD_FLAGS) -c $(PCAPPP_INCLUDES) -c -o build/OutDevice.o OutDevice.cpp
	g++ $(PCAPPP_BUILD_FLAGS) -c $(PCAPPP_INCLUDES) -c -o build/InDevice.o InDevice.cpp
	g++ $(PCAPPP_BUILD_FLAGS) -c $(PCAPPP_INCLUDES) -c -o build/RoutingTable.o RoutingTable.cpp
	g++ $(PCAPPP_BUILD_FLAGS) -c $(PCAPPP_INCLUDES) -c -o build/Helper.o Helper.cpp
	g++ $(PCAPPP_BUILD_FLAGS) -c $(PCAPPP_INCLUDES) -c -o build/main.o main.cpp
	g++ $(PCAPPP_LIBS_DIR) -L/usr/include/mysql  -static-libstdc++ -o build/shaper build/Receiver.o build/Structs.o build/Controller.o build/Router.o build/RoutingTable.o build/OutDevice.o build/InDevice.o build/Helper.o build/main.o $(PCAPPP_LIBS) -lmysqlclient

# Clean Target
clean:
	rm build/main.o
	rm build/Receiver.o
	rm build/Controller.o
	rm build/Router.o
	rm build/InDevice.o
	rm build/OutDevice.o
	rm build/RoutingTable.o
	rm build/Structs.o
	rm build/Helper.o
	rm build/shaper
