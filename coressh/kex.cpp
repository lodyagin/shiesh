/* $OpenBSD: kex.c,v 1.79 2007/06/05 06:52:37 djm Exp $ */
/*
 * Copyright (c) 2000, 2001 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "StdAfx.h"
#include "key.h"
#include "kex.h"
#include "ssh2.h"
#include <openssl/crypto.h>


namespace coressh {

#if OPENSSL_VERSION_NUMBER >= 0x00907000L
# if defined(HAVE_EVP_SHA256)
# define evp_ssh_sha256 EVP_sha256
# else
extern const EVP_MD *evp_ssh_sha256(void);
# endif
#endif

/* prototype */
static void kex_kexinit_finish(Kex *);
static void kex_choose_conf(Kex *);

/* ARGSUSED */
void
kex_protocol_error(int type, u_int32_t seq, void *ctxt)
{
	error("Hm, kex protocol error: type %d seq %u", type, seq);
}

#if defined(DEBUG_KEX) || defined(DEBUG_KEXDH)
void
dump_digest(char *msg, u_char *digest, int len)
{
	u_int i;

	fprintf(stderr, "%s\n", msg);
	for (i = 0; i < len; i++) {
		fprintf(stderr, "%02x", digest[i]);
		if (i%32 == 31)
			fprintf(stderr, "\n");
		else if (i%8 == 7)
			fprintf(stderr, " ");
	}
	fprintf(stderr, "\n");
}
#endif

}
