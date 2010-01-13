/* $OpenBSD: atomicio.h,v 1.10 2006/08/03 03:34:41 deraadt Exp $ */

/*
 * Copyright (c) 2006 Damien Miller.  All rights reserved.
 * Copyright (c) 1995,1999 Theo de Raadt.  All rights reserved.
 * All rights reserved.
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

namespace coressh {

/*
 * Ensure all of data on socket comes through. f==::send
 */
size_t	atomicio_send
  (int (__stdcall *f) (SOCKET, const char *, int, int), 
   SOCKET fd, void *_s, size_t n/*, int flags*/);

/*
 * Ensure all of data on socket comes through. f==::recv
 */
size_t	atomicio_recv
  (int (__stdcall *f) (SOCKET, char *, int, int), 
   SOCKET fd, void *_s, size_t n/*, int flags*/);

//#define vwrite (ssize_t (*)(int, void *, size_t))write

/*
 * ensure all of data on socket comes through. f==readv || f==writev
 */
//size_t	atomiciov(ssize_t (*)(int, const struct iovec *, int),
//    int, const struct iovec *, int);

};


