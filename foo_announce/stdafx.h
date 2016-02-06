// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// need to include these here, otherwise foobar's header will
// force us to use WinSock 1. 
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "../../foobar2000/SDK/foobar2000.h"

#include "../../foobar2000/ATLHelpers/ATLHelpers.h"

// TODO: reference additional headers your program requires here
