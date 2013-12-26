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
#include "AuthDispatcher.h"
#include "ssh2.h"
#include "CoreConnection.h"

AuthDispatcher::AuthDispatcher (CoreConnection* con)
: Dispatcher (con)
{
  dispatch[SSH2_MSG_SERVICE_REQUEST] = 
    &Dispatcher::service_request_msg;
}

void AuthDispatcher::service_request_msg
  (int type, u_int32_t seq, void *ctxt)
{
  connection->input_service_request
    (type, seq, ctxt);
}

void AuthDispatcher::userauth_request_msg 
  (int type, u_int32_t seq, void *ctxt)
{
  connection->input_userauth_request
    (type, seq, ctxt);
}

void AuthDispatcher::enable_userauth_request_msg ()
{
  dispatch[SSH2_MSG_USERAUTH_REQUEST] =
    &Dispatcher::userauth_request_msg;
}

void AuthDispatcher::disable_userauth_request_msg ()
{
  dispatch[SSH2_MSG_USERAUTH_REQUEST] =
    &Dispatcher::ignored_msg;
}

void AuthDispatcher::unexpected_msg 
  (int type, u_int32_t seq, void * ctxt)
{
  protocol_error (type, seq, ctxt);
}
