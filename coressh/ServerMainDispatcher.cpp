#include "StdAfx.h"
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

void ServerMainDispatcher::kexinit_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::channel_close_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::channel_data_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::channel_eof_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::channel_extended_data_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::channel_open_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::channel_open_confirmation_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::channel_open_failure_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::channel_request_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::channel_window_adjust_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::global_request_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::channel_success_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::channel_failure_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::request_success_msg (int, u_int32_t, void *) {}
void ServerMainDispatcher::request_failure_msg (int, u_int32_t, void *) {}

void ServerMainDispatcher::unexpected_msg 
  (int type, u_int32_t seq, void * ctxt)
{
  protocol_error (type, seq, ctxt);
}
