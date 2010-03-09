#include "StdAfx.h"
#include "SFTPChannel.h"
#include "misc.h"
#include "sftp-common.h"

SFTPChannel::SFTPChannel (const SessionChannel& from)
  : SessionChannel (from)
{
  from.thisChannelUpgraded = true;
}

bool SFTPChannel::is_complete_packet_in_descending ()
{
  u_int buf_len = buffer_len (descending);

  if (buf_len < 5) return false;

  u_char* cp = (u_char*) buffer_ptr(descending);
  u_int msg_len = get_u32(cp); //sftp part length
  if (msg_len > SFTP_MAX_MSG_LENGTH) {
	  error("bad message from local user ?"/*,
      pw->userName*/);
	  //FIXME close the channel
  }
  if (buf_len < msg_len + 4) return false;

  // there is complete sftp packet in the buffer
  return true;
}

void SFTPChannel::subproc2ascending ()
{
  toChannel->get 
    (ascending, true /* sftp fromat get - string */); 
  // put full size message in ascending
}

void SFTPChannel::descending2subproc ()
{
  if (!is_complete_packet_in_descending ())
  {
    // get consumed only
    fromChannel->put (0, 0, &local_consumed);
    return;
  }

  const u_int msg_len = buffer_get_int (descending);

  // it decrease c->local_consumed on data already
  // on a "subsystem processor" part
  // <NB> consume is regarded to another transactions
  fromChannel->put 
    (buffer_ptr (descending), 
     msg_len, 
     &local_consumed
     );
  assert (local_consumed >= 0);
 
  if (local_consumed) 
    local_consumed += 4; // for the buf_len field red above

  /* discard the remaining bytes from the current packet */
  buffer_consume(descending, msg_len);
}

