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

