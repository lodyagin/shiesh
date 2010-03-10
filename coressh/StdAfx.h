#pragma once
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN // without winsock 1
#include <windows.h>
#include <Ws2tcpip.h>
#include <Winsock2.h>

#if (_WIN32_WINNT < 0x0600)
#include "FileExtd.h"
#endif

#include "defines.h"
#include "ssh.h"
#include "xmalloc.h"
#include "log.h"
#include "SException.h"
#include "Logging.h"
#include "SCommon.h"
#include "SWinCheck.h"
#include "zlib.h"
#include "port.h"

#include <string>
#include <openssl/ossl_typ.h>
#include <assert.h>
