/* $OpenBSD: serverloop.c,v 1.153 2008/06/30 12:15:39 djm Exp $ */
/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Server main loop for handling the interactive session.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 * SSH2 support by Markus Friedl.
 * Copyright (c) 2000, 2001 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "StdAfx.h"
#include "CoreConnection.h"
#include "Channel.h"
#include "ChannelPars.h"
#include "Session.h"
#include "SessionPars.h"
#include "packet.h"
#include "ssh2.h"

class DescendingProbe : public std::unary_function<void, Channel&>
{
public:
  DescendingProbe (CoreConnection& _con/*, bool sigs[]*/) 
    : con (_con), 
      signalled (false), 
      needCheckConsumed (false)
  {}

  void operator () (Channel& c)
  {
    //sigs[c.self] = 
    bool sig =
      buffer_len (&c.descending) > 0
      && c.is_complete_packet_in_descending ();

    if (sig)
      signalled = true;

    // check for local window here, if it -> 0
    // we must periodically check "consumed" quantity 
    // in fromChannel buffer
    // in beleave "subsystem process" is making its work.
    // But we shouldn't do it very often, thus prepeare all for wait
    if (c.get_local_window () < c.get_local_maxpacket ())
    {
      //sigs[c.self] = true;
      needCheckConsumed = true;
    }
  }

  bool signalled;
  bool needCheckConsumed;
protected:
  CoreConnection& con;
};

void
CoreConnection::wait_until_can_do_something
  (HANDLE eventArray[], 
   int nEvents,
   u_int max_time_milliseconds,
   bool signalled[]
   )
{

  u_int timeout = (max_time_milliseconds != 0) 
         ? max_time_milliseconds : WSA_INFINITE;

	/*int client_alive_scheduled = 0;
	int program_alive_scheduled = 0;*/

#if 0 //FIXME should be enabled
  /*
	 * if using client_alive, set the max timeout accordingly,
	 * and indicate that this particular timeout was for client
	 * alive by setting the client_alive_scheduled flag.
	 *
	 * this could be randomized somewhat to make traffic
	 * analysis more difficult, but we're not doing it yet.
	 */
	if (/*compat20*/ true &&
	    max_time_milliseconds == 0 && options.client_alive_interval) {
		client_alive_scheduled = 1;
		max_time_milliseconds = options.client_alive_interval * 1000;
	}
#endif

      //FIXME! channel_not_very_much_buffered_data condition !

	//notify_prepare(*readsetp); //notify pipe

	/*
	 * If child has terminated and there is enough buffer space to read
	 * from it, then read as much as is available and exit.
	 */ // TODO
	/*if (child_terminated && packet_not_very_much_data_to_write())
		if (max_time_milliseconds == 0 || client_alive_scheduled)
			max_time_milliseconds = 100;*/

  memset (signalled, 0, nEvents * sizeof (bool));

  DescendingProbe probe (*this);
  ChannelRepository::for_each (probe);
  if (probe.needCheckConsumed)
    timeout = MAX (timeout, 5000); // TODO explicit value

  if (!socket->wait_fd_write ()
    && buffer_len(&output) > 0) // can write
  {
    signalled[SocketEvt] = true;
  }
  
  if (!(probe.signalled|| signalled[SocketEvt])) 
    // no events were found to this moment
  {
    //waitResult = //::WSAWaitForMultipleEvents
      ::WaitForMultipleObjectsEx
      (nEvents, eventArray, FALSE, /* wait any*/
       timeout,
        FALSE
        );
  }

  DWORD waitResult;
  int arrayOffset = 0;
  while (nEvents - arrayOffset > 0
    && (waitResult = ::WSAWaitForMultipleEvents
         (nEvents - arrayOffset, 
          eventArray + arrayOffset, 
          FALSE, /* wait any*/
          0,
          FALSE
         )) != WSA_WAIT_TIMEOUT) // FIXME check condition
  {
    const int eventNum = waitResult - WSA_WAIT_EVENT_0 + arrayOffset;
    
    if (eventNum < 0 || eventNum >= nEvents)
        THROW_EXCEPTION (SException, 
           L" bad result of events waiting");

    signalled[eventNum] = true;
#if 0
    debug ("wait_until_can_do_something: event %d is signalled",
      (int) eventNum);
#endif
    if (eventNum == SocketEvt || eventNum == SubsystemTermEvt) 
      ::WSAResetEvent (eventArray[eventNum]);
      // for channel messages do reset in BusyThreadReadBuffer
      // after no data in buffer
    arrayOffset = eventNum + 1;
  }

  //notify_done(*readsetp); //notify pipe
}

