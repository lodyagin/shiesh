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
#pragma once

#include "Logging.h"

#define DISPATCH_MIN	0
#define DISPATCH_MAX	255

class CoreConnection;

class Dispatcher
{
public:
  static Logging log;
  
  enum {
	  DISPATCH_BLOCK,
	  DISPATCH_NONBLOCK
  };

  Dispatcher(CoreConnection* con);
  //virtual ~Dispatcher(void);

  void dispatch_run(int, volatile bool *, void *);

  // functions for messages
  virtual void kexinit_msg (int, u_int32_t, void *) = 0;
  virtual void service_request_msg (int, u_int32_t, void *) = 0;
  virtual void userauth_request_msg (int, u_int32_t, void *) = 0;
  virtual void channel_close_msg (int, u_int32_t, void *) = 0;
  virtual void channel_data_msg (int, u_int32_t, void *) = 0;
  virtual void channel_eof_msg (int, u_int32_t, void *) = 0;
  virtual void channel_extended_data_msg (int, u_int32_t, void *) = 0;
  virtual void channel_open_msg (int, u_int32_t, void *) = 0;
  virtual void channel_open_confirmation_msg (int, u_int32_t, void *) = 0;
  virtual void channel_open_failure_msg (int, u_int32_t, void *) = 0;
  virtual void channel_request_msg (int, u_int32_t, void *) = 0;
  virtual void channel_window_adjust_msg (int, u_int32_t, void *) = 0;
  virtual void global_request_msg (int, u_int32_t, void *) = 0;
  virtual void channel_success_msg (int, u_int32_t, void *) = 0;
  virtual void channel_failure_msg (int, u_int32_t, void *) = 0;
  virtual void request_success_msg (int, u_int32_t, void *) = 0;
  virtual void request_failure_msg (int, u_int32_t, void *) = 0;

  // Called when msg should be ignored
  virtual void ignored_msg (int, u_int32_t, void *);

  static std::vector<std::string> msgNames;

protected:

  // Called when unecpected msg arrived
  virtual void unexpected_msg 
    (int, u_int32_t, void *);

  // helper function
  void protocol_error (int, u_int32_t, void *);

  typedef void (Dispatcher::*dispatch_fn) 
    (int, u_int32_t, void *);
  
  CoreConnection* connection;

  dispatch_fn dispatch[DISPATCH_MAX];

};


