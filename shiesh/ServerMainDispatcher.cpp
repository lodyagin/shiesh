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
#include "CoreConnection.h"
#include "ServerMainDispatcher.h"
#include "ssh2.h"

ServerMainDispatcher::ServerMainDispatcher(CoreConnection* con)
: Dispatcher (con)
{
	dispatch[SSH2_MSG_CHANNEL_CLOSE]
    = &Dispatcher::channel_close_msg;
	dispatch[SSH2_MSG_CHANNEL_DATA]
    = &Dispatcher::channel_data_msg;
	dispatch[SSH2_MSG_CHANNEL_EOF]
    = &Dispatcher::channel_eof_msg;
	dispatch[SSH2_MSG_CHANNEL_EXTENDED_DATA]
    = &Dispatcher::channel_extended_data_msg;
	dispatch[SSH2_MSG_CHANNEL_OPEN]
    = &Dispatcher::channel_open_msg;
	dispatch[SSH2_MSG_CHANNEL_OPEN_CONFIRMATION]
    = &Dispatcher::channel_open_confirmation_msg;
	dispatch[SSH2_MSG_CHANNEL_OPEN_FAILURE]
    = &Dispatcher::channel_open_failure_msg;
	dispatch[SSH2_MSG_CHANNEL_REQUEST]
    = &Dispatcher::channel_request_msg;
	dispatch[SSH2_MSG_CHANNEL_WINDOW_ADJUST]
    = &Dispatcher::channel_window_adjust_msg;
	dispatch[SSH2_MSG_GLOBAL_REQUEST]
    = &Dispatcher::global_request_msg;
	dispatch[SSH2_MSG_CHANNEL_SUCCESS]
    = &Dispatcher::channel_success_msg;
	dispatch[SSH2_MSG_CHANNEL_FAILURE]
    = &Dispatcher::channel_failure_msg;

  /* client_alive */
	dispatch[SSH2_MSG_REQUEST_SUCCESS]
    = &Dispatcher::request_success_msg;
	dispatch[SSH2_MSG_REQUEST_FAILURE]
    = &Dispatcher::request_failure_msg;
	/* rekeying */
	dispatch[SSH2_MSG_KEXINIT]
    = &Dispatcher::kexinit_msg;
}

void ServerMainDispatcher::kexinit_msg 
  (int type, u_int32_t seq, void *ctxt) 
{ 
  connection->kex_input_kexinit (type, seq, ctxt);
}

void ServerMainDispatcher::channel_close_msg 
  (int type, u_int32_t seq, void *ctxt) 
{ 
  connection->channel_input_oclose (type, seq, ctxt);
}

void ServerMainDispatcher::channel_data_msg 
  (int type, u_int32_t seq, void *ctxt) 
{
  connection->channel_input_data (type, seq, ctxt);
}

void ServerMainDispatcher::channel_eof_msg
  (int type, u_int32_t seq, void *ctxt) 
{
  connection->channel_input_eof (type, seq, ctxt);
}

void ServerMainDispatcher::channel_extended_data_msg (int, u_int32_t, void *){  error ("ignored channel extended data msg"); }

void ServerMainDispatcher::channel_open_msg 
  (int type, u_int32_t seq, void *ctxt) 
{
  connection->server_input_channel_open (type, seq, ctxt);
}

void ServerMainDispatcher::channel_open_confirmation_msg 
  (int, u_int32_t, void *) 
{ error ("ignored channel open confirmation msg"); }

void ServerMainDispatcher::channel_open_failure_msg 
  (int, u_int32_t, void *) 
{ error ("ignored channel open failure msg"); }

void ServerMainDispatcher::channel_request_msg 
  (int type, u_int32_t seq, void *ctxt) 
{
  connection->server_input_channel_req (type, seq, ctxt);
}

void ServerMainDispatcher::channel_window_adjust_msg 
  (int type, u_int32_t seq, void *ctxt) 
{ 
  connection->channel_input_window_adjust (type, seq, ctxt);
}

void ServerMainDispatcher::global_request_msg 
  (int type, u_int32_t seq, void *ctxt) 
{  
  connection->server_input_global_request (type, seq, ctxt); 
}

void ServerMainDispatcher::channel_success_msg (int, u_int32_t, void *) 
{ error ("ignored channel success msg"); }

void ServerMainDispatcher::channel_failure_msg (int, u_int32_t, void *) 
{ error ("ignored channel failure msg"); }

void ServerMainDispatcher::request_success_msg (int, u_int32_t, void *) 
{ error ("ignored request success msg"); }

void ServerMainDispatcher::request_failure_msg (int, u_int32_t, void *) 
{ error ("ignored request failure msg"); }

void ServerMainDispatcher::unexpected_msg 
  (int type, u_int32_t seq, void * ctxt)
{
  protocol_error (type, seq, ctxt);
}
