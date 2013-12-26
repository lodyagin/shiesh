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
