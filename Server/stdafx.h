#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN // without winsock 1
#include <windows.h>
#include <Ws2tcpip.h>
#include <Winsock2.h>

#include "Logging.h"
#include "SCheck.h"
#include "SCommon.h"
#include "SMutex.h"
#include "SThread.h"
#include "SException.h"
#include "SShutdown.h"

#include <iostream>
#include <stdio.h>
#include <tchar.h>

//#include "sqlite3.h"