/*
 * Processes input from the client.  Input data is stored
 * in buffers and processed later.
 */
void
CoreConnection::process_input(long networkEvents)
{
	int len;
	char buf[16384];

	/* Read and buffer any input data from the client. */
	if (networkEvents & FD_READ) {
    len = ::recv(socket->get_socket (), buf, sizeof(buf), 0);
		if (len == 0) {
			verbose("Connection closed by %.100s",
			    socket->get_peer_address ().get_ip ().c_str ());
			connection_closed = true;
		} 
    else if (len < 0) 
    {
      const int err = ::WSAGetLastError ();
			if (err != WSAEINTR &&
			    err != WSAEWOULDBLOCK) 
      {
				verbose("Read error from remote host "
				    "%.100s: %.100s",
            socket->get_peer_address ().get_ip ().c_str (),
            sWinErrMsg (err).c_str ());
        // FIXME UNICODE
				cleanup_exit(255);
			}
		} else {
			/* Buffer any received data. */
			packet_process_incoming(buf, len);
		}
	}
}

/*
 * Sends data from internal buffers to client.
 */
void
CoreConnection::process_output(/*long networkEvents*/)
{
	/* Send any buffered packet data to the client. */
	//if (networkEvents & FD_WRITE)
		packet_write_poll();
}

Channel *
CoreConnection::server_request_session 
  (const ChannelPars& chPars)
{
	Channel *c;

	debug("input_session_request");
	packet_check_eom(this);

	if (no_more_sessions) {
		packet_disconnect("Possible attack: attempt to open a session "
		    "after additional sessions disabled");
	}

  // create a channel
  c = ChannelRepository::create_object (chPars);

  // create a session associated with the channel
  SessionPars sessPars;
  sessPars.authctxt = authctxt;
  sessPars.chanid = c->self;
  sessPars.connection = this;
  (void) SessionRepository::create_object (sessPars); 
  //TODO session creation fail conditions from OpenSSH
  //{
	//	debug("session open failed, free channel %d", c->self);
	//	channel_free(c);
	//	return NULL;
	//}

  // FIXME: check this:
	/*
	 * A server session has no fd to read or write until a
	 * CHANNEL_REQUEST for a shell is made, so we set the type to
	 * SSH_CHANNEL_LARVAL.  Additionally, a callback for handling all
	 * CHANNEL_REQUEST messages is registered.
	 */

	//channel_register_cleanup(c->self, session_close_by_channel, 0);
	return c;
}

void 
CoreConnection::server_input_channel_open
  (int type, u_int32_t seq, void *ctxt)
{
	Channel *c = NULL;
	char *ctype;
	//int rchan;
	u_int /*rmaxpack, rwindow,*/ len;
  SessionChannelPars chPars;

	ctype = packet_get_string(&len);
  chPars.remote_id = packet_get_int();
	chPars.rwindow = packet_get_int();
	chPars.rmaxpack = packet_get_int();
  chPars.connection = this;

	debug("server_input_channel_open: ctype %s rchan %d win %d max %d",
	    ctype, 
      (int) chPars.remote_id, 
      (int) chPars.rwindow, 
      (int) chPars.rmaxpack
      );

	if (strcmp(ctype, "session") == 0) {
		c = server_request_session(chPars);
	} 
  /*else if (strcmp(ctype, "direct-tcpip") == 0) {
		c = server_request_direct_tcpip();
	} */
	if (c != NULL) {
		debug("server_input_channel_open: confirm %s", ctype);
		// FIXME if (c->type != SSH_CHANNEL_CONNECTING) {
			packet_start(SSH2_MSG_CHANNEL_OPEN_CONFIRMATION);
			packet_put_int(c->get_remote_id ());
			packet_put_int(c->self);
			packet_put_int(c->get_local_window ());
			packet_put_int(c->get_local_maxpacket ());
			packet_send();
		//}
	/*} else {
		debug("server_input_channel_open: failure %s", ctype);
		packet_start(SSH2_MSG_CHANNEL_OPEN_FAILURE);
		packet_put_int(rchan);
		packet_put_int(SSH2_OPEN_ADMINISTRATIVELY_PROHIBITED);
		if (!(datafellows & SSH_BUG_OPENFAILURE)) {
			packet_put_cstring("open failed");
			packet_put_cstring("");
		}
		packet_send();*/
	}
	xfree(ctype);
}

