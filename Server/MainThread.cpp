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
  SOCKET listen_socket = 0;

  sSocketCheckWithMsg
    (::WSAStartup (MAKEWORD (2, 2), &wsaData) == 0,
      "WSAStartup failed");
  {
    // Create the listening socket
    RListeningSocket listen_socket 
      (RServerSocketAddress (22),
       500);  //TODO

    CoreConnectionFactory cf;

    listen_socket.listen 
      (ConnectionFactory::instance ());
    // the function returns on the 
    // current thread stop request
  }

  ::WSACleanup (); // ignore the return code
}
