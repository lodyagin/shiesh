#include "stdafx.h"
#include "MainThread.h"
#include "RListeningSocket.h"
#include "SWinCheck.h"

void MainThread::run ()
{
  WSADATA wsaData;
  SOCKET listen_socket = 0;

  sSocketCheck
    (::WSAStartup (MAKEWORD (2, 2), &wsaData) == 0,
      "WSAStartup failed");
  {
    // Create the listening socket
    RListeningSocket listen_socket 
      (RServerSocketAddress (22));

    ConnectionFactory cf;

    listen_socket.listen 
      (500,  //TODO
       ConnectionFactory::instance ());
    // the function returns on the 
    // current thread stop request
  }

  ::WSACleanup (); // ignore the return code
}
