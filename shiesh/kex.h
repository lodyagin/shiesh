/* $OpenBSD: kex.h,v 1.46 2007/06/07 19:37:34 pvalchev Exp $ */

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
#pragma once

#include "cipher.h"
#include "buffer.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include "key.h"
#include "SensitiveData.h"

class CoreConnection;

namespace ssh {

#define	KEX_DH1			"diffie-hellman-group1-sha1"
#define	KEX_DH14		"diffie-hellman-group14-sha1"
#define	KEX_DHGEX_SHA1		"diffie-hellman-group-exchange-sha1"
#define	KEX_DHGEX_SHA256	"diffie-hellman-group-exchange-sha256"

#define COMP_NONE	0
#define COMP_ZLIB	1
#define COMP_DELAYED	2

enum kex_init_proposals {
	PROPOSAL_KEX_ALGS,
	PROPOSAL_SERVER_HOST_KEY_ALGS,
	PROPOSAL_ENC_ALGS_CTOS,
	PROPOSAL_ENC_ALGS_STOC,
	PROPOSAL_MAC_ALGS_CTOS,
	PROPOSAL_MAC_ALGS_STOC,
	PROPOSAL_COMP_ALGS_CTOS,
	PROPOSAL_COMP_ALGS_STOC,
	PROPOSAL_LANG_CTOS,
	PROPOSAL_LANG_STOC,
	PROPOSAL_MAX
};

enum kex_modes {
	MODE_IN,
	MODE_OUT,
	MODE_MAX
};

enum kex_exchange {
	KEX_DH_GRP1_SHA1,
	KEX_DH_GRP14_SHA1,
	KEX_DH_GEX_SHA1,
	KEX_DH_GEX_SHA256,
	KEX_GSS_GRP1_SHA1,
	KEX_GSS_GRP14_SHA1,
	KEX_GSS_GEX_SHA1,
	KEX_MAX
};

#define KEX_INIT_SENT	0x0001

#define KEX_COOKIE_LEN	16


typedef struct Kex Kex;
typedef struct Mac Mac;
typedef struct Comp Comp;
typedef struct Enc Enc;
typedef struct Newkeys Newkeys;

struct Enc {
	char	*name;
	Cipher	*cipher;
	int	enabled;
	u_int	key_len;
	u_int	block_size;
	u_char	*key;
	u_char	*iv;
};
struct Mac {
	char	*name;
	int	enabled;
	u_int	mac_len;
	u_char	*key;
	u_int	key_len;
	int	type;
	const EVP_MD	*evp_md;
	HMAC_CTX	evp_ctx;
	struct umac_ctx *umac_ctx;
};
struct Comp {
	int	type;
	int	enabled;
	char	*name;
};
struct Newkeys {
	Enc	enc;
	Mac	mac;
	Comp	comp;
};

//typedef Key* (SensitiveData::*load_host_key_fn) (int);

struct Kex {
	u_char	*session_id;
	u_int	session_id_len;
	Newkeys	*newkeys[MODE_MAX];
	u_int	we_need;
	int	server;
	char	*name;
	int	hostkey_type;
	int	kex_type;
	Buffer	my;
	Buffer	peer;

  //an operation is completed
	volatile bool done;

	int	flags;
	const EVP_MD *evp_md;
#ifdef GSSAPI
	int	gss_deleg_creds;
	int	gss_trust_dns;
	char    *gss_host;
#endif
  std::string client_version_string;
  std::string server_version_string;
	int	(*verify_host_key)(Key *);
  Key	*(SensitiveData::*load_host_key)(int);
  //load_host_key_fn load_host_key;
	int	(SensitiveData::*host_key_index)(Key *);
  void	(CoreConnection::*kex[KEX_MAX])(Kex *);
};

void
kex_dh_hash
  (const char *, 
   const char *, 
   char *, int, char *, int, u_char *, int,
    BIGNUM *, BIGNUM *, BIGNUM *, u_char **, u_int *);

void
kexgex_hash(const EVP_MD *, const char *, const char *, char *, int, char *,
    int, u_char *, int, int, int, int, BIGNUM *, BIGNUM *, BIGNUM *,
    BIGNUM *, BIGNUM *, u_char **, u_int *);

#if defined(DEBUG_KEX) || defined(DEBUG_KEXDH)
void	dump_digest(char *, u_char *, int);
#endif

}

