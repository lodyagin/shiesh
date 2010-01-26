#include "StdAfx.h"
#include "Dispatcher.h"
#include "CoreConnection.h"
#include "ssh2.h"

Dispatcher::Dispatcher(CoreConnection* con)
: connection (con)
{
  assert (connection);

  for (int i = 0; i < DISPATCH_MAX; i++)
    dispatch[i] = &Dispatcher::unexpected_msg;
}

/*Dispatcher::~Dispatcher(void)
{
}*/

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
		if (type > 0 && type < DISPATCH_MAX && dispatch[type] != NULL)
			(this->*(dispatch[type]))(type, seqnr, ctxt);
		else
			unexpected_msg (type, seqnr, ctxt);
		if (done != NULL && *done)
			return;
	}

}
