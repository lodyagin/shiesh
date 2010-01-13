#pragma once
#include "defines.h"
#include "xmalloc.h"
#include "log.h"
#include "CoreConnection.h"
#include "SException.h"
#include "Logging.h"
#include "SCommon.h"
#include <winsock2.h>
#include <string>

#define SIZE_T_MAX MAXSIZE_T

#define snprintf _snprintf

#include "Ws2tcpip.h" 