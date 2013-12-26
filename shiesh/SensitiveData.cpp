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

