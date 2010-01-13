// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <stdio.h>
#include <tchar.h>

#define _WIN32_WINNT 0x0400
// TODO: reference additional headers your program requires here

#include "Logging.h"
#include "SCheck.h"
#include "SCommon.h"
#include "SMutex.h"
#include "SThread.h"
#include "SException.h"
#include "SShutdown.h"
#include <iostream>
