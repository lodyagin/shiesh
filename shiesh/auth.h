/* $OpenBSD: auth.h,v 1.61 2008/07/02 12:03:51 dtucker Exp $ */

/*
 * Copyright (c) 2000 Markus Friedl.  All rights reserved.
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
 *
 */

#pragma once

#include "buffer.h"
#include "key.h"
#include "User.h"
#include "hostfile.h"

#include <openssl/rsa.h>

#ifdef HAVE_LOGIN_CAP
#include <login_cap.h>
#endif
#ifdef BSD_AUTH
#include <bsd_auth.h>
#endif
#ifdef KRB5
#include <krb5.h>
#endif

class CoreConnection;

namespace ssh {

typedef struct Authctxt Authctxt;
typedef struct Authmethod Authmethod;
typedef struct KbdintDevice KbdintDevice;

// FIXME check access from diff places
struct Authctxt {
  Authctxt () 
  {
    ::memset (this, 0, sizeof Authctxt); // FIXME
  }
  
  ~Authctxt ()
  {
    //delete pw; //FIXME
  }

	volatile bool	 success;
	int		 authenticated;	/* authenticated and alarms cancelled */
	int		 postponed;	/* authentication needs another step */
	int		 valid;		/* user exists and is allowed to login */
	int		 attempt;
	int		 failures;
	int		 server_caused_failure; 
	int		 force_pwchange;
	char		*user;		/* username sent by the client */
	char		*service;
	User  	*pw;		/* set if 'valid' */
	char		*style;
	char		*role;
	void		*kbdintctxt;
#ifdef BSD_AUTH
	auth_session_t	*as;
#endif
#ifdef KRB5
	krb5_context	 krb5_ctx;
	krb5_ccache	 krb5_fwd_ccache;
	krb5_principal	 krb5_user;
	char		*krb5_ticket_file;
	char		*krb5_ccname;
#endif
	Buffer		*loginmsg;
	void		*methoddata;
};
/*
 * Every authentication method has to handle authentication requests for
 * non-existing users, or for users that are not allowed to login. In this
 * case 'valid' is set to 0, but 'user' points to the username requested by
 * the client.
 */

typedef int	(CoreConnection::*authmethod_fn)(Authctxt *authctxt);
struct Authmethod {
  /*Authmethod ()
    : name (0), userauth (0), enabled (0)
  {}*/

  Authmethod (const char* n, authmethod_fn ua, int* en)
    : name (n), userauth (ua), enabled (en)
  {}

	const char	*name;
  authmethod_fn userauth;
	int	*enabled;
};

/*
 * Keyboard interactive device:
 * init_ctx	returns: non NULL upon success
 * query	returns: 0 - success, otherwise failure
 * respond	returns: 0 - success, 1 - need further interaction,
 *		otherwise - failure
 */
struct KbdintDevice
{
	const char *name;
	void*	(*init_ctx)(Authctxt*);
	int	(*query)(void *ctx, char **name, char **infotxt,
		    u_int *numprompts, char ***prompts, u_int **echo_on);
	int	(*respond)(void *ctx, u_int numresp, char **responses);
	void	(*free_ctx)(void *ctx);
};

int      auth_rhosts(User *, const char *);
int
auth_rhosts2(User *, const char *, const char *, const char *);

int	 auth_rhosts_rsa(Authctxt *, char *, Key *);
int      auth_password(Authctxt *, const char *);
int      auth_rsa(Authctxt *, BIGNUM *);
int      auth_rsa_challenge_dialog(Key *);
BIGNUM	*auth_rsa_generate_challenge(Key *);
int	 auth_rsa_verify_response(Key *, BIGNUM *, u_char[]);
int	 auth_rsa_key_allowed(User *, BIGNUM *, Key **);

int	 auth_rhosts_rsa_key_allowed(User *, char *, char *, Key *);
int	 hostbased_key_allowed(User *, const char *, char *, Key *);
int	 user_key_allowed(User *, Key *);

#ifdef KRB5
int	auth_krb5(Authctxt *authctxt, krb5_data *auth, char **client, krb5_data *);
int	auth_krb5_tgt(Authctxt *authctxt, krb5_data *tgt);
int	auth_krb5_password(Authctxt *authctxt, const char *password);
void	krb5_cleanup_proc(Authctxt *authctxt);
#endif /* KRB5 */

#if defined(USE_SHADOW) && defined(HAS_SHADOW_EXPIRE)
#include <shadow.h>
int auth_shadow_acctexpired(struct spwd *);
int auth_shadow_pwexpired(Authctxt *);
#endif

//#include "auth-pam.h"
//#include "audit.h"
void remove_kbdint_device(const char *);

#if 0
void disable_forwarding(void);
#endif

//void	do_authentication(Authctxt *);

//void	auth_log(Authctxt *, int, char *, char *); //FIXME
void	userauth_finish(Authctxt *, int, char *);
void	userauth_send_banner(const char *);
int	auth_root_allowed(char *);

#if 0
char	*auth2_read_banner(void);
#endif

void	privsep_challenge_enable(void);

int	auth2_challenge(Authctxt *, char *);
void	auth2_challenge_stop(Authctxt *);
int	bsdauth_query(void *, char **, char **, u_int *, char ***, u_int **);
int	bsdauth_respond(void *, u_int, char **);
int	skey_query(void *, char **, char **, u_int *, char ***, u_int **);
int	skey_respond(void *, u_int, char **);

int	allowed_user(User *);
//User * getpwnamallow(const char *user); //FIXME

char	*get_challenge(Authctxt *);
int	verify_response(Authctxt *, const char *);
void	abandon_challenge_response(Authctxt *);

char	*authorized_keys_file(User *);
char	*authorized_keys_file2(User *);

FILE	*auth_openkeyfile(const char *, User *, int);

HostStatus
check_key_in_hostfiles(User *, Key *, const char *,
    const char *, const char *);

int	reject_blacklisted_key(Key *, int);

/* hostkey handling */
Key	*get_hostkey_by_index(int);
Key	*get_hostkey_by_type(int);
int	 get_hostkey_index(Key *);
int	 ssh1_session_key(BIGNUM *);

/* debug messages during authentication */
void	 auth_debug_add(const char *fmt,...);
void	 auth_debug_send(void);
void	 auth_debug_reset(void);

//User *fakepw(void); 

int	 sys_auth_passwd(Authctxt *, const char *);

#define AUTH_FAIL_MSG "Too many authentication failures for %.100s"

#define SKEY_PROMPT "\nS/Key Password: "

#if defined(KRB5) && !defined(HEIMDAL)
#include <krb5.h>
krb5_error_code ssh_krb5_cc_gen(krb5_context, krb5_ccache *);
#endif

}

