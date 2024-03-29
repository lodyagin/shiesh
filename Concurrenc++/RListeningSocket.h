/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Cohors Concurro library.

  This library is free software: you can redistribute
  it and/or modify it under the terms of the GNU Lesser General
  Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License
  for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/
#pragma once
#include "RSocketGroup.h"
#include "RServerSocketAddress.h"
#include "ConnectionFactory.h"
#include "SThread.h"
#include <winsock2.h>
#include "Logging.h"
#include "RListeningSocket.h"
#include "RConnectedSocket.h"
#include <Ws2tcpip.h>
#include "SWinCheck.h"
#include <algorithm>


template<class ConnectionFactory>
class RListeningSocket : public RSocketGroup
{
public:
  // Create and bind the server socket
  RListeningSocket 
    (const RServerSocketAddress& addr,
     unsigned int backlog);

  ~RListeningSocket ();

  // Listen and generate new connections
  void listen (ConnectionFactory& cf);

protected:

  // Called by constructor
  void bind (const RServerSocketAddress& addr,
             unsigned int backlog);

  WSAEVENT *events;

private:
  static Logging log;
};

template<class ConnectionFactory>
Logging RListeningSocket<ConnectionFactory>::log ("RListeningSocket");


//FIXME! close sockets on errors
template<class ConnectionFactory>
RListeningSocket<ConnectionFactory>::RListeningSocket
  (const RServerSocketAddress& addr,
   unsigned int backlog)
   : events (0)
{
  // create server sockets for each address and bind them
  bind (addr, backlog);

  //Create event objects
  events = new WSAEVENT [sockets.size () + 2];
  // TODO check <= 64

  // Start event recording
  int i = 0;
  for (Group::iterator it = sockets.begin ();
       it != sockets.end ();
       it++
       )
  {
     events[i] = ::WSACreateEvent ();
     if (events[i] == WSA_INVALID_EVENT)
      THROW_EXCEPTION 
        (SException, L"WSACreateEvent call failed");

    // Start recording of socket events
    sSocketCheckWithMsg 
     (::WSAEventSelect 
        (*it, events[i], FD_ACCEPT)
      != SOCKET_ERROR, L" on WSAEventSelect (FD_ACCEPT)");
    i++;
  }
  // the last for shutdown
  events[i++] = SThread::current().get_stop_event ()
    .evt ();
  events[i] = 0;
}

template<class ConnectionFactory>
RListeningSocket<ConnectionFactory>::~RListeningSocket ()
{
  for (Group::size_type k = 0; k < sockets.size (); k++)
    ::WSACloseEvent (events[k]); //TODO check ret

  delete [] events;
}


template<class ConnectionFactory>
void RListeningSocket<ConnectionFactory>::bind 
  (const RServerSocketAddress& addr,
   unsigned int backlog)
{
  // bind for each discovered server address
  for (RMultiprotoSocketAddress::const_iterator ai =
         addr.begin ();
       ai != addr.end ();
       ai++)
  {
    SOCKET s = ::socket 
      (ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (s == INVALID_SOCKET)
    {
      LOG4STRM_WARN 
        (log.GetLogger (),
         oss_ << "Create socket failed for "
              << (*ai);
         );
      continue;
    }

    // set non blocking
    u_long mode = 0;  
    if (SOCKET_ERROR == ioctlsocket(s, FIONBIO, &mode))
    {
      ::shutdown (s, SD_BOTH); 
      ::closesocket (s);
      continue;
    }

    // Set reuse address
    const int on = 1;
    sSocketCheck 
      (::setsockopt
        (s,
         SOL_SOCKET,
         SO_REUSEADDR,
         (const char*) &on,
         sizeof (on)
         ) == 0
       );

    if (SOCKET_ERROR == ::bind
         (s, 
          ai->ai_addr, 
          ai->ai_addrlen)
          )
    {
      LOG4STRM_WARN 
        (log.GetLogger (),
         oss_ << "Bind socket failed for "
              << (*ai)
         );
      ::shutdown (s, SD_BOTH); 
      ::closesocket (s);
      continue;
    }

    sockets.push_back (s);

    sSocketCheck (::listen (s, (int) backlog) == 0);

    LOG4STRM_INFO
      (log.GetLogger (),
       oss_ << "Start listen for connections at "
       //<< ::inet_ntoa (saddr.sin_addr)
       //<< ':' << ::htons (saddr.sin_port)
            << (*ai);
       );
  }
}

template<class ConnectionFactory>
void RListeningSocket<ConnectionFactory>::listen (ConnectionFactory& cf)
{
  //TODO keepalive option

  events[sockets.size () + 1] = cf.connectionTerminated.evt ();

  sockaddr_in6 sa;

  while (1) // loop for producing new connections
  {
    DWORD waitResult = ::WSAWaitForMultipleEvents 
      (sockets.size () + 2,
       events,
       FALSE, // "OR" wait
       WSA_INFINITE,
       FALSE );

    int evNum = waitResult - WSA_WAIT_EVENT_0;

    if (evNum == sockets.size ()) // stop the thread
      return;

    if (evNum == sockets.size () + 1)
    {
      cf.destroy_terminated_connections ();
    }
    else
    {
      ::WSAResetEvent (events[evNum]);

      SOCKET s = 0;
      int sa_len = sizeof (sa);

      LOG4STRM_INFO
        (log.GetLogger (),
         oss_ << "Ready for accept on the server socket " 
         << waitResult);

      sSocketCheckWithMsg 
        ((s = ::accept 
           (sockets.at (waitResult), 
            (sockaddr*) &sa, 
            &sa_len)) != INVALID_SOCKET,
            "when accept"
         );

      LOG4STRM_DEBUG 
        (log.GetLogger (), 
         oss_ << "accept returns the new connection: ";
         RSocketAddress::outString (oss_, (sockaddr*) &sa)
         );

      cf.create_new_connection 
        (new RConnectedSocket (s, true)); //FIXME memleak ?
    }
  }
}
