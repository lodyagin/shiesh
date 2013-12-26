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


