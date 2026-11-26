// Sim replacement for FreeRTOS+TCP's FreeRTOS_IP.h: the socket API on top of Linux sockets.
// The firmware (ts_link.cpp, ota.cpp) compiles unchanged; no TCP/IP stack, pcap or root needed.
// The simulated ECU listens on the host (default 127.0.0.1, override with ECU_SIM_BIND).
#pragma once
#include "FreeRTOS_Sockets.h"
