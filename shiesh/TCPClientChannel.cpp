/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (c) 2009-2013, Sergei Lodyagin
  All rights reserved.

  Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that the
  following conditions are met:

  1. Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

  2. Redistributions in binary form must reproduce the
  above copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.

*/
#include "StdAfx.h"
#include "TCPClientChannel.h"
#include "ssh2.h"
#include "CoreConnection.h"

TCPClientChannel::TCPClientChannel
 (const std::string& channelType,
  const std::string& channelId,
  CoreConnection* connection,
  const std::string& _hostToConnect,
  int _portToConnect
  )
: SessionChannel
  (channelType,
   channelId,
   WindowDefaultSize,
   connection
   ),
   csa (0),
   isConnecting (true)
{
  std::string port;
  ::toString (_portToConnect, port);

  csa = new RClientSocketAddress  // FIXME check alloc
    (_hostToConnect.c_str (), port.c_str ());
}

TCPClientChannel::~TCPClientChannel ()
{
  delete csa;
}

void TCPClientChannel::channel_post ()
{
  SessionChannel::channel_post ();
  if (isConnecting)
  {
		con->packet_start
      (SSH2_MSG_CHANNEL_OPEN_CONFIRMATION);
		con->packet_put_int (get_remote_id ());
		con->packet_put_int (self);
		con->packet_put_int (get_local_window ());
		con->packet_put_int (get_local_maxpacket ());
		con->packet_send ();

    isConnecting = false;
  }
}

Logging TCPClient::log ("TCPClient");

TCPClient::TCPClient
  (const std::string &objectId,
   User *const _pw, 
   BusyThreadWriteBuffer<Buffer>* in,
   BusyThreadReadBuffer<Buffer>* out,
   SEvent* terminatedSignal,
   int _channelId,
   const RClientSocketAddress& _csa,
   TCPClientChannel* _channel
   )
: Subsystem 
    (objectId, _pw, in, out, 
     terminatedSignal, _channelId
     ),
  socket (0),
  csa (&_csa),
  channel (_channel)
{
}

void TCPClient::run ()
{
	char buf[16384];

  LOG4CXX_INFO
    (log.GetLogger (),
     L"TCPClient: start socket connecting"
     );
  socket = connector.connect_first (*csa);
  //socket->set_blocking (false);
  LOG4CXX_INFO
    (log.GetLogger (),
     L"TCPClient: socket is connected"
     );

  channel->open ();
  toChannel->dataReady.set ();

  void* inputMsg = 0;
  u_int32_t inputMsgLen;

  HANDLE evts[1] = {
    socket->get_event_object ()
  };

  try
  {
	  for (;;) 
    {
      DWORD socketEvts = socket->get_events 
        (true); // reset the event object

      if (socketEvts & FD_READ)
      {
        int len = ::recv 
          (socket->get_socket (),
           buf,
           sizeof (buf),
           0);
        if (len == 0)
        {
          LOG4CXX_INFO
            (log.GetLogger (),
             L"TCPClient: connection is closed "
             L"by a socket server"
            );
          if (inputMsg) 
            ssh::xfree (inputMsg);
          return; // FIXME check inputMsg free
        }
        else if (len < 0)
        {
          const int err = ::WSAGetLastError ();
			    if (err != WSAEINTR &&
			        err != WSAEWOULDBLOCK) 
          {
            THROW_EXCEPTION 
              (SException,
               oss_ <<  L"Read error from a socket server: "
                    << sWinErrMsg (err)
               );
          }
        }
        else 
          toChannel->put (buf, len);
      }
      if (socketEvts & FD_CLOSE)
      {
          LOG4CXX_INFO
            (log.GetLogger (),
             L"TCPClient: connection is closed "
             L"by a socket server"
            );
          if (inputMsg) 
            ssh::xfree (inputMsg);
          return; // FIXME check inputMsg free
      }

      // wait until an input message arrived
      inputMsg = fromChannel->get (&inputMsgLen, evts, 1);

      if (inputMsg && inputMsgLen == 0) 
      {
        ssh::xfree (inputMsg); inputMsg = 0;
        assert (fromChannel->n_msgs_in_the_buffer () == 0);

        if (fromChannel->n_msgs_in_the_buffer () != 0)
        {
          LOG4CXX_ERROR 
            (Logging::Root (),
            L"Program error: EOF received but the input buffer"
            L" contains more data to process. The Shell session"
            L" will not be closed (resource leak).");
          continue;
        }

        //FIXME Flush all buffers
		    return; // exit the thread
	    } 

      if (inputMsg)
      {
        socket->atomicio_send (inputMsg, inputMsgLen); // FIXME!!! (blocking)
        ssh::xfree (inputMsg); inputMsg = 0;
      }
    }
  }
  catch (...)
  {
    if (inputMsg) 
      ssh::xfree (inputMsg);
    throw;
  }

}

void TCPClient::start ()
{
  Subsystem::start ();
}

Subsystem* TCPClientPars::create_derivation 
  (const Repository<Subsystem, SubsystemPars>::
   ObjectCreationInfo& info
   ) const
{
  // FIXME check alloc
  return new TCPClient
    (info.objectId,
     pw,
     inBuffer,
     outBuffer,
     subsystemTerminated,
     channelId,
     *channel->csa,
     channel
     );
}