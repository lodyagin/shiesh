#include "stdafx.h"
#include "MainThread.h"
#include "RListeningSocket.h"
#include "SWinCheck.h"
#include "CoreConnectionFactory.h"
#include "SensitiveData.h"
#include <openssl/evp.h>

void MainThread::run ()
{
	SSLeay_add_all_algorithms();

  // The singleton
  // FIXME reenterab. everywere
  SensitiveData* sensitive =
    new SensitiveData;

  WSADATA wsaData;

  sSocketCheckWithMsg
    (::WSAStartup (MAKEWORD (2, 2), &wsaData) == 0,
      "WSAStartup failed");
  {
    // Create the listening socket
    RListeningSocket<CoreConnectionFactory> 
    listen_socket (RServerSocketAddress (22),
         500);  //TODO

    CoreConnectionFactory cf (this); 
    // use this thread as parent of the connected threads

    listen_socket.listen (cf);
    // the function returns on the 
    // current thread stop request
  }

  ::WSACleanup (); // ignore the return code
}
