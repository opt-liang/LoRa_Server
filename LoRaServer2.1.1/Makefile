AS = ApplicationServer/
NS= NetworkServer/
NC = NetworkController/
CS = CustomerServer/
CMD = Console/
COM = Common/
VER = Version/

CRYPT_LIST = aes.o cmac.o
COMMON_LIST = $(CRYPT_LIST) Application.o ApplicationServer.o CommandLineInterface.o CommandParser.o CommandParserAppMethods.o CommandParserMethods.o CommandParserUsingGlobalData.o ConfiguredValue.o ConfiguredValueLoRa.o DebugMonitor.o GlobalData.o GlobalFunctions.o IpSocket.o IpSocketSet.o JsonCommand.o JsonGenerate.o JsonParser.o JsonReceive.o JsonReceiveIp.o JsonString.o LoRa.o LoRaApplicationDatabase.o LoRaApplicationDatabaseInterface.o LoRaCypher.o LoRaDatabase.o LoRaRegion.o LoRaUtility.o Maths.o Position.o Semaphore.o SqlDb.o Service.o ServerThreadFunctions.o TcpSocket.o TcpConnectionManager.o TcpTextSocketConnectionManager.o Thread-Posix.o TimeFunctions.o TimeRecord.o TransmissionRecord.o UdpSocket.o Utilities.o ValueWithValidity.o WordStore.o

NETWORK_SERVER_LIST = AddressAllocator.o AddressRange.o BestGateway.o CommandParserNS.o DummyFunctionsNS.o FrameReception.o GatewayMessageProtocol.o GatewayMessageProtocolReceive.o GatewayMessageProtocolReceiveNS.o GatewayNS.o GlobalDataNS.o GlobalFunctionsNS.o JoinControllerNS.o JsonReceiveNS.o LoRaDatabaseInterfaceNS.o LoRaDatabaseNS.o LoRaNS.o LoRaRegionNS.o MainNS.o MoteNS.o MoteTransmitController.o OptionController.o TransmitQueue.o WaitingFrame.o

APPLICATION_SERVER_LIST = CommandParserAS.o DummyFunctionsAS.o GlobalDataAS.o GlobalFunctionsAS.o JoinControllerAS.o JsonReceiveAS.o LoRaDatabaseAS.o LoRaDatabaseInterfaceAS.o MainAS.o MoteAS.o

CUSTOMER_SERVER_LIST = ApplicationDataOutput.o CommandParserCS.o DummyFunctionsCS.o GlobalDataCS.o GlobalFunctionsCS.o JsonReceiveCS.o LoRaDatabaseCS.o LoRaDatabaseInterfaceCS.o MainCS.o MoteCS.o

NETWORK_CONTROLLER_LIST = AdaptiveDataRateController.o CommandParserNC.o DataRateStore.o DummyFunctionsNC.o EuiReminderQueue.o GatewayNC.o GlobalDataNC.o GlobalFunctionsNC.o JsonReceiveNC.o LoRaDatabaseInterfaceNC.o LoRaDatabaseNC.o MainNC.o MonitorAlgorithm.o MoteNC.o TrivialControllerAgorithm.o

COMMAND_CONSOLE_LIST = DummyFunctionsCC.o MainCC.o PendingAcknowledgeQueue.o ServerAddressController.o

CC_COMMON_LIST = DebugMonitor.o IpSocket.o IpSocketSet.o JsonCommand.o JsonParser.o LoRa.o LoRaUtility.o Thread-Posix.o TimeFunctions.o TimeRecord.o TransmissionRecord.o UdpSocket.o Utilities.o WordStore.o

FULL_OBJECT_LIST = $(COMMON_LIST) $(NETWORK_SERVER_LIST) $(APPLICATION_SERVER_LIST) $(CUSTOMER_SERVER_LIST) $(NETWORK_CONTROLLER_LIST) $(COMMAND_CONSOLE_LIST) $(CC_COMMON_LIST) BuildVersion.o

INCLUDES = -I $(COM)
LIBRARIES =  -lrt `mysql_config --cflags` `mysql_config --libs`
DEFINES = 
VERSION_DEFINE = -DBUILD_VERSION='"R2.1.1"'

# Uncomment for debug
#DEBUG = 1
ifdef DEBUG
	OPTIMISATION = -g
else
	OPTIMISATION = -O3
endif
COMPILE_OPTIONS = -posix -fstack-check $(OPTIMISATION) -fexceptions $(DEFINES)
LINK_OPTIONS = -posix -pthread

DIR_LIST = $(COM):$(NS):$(NC):$(AS):$(CMD):$(VER):$(CS)
VPATH = $(DIR_LIST)
TARGETS = loraNS loraNC loraAS loraCS loracmd

CFLAGS=$(INCLUDES)
.PHONY: all
	

all: $(TARGETS)
	

loraNS : updateBuildTime $(NETWORK_SERVER_LIST) $(COMMON_LIST) BuildVersion.o
	g++ $(LINK_OPTIONS) -o $@ $(NETWORK_SERVER_LIST) $(COMMON_LIST) BuildVersion.o $(LIBRARIES) 

loraAS : updateBuildTime $(APPLICATION_SERVER_LIST) $(COMMON_LIST) BuildVersion.o
	g++ $(LINK_OPTIONS) -o $@ $(APPLICATION_SERVER_LIST) $(COMMON_LIST) BuildVersion.o $(LIBRARIES) 

loraNC : updateBuildTime $(NETWORK_CONTROLLER_LIST) $(COMMON_LIST) BuildVersion.o
	g++ $(LINK_OPTIONS) -o $@ $(NETWORK_CONTROLLER_LIST) $(COMMON_LIST) BuildVersion.o $(LIBRARIES) 

loraCS : updateBuildTime $(CUSTOMER_SERVER_LIST) $(COMMON_LIST) BuildVersion.o
	g++ $(LINK_OPTIONS) -o $@ $(CUSTOMER_SERVER_LIST) $(COMMON_LIST) BuildVersion.o $(LIBRARIES) 
	
loracmd : updateBuildTime $(COMMAND_CONSOLE_LIST) $(CC_COMMON_LIST) BuildVersion.o
	g++ $(LINK_OPTIONS) -o $@ $(COMMAND_CONSOLE_LIST) $(CC_COMMON_LIST) BuildVersion.o $(LIBRARIES) 

.PHONY: updateBuildTime
updateBuildTime:
	rm -f BuildVersion.o


#BuildVersionmake 
BuildVersion.o : BuildVersion.cpp
	g++ -c $(COMPILE_OPTIONS) $(INCLUDES) $(VERSION_DEFINE) $<

#Plain C++
%.o: %.cpp $(COM)*.hpp
	g++ -c $(COMPILE_OPTIONS) $(INCLUDES) $<
#	%.o: %.cpp *.hpp *.h
#	gxxx++ -c $(COMPILE_OPTIONS) $(INCLUDES) $<
#	
#C
%.o: %.c
	gcc -c $(COMPILE_OPTIONS) $(INCLUDES) $<

.PHONY: clean
clean:
	rm -f $(FULL_OBJECT_LIST) $(TARGETS)


