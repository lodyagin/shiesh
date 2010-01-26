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

