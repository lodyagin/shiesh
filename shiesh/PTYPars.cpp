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
#include "PTYPars.h"
#include "PTY.h"
#include "packet.h"
#include "CoreConnection.h"

PTYPars::PTYPars
  (const char* _name/*,
   CoreConnection* con*/)
: ChannelRequestPars (_name/*, con*/),
  col (0), row (0), xpixel (0), ypixel (0)
{
}

PTY* PTYPars::create_derivation 
  (const Repository<PTY, PTYPars>::
   ObjectCreationInfo& info
   ) const
{
  return new PTY 
    (info.objectId, term, col, row);
}

void PTYPars::read_from_packet (CoreConnection* con)
{
	u_int len;
	int n_bytes;

#if 0 // FIXME
	if (s->ttyfd != -1) {
		packet_disconnect("Protocol error: you already have a pty.");
		return 0;
	}
#endif

	const char* term2 = con-> packet_get_string(&len);
  term = term2;
  xfree ((void*) term2);

  col = con-> packet_get_int();
	row = con-> packet_get_int();
	xpixel = con-> packet_get_int();
	ypixel = con-> packet_get_int();

#if 0
	/* Allocate a pty and open it. */
	debug("Allocating pty.");
	if (!PRIVSEP(pty_allocate(&s->ptyfd, &s->ttyfd, s->tty,
	    sizeof(s->tty)))) {
		if (s->term)
			xfree(s->term);
		s->term = NULL;
		s->ptyfd = -1;
		s->ttyfd = -1;
		error("session_pty_req: session %d alloc failed", s->self);
		return 0;
	}
	debug("session_pty_req: session %d alloc %s", s->self, s->tty);
#endif

  PTY::tty_parse_modes(con, &n_bytes);

#if 0
  /* Set window size from the packet. */
	pty_change_window_size(s->ptyfd, s->row, s->col, s->xpixel, s->ypixel);
#endif

	packet_check_eom (con);
}

void PTYPars::outString (std::ostream& out) const
{
  LOG4STRM_DEBUG
    (Logging::Root (),
     oss_ << "<> PTYPars : col = " << col
     << ", row = " << row
     << ", xpixel = " << xpixel
     << ", ypixel = " << ypixel
     << ", TERM = [" << term << ']'
     );
}
