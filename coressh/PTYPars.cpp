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
  return new PTY (info.objectId);
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