#if 0
static void
server_input_global_request(int type, u_int32_t seq, void *ctxt)
{
	char *rtype;
	int want_reply;
	int success = 0;

	rtype = packet_get_string(NULL);
	want_reply = packet_get_char();
	debug("server_input_global_request: rtype %s want_reply %d", rtype, want_reply);

	/* -R style forwarding */
	if (strcmp(rtype, "tcpip-forward") == 0) {
		struct passwd *pw;
		char *listen_address;
		u_short listen_port;

		pw = the_authctxt->pw;
		if (pw == NULL || !the_authctxt->valid)
			fatal("server_input_global_request: no/invalid user");
		listen_address = packet_get_string(NULL);
		listen_port = (u_short)packet_get_int();
		debug("server_input_global_request: tcpip-forward listen %s port %d",
		    listen_address, listen_port);

		/* check permissions */
		if (!options.allow_tcp_forwarding ||
		    no_port_forwarding_flag
#ifndef NO_IPPORT_RESERVED_CONCEPT
		    || (listen_port < IPPORT_RESERVED && pw->pw_uid != 0)
#endif
		    ) {
			success = 0;
			packet_send_debug("Server has disabled port forwarding.");
		} else {
			/* Start listening on the port */
			success = channel_setup_remote_fwd_listener(
			    listen_address, listen_port, options.gateway_ports);
		}
		xfree(listen_address);
	} else if (strcmp(rtype, "cancel-tcpip-forward") == 0) {
		char *cancel_address;
		u_short cancel_port;

		cancel_address = packet_get_string(NULL);
		cancel_port = (u_short)packet_get_int();
		debug("%s: cancel-tcpip-forward addr %s port %d", __func__,
		    cancel_address, cancel_port);

		success = channel_cancel_rport_listener(cancel_address,
		    cancel_port);
		xfree(cancel_address);
	} else if (strcmp(rtype, "no-more-sessions@openssh.com") == 0) {
		no_more_sessions = 1;
		success = 1;
	}
	if (want_reply) {
		packet_start(success ?
		    SSH2_MSG_REQUEST_SUCCESS : SSH2_MSG_REQUEST_FAILURE);
		packet_send();
		packet_write_wait();
	}
	xfree(rtype);
}
#endif

void
CoreConnection::server_input_channel_req
  (int type, u_int32_t seq, void *ctxt)
{
	Channel *c;
	int id, reply, success = 0;
	char *rtype;

	id = packet_get_int();
	rtype = packet_get_string(NULL);
	reply = packet_get_char();

	debug("server_input_channel_req: channel %d request %s reply %d",
	    id, rtype, reply);

  //TODO doesn't check private channels (see OpenSSH)
  if ((c = ChannelRepository::get_object_by_id (id)) == NULL)
		packet_disconnect("server_input_channel_req: "
		    "unknown channel %d", id);
	
  if (
      (
       c->channelStateIs ("larval") ||
	     c->channelStateIs ("open")
       ) 
       && c->ctype == "session"
      )
  {
   	Session *s = get_session_by_channel(c->self);
    if (!s)
		  logit("session_input_channel_req: no session %d req %.100s",
		    (int) c->self, rtype);
    else
      success = s->session_input_channel_req 
        (this, c, rtype);
  }

  // FIXME implement other types of request
  // Some request should be ignored without failure.

	if (reply) 
  {
		packet_start(success ?
		    SSH2_MSG_CHANNEL_SUCCESS : SSH2_MSG_CHANNEL_FAILURE);
		packet_put_int(c->get_remote_id ());
		packet_send();
	}
	xfree(rtype);
}

