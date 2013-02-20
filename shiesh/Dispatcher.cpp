#include "StdAfx.h"
#include "Dispatcher.h"
#include "CoreConnection.h"
#include "ssh2.h"

// maps msg name <-> code
struct MsgInfo
{
  const char* name;
  unsigned code;
};

const static MsgInfo msgInfoArr[] =
{
  {"MSG_NONE",				0},
  {"MSG_DISCONNECT",				1},
  {"MSG_IGNORE",					2},
  {"MSG_UNIMPLEMENTED",				3},
  {"MSG_DEBUG",					4},
  {"MSG_SERVICE_REQUEST",			5},
  {"MSG_SERVICE_ACCEPT",				6},
  {"MSG_KEXINIT",				20},
  {"MSG_NEWKEYS",				21},
  {"MSG_KEXDH_INIT",				30},
  {"MSG_KEXDH_REPLY",				31},

  {"MSG_USERAUTH_REQUEST",			50},
  {"MSG_USERAUTH_FAILURE",			51},
  {"MSG_USERAUTH_SUCCESS",			52},
  {"MSG_USERAUTH_BANNER",			53},

  {"MSG_GLOBAL_REQUEST",				80},
  {"MSG_REQUEST_SUCCESS",			81},
  {"MSG_REQUEST_FAILURE",			82},

  {"MSG_CHANNEL_OPEN",				90},
  {"MSG_CHANNEL_OPEN_CONFIRMATION",		91},
  {"MSG_CHANNEL_OPEN_FAILURE",			92},
  {"MSG_CHANNEL_WINDOW_ADJUST",			93},
  {"MSG_CHANNEL_DATA",				94},
  {"MSG_CHANNEL_EXTENDED_DATA",			95},
  {"MSG_CHANNEL_EOF",				96},
  {"MSG_CHANNEL_CLOSE",				97},
  {"MSG_CHANNEL_REQUEST",			98},
  {"MSG_CHANNEL_SUCCESS",			99},
  {"MSG_CHANNEL_FAILURE",			100}
};

Logging Dispatcher::log ("Dispatcher");

std::vector<std::string> Dispatcher::msgNames;

Dispatcher::Dispatcher(CoreConnection* con)
: connection (con)
{
  assert (connection);

  for (int i = 0; i < DISPATCH_MAX; i++)
    dispatch[i] = &Dispatcher::unexpected_msg;

  if (msgNames.size () == 0)
  {
    for (
      int i = sizeof (msgInfoArr) / sizeof (MsgInfo) - 1; 
      i >= 0;
      i--
      )
    {
      const unsigned k = msgInfoArr[i].code;
      if (msgNames.size () < k + 1) 
        msgNames.resize (k + 1);
      msgNames[k] = msgInfoArr[i].name;
    }
  }
}

void Dispatcher::unexpected_msg (int type, u_int32_t, void *)
{
  connection->packet_disconnect
    ("protocol error: rcvd type %d", type);
}

void Dispatcher::ignored_msg (int type, u_int32_t seq, void *)
{
  logit ("protocol_ignore: type %d seq %u", type, seq);
}


void Dispatcher::dispatch_run
  (int mode, volatile bool *done, void *ctxt)
{
	for (;;) {
		int type;
		u_int32_t seqnr;

		if (mode == DISPATCH_BLOCK) 
    {
			type = connection->packet_read_seqnr(&seqnr);
		} 
    else 
    {
			type = connection->packet_read_poll_seqnr
        (&seqnr);

			if (type == SSH_MSG_NONE)
				return;
		}
		if (type > 0 
        && type < DISPATCH_MAX 
        && dispatch[type] != NULL
        )
    {
      LOG4STRM_DEBUG
        (log.GetLogger (),
         oss_ << " .> " << msgNames[type]);

			(this->*(dispatch[type]))(type, seqnr, ctxt);
    }
		else
			unexpected_msg (type, seqnr, ctxt);
		if (done != NULL && *done)
			return;
	}
}

void Dispatcher::protocol_error(int type, u_int32_t seq, void *ctxt)
{
	logit("dispatch_protocol_error: type %d seq %u", type, seq);
	connection->packet_start(SSH2_MSG_UNIMPLEMENTED);
	connection->packet_put_int(seq);
	connection->packet_send();
	connection->packet_write_wait();
}
