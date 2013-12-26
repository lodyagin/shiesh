/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Shielded Shell. 
  The Shielded Shell (ShieSH) is a port of Open SSH to
  Windows operating system.

  This library is free software: you can redistribute
  it and/or modify it under the terms of the GNU General
  Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General
  Public License along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
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
