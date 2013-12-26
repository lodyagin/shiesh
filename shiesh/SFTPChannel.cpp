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

