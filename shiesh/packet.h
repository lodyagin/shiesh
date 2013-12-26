/* $OpenBSD: packet.h,v 1.49 2008/07/10 18:08:11 markus Exp $ */

/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Interface for the packet protocol functions.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

#pragma once

#include <openssl/bn.h>

namespace ssh {


//void	 tty_make_modes(int, struct termios *);
//void	 tty_parse_modes(int, int *);

//extern u_int max_packet_size;
//extern int keep_alive_timeouts;
//int	 packet_set_maxsize(u_int);
#define  packet_get_maxsize() max_packet_size

/* don't allow remaining bytes after the end of the message */
#define packet_check_eom(clas) \
do { \
	int _len = (clas)->packet_remaining(); \
	if (_len > 0) { \
		logit("Packet integrity error (%d bytes remaining) at %s:%d", \
		    _len ,__FILE__, __LINE__); \
		(clas)->packet_disconnect("Packet integrity error."); \
	} \
} while (0)

//int	 packet_need_rekeying(void);
//void	 packet_set_rekey_limit(u_int32_t);

}

