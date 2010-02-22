#include "StdAfx.h"
#include "PTY.h"
#include "packet.h"

PTY::PTY (CoreConnection* _con) : con (_con)
{
  assert (con);
}

PTY::~PTY(void)
{
}

bool PTY::session_pty_req ()
{
	u_int len;
	int n_bytes;

#if 0 // FIXME
	if (s->ttyfd != -1) {
		packet_disconnect("Protocol error: you already have a pty.");
		return 0;
	}
#endif

	const char* term = con->packet_get_string(&len);

  u_int col = con->packet_get_int();
	u_int row = con->packet_get_int();
	u_int xpixel = con->packet_get_int();
	u_int ypixel = con->packet_get_int();

	if (strcmp(term, "") == 0) {
		xfree((void*) term);
		term = NULL;
	}

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

	tty_parse_modes(/*s->ttyfd,*/ &n_bytes);

#if 0
  /* Set window size from the packet. */
	pty_change_window_size(s->ptyfd, s->row, s->col, s->xpixel, s->ypixel);
#endif

	packet_check_eom (con);
	return true;
}

