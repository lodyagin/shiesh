/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dknet.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 */

/* $Id$ */

#pragma once

#include "defines.h"

namespace ssh {

#if defined(HAVE_MD5_PASSWORDS) && !defined(HAVE_MD5_CRYPT)

int is_md5_salt(const char *);
std::string md5_crypt(const char *, const char *);

#endif /* defined(HAVE_MD5_PASSWORDS) && !defined(HAVE_MD5_CRYPT) */

}
