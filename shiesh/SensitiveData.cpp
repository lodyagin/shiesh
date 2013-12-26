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
#include "SensitiveData.h"
#include "buffer.h"
#include "authfile.h"

using namespace ssh;

char* SensitiveData::host_key_files[num_host_key_files] =
{
  "c:\\ssh\\data\\host_key_dsa",
  "c:\\ssh\\data\\host_key_rsa" //FIXME
};

SensitiveData::SensitiveData(void)
  : //server_key (0),
    host_keys (0),
    have_ssh2_key (false)
{
  load ();
}

SensitiveData::~SensitiveData(void)
{
	int i;

#if 0
	if (server_key) {
		key_free(server_key);
		server_key = NULL;
	}
#endif
	for (i = 0; i < num_host_key_files; i++) {
		if (host_keys[i]) {
			key_free((Key*) host_keys[i]);
			host_keys[i] = NULL;
		}
	}
#if 0
	ssh1_host_key = NULL;
	memset(ssh1_cookie, 0, SSH_SESSION_KEY_LENGTH);
#endif
}

char *
SensitiveData::list_hostkey_types(void)
{
	Buffer b;
	const char *p;
	char *ret;
	int i;

	buffer_init(&b);
	for (i = 0; i < num_host_key_files; i++) {
		Key *key = host_keys[i];
		if (key == NULL)
			continue;
		switch (key->type) {
		case KEY_RSA:
		case KEY_DSA:
			if (buffer_len(&b) > 0)
				buffer_append(&b, ",", 1);
			p = key_ssh_name(key);
			buffer_append(&b, p, strlen(p));
			break;
		}
	}
	buffer_append(&b, "\0", 1);
	ret = xstrdup((const char*) buffer_ptr(&b));
	buffer_free(&b);
	debug("list_hostkey_types: %s", ret);
	return ret;
}

Key *
SensitiveData::get_hostkey_by_type(int type)
{
	int i;

	for (i = 0; i < num_host_key_files; i++) {
		Key *key = host_keys[i];
		if (key != NULL && key->type == type)
			return key;
	}
	return NULL;
}

Key *
SensitiveData::get_hostkey_by_index(int ind)
{
	if (ind < 0 || ind >= num_host_key_files)
		return (NULL);
	return (host_keys[ind]);
}

int
SensitiveData::get_hostkey_index(Key *key)
{
	int i;

	for (i = 0; i < num_host_key_files; i++) {
		if (key == host_keys[i])
			return (i);
	}
	return (-1);
}

void
SensitiveData::load ()
{
  int i;
  Key* key = 0;

	/* load private host keys */
	host_keys = (Key**) xcalloc(num_host_key_files,
		sizeof(Key *));
	for (i = 0; i < num_host_key_files; i++)
		host_keys[i] = NULL;

	for (i = 0; i < num_host_key_files; i++) {
		key = key_load_private(host_key_files[i], "", NULL);
		host_keys[i] = key;
		if (key == NULL) {
			error("Could not load host key: %s",
				host_key_files[i]);
			host_keys[i] = NULL;
			continue;
		}
		/*if (reject_blacklisted_key(key, 1) == 1) {
			key_free(key);
			host_keys[i] = NULL;
			continue;
		}*/ //TODO
		switch (key->type) {
		case KEY_RSA1:
			/*ssh1_host_key = key;
			have_ssh1_key = 1;*/
      fatal ("SSH1 is not supported (found RSA1 hostkey)");
			break;
		case KEY_RSA:
		case KEY_DSA:
			have_ssh2_key = 1;
			break;
		}
		debug("private host key: #%d type %d %s", i, key->type,
			key_type(key));
	}
	/*if ((options.protocol & SSH_PROTO_1) && !have_ssh1_key) {
		logit("Disabling protocol version 1. Could not load host key");
		options.protocol &= ~SSH_PROTO_1;
	}*/

	if (!have_ssh2_key) {
		fatal("sshd: no hostkeys available -- exiting.");
	}

}

