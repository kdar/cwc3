# =========================================================================
#     This makefile was generated by
#     Bakefile 0.2.6 (http://www.bakefile.org)
#     Do not modify, all changes will be overwritten!
# =========================================================================



# -------------------------------------------------------------------------
# These are configurable options:
# -------------------------------------------------------------------------

# C++ compiler 
CXX = g++

# Standard flags for C++ 
CXXFLAGS ?= 

# Standard preprocessor flags (common for CC and CXX) 
CPPFLAGS ?= 

# Standard linker flags 
LDFLAGS ?= 

#  [debug,release]
BUILD ?= release

#  [text,cdk]
INTERFACE ?= cdk

# Location and arguments of wx-config script 
WX_CONFIG ?= wx-config

# C++ flags to use with wxWidgets code 
WX_CXXFLAGS ?= `$(WX_CONFIG) --cxxflags`



# -------------------------------------------------------------------------
# Do not modify the rest of this file!
# -------------------------------------------------------------------------

### Variables: ###

CPPDEPS = -MT$@ -MF`echo $@ | sed -e 's,\.o$$,.d,'` -MD -MP
CWC3_CXXFLAGS = $(__OPTIMIZE_FLAG_1) -D_THREAD_SAFE -pthread \
	$(__INTERFACE_DEFINES_p) $(__DEBUG_DEFINES_p) -DPLATFORM_UNIX $(DEBUG_INFO) \
	-Iinclude -Ilib/include $(__INTERFACE_INCLUDE_p) $(WX_CXXFLAGS) $(CPPFLAGS) \
	$(CXXFLAGS)
CWC3_OBJECTS =  \
	build/cwc3_Main.o \
	build/cwc3_Game.o \
	build/cwc3_Interface.o \
	build/cwc3_CommandHandler.o \
	build/cwc3_Functions.o \
	build/cwc3_Client.o \
	build/cwc3_ClientManager.o \
	build/cwc3_PacketCapture.o \
	build/cwc3_TcpKill.o \
	build/cwc3_Config.o \
	build/cwc3_SlotManager.o \
	build/cwc3_AutoRefresh.o \
	build/cwc3_Netclip.o \
	build/cwc3_Slot.o \
	build/cwc3_Database.o \
	build/cwc3_IpInfo.o \
	build/cwc3_Info.o \
	build/cwc3_Event.o \
	build/cwc3_NetEventDispatcher.o \
	build/cwc3_Command.o \
	build/cwc3_CoreCommands.o \
	build/cwc3_PhraseModule.o \
	build/cwc3_KickModule.o \
	build/cwc3_EnsureKick.o \
	build/cwc3_BanModule.o \
	build/cwc3_ExtraInfoModule.o \
	build/cwc3_InjectModule.o

### Conditionally set variables: ###

ifeq ($(BUILD),debug)
DEBUG_INFO = -ggdb
endif
ifeq ($(BUILD),release)
DEBUG_INFO = 
endif
ifeq ($(INTERFACE),cdk)
INTERFACE_LIBS = -lcdk -lncurses
endif
ifeq ($(INTERFACE),text)
INTERFACE_LIBS = 
endif
ifeq ($(BUILD),debug)
__OPTIMIZE_FLAG_1 = -O0
endif
ifeq ($(BUILD),release)
__OPTIMIZE_FLAG_1 = -O2
endif
ifeq ($(INTERFACE),cdk)
__INTERFACE_DEFINES_p = -DUSE_INTERFACE_CDK
endif
ifeq ($(INTERFACE),text)
__INTERFACE_DEFINES_p = -DUSE_INTERFACE_TEXT
endif
ifeq ($(BUILD),debug)
__DEBUG_DEFINES_p = -DDEBUG=1 -D__WXDEBUG__ -DWXUSINGDLL_BASE
endif
ifeq ($(BUILD),release)
__DEBUG_DEFINES_p = -DDEBUG=0 -DWX_DEBUG=0
endif
ifeq ($(INTERFACE),cdk)
__INTERFACE_INCLUDE_p = -I/usr/include/cdk
endif
ifeq ($(INTERFACE),text)
__INTERFACE_INCLUDE_p = 
endif


all: build
build:
	@mkdir -p build

### Targets: ###

all: build/cwc3

install: 

uninstall: 

clean: 
	rm -f build/*.o
	rm -f build/*.d
	rm -f build/cwc3

build/cwc3: $(CWC3_OBJECTS)
	$(CXX) -o $@ $(CWC3_OBJECTS)   -pthread $(DEBUG_INFO) $(LDFLAGS)  `$(WX_CONFIG) --libs net,base` -lpcap -lsqlite3 -lboost_regex -lboost_system -lnet $(INTERFACE_LIBS)
	mv build/cwc3 dist/

build/cwc3_Main.o: src/Main.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_Game.o: src/Game.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_Interface.o: src/Interface.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_CommandHandler.o: src/CommandHandler.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_Functions.o: src/Functions.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_Client.o: src/Client.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_ClientManager.o: src/ClientManager.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_PacketCapture.o: src/PacketCapture.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_TcpKill.o: src/TcpKill.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_Config.o: src/Config.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_SlotManager.o: src/SlotManager.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_AutoRefresh.o: src/AutoRefresh.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_Netclip.o: src/Netclip.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_Slot.o: src/Slot.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_Database.o: src/Database.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_IpInfo.o: src/IpInfo.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_Info.o: src/Info.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_Event.o: src/Event.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_NetEventDispatcher.o: src/NetEventDispatcher.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_Command.o: src/Command.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_CoreCommands.o: src/CoreCommands.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_PhraseModule.o: src/PhraseModule/PhraseModule.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_KickModule.o: src/KickModule/KickModule.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_EnsureKick.o: src/KickModule/EnsureKick.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_BanModule.o: src/BanModule/BanModule.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_ExtraInfoModule.o: src/ExtraInfoModule/ExtraInfoModule.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

build/cwc3_InjectModule.o: src/InjectModule/InjectModule.cpp
	$(CXX) -c -o $@ $(CWC3_CXXFLAGS) $(CPPDEPS) $<

.PHONY: all install uninstall clean


# Dependencies tracking:
-include build/*.d
