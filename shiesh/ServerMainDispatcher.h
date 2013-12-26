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
#include "dispatcher.h"

class ServerMainDispatcher : public Dispatcher
{
public:
  ServerMainDispatcher (CoreConnection* con);

protected:
  // Overrides
  void kexinit_msg (int, u_int32_t, void *);
  void channel_close_msg (int, u_int32_t, void *);
  void channel_data_msg (int, u_int32_t, void *);
  void channel_eof_msg (int, u_int32_t, void *);
  void channel_extended_data_msg (int, u_int32_t, void *);
  void channel_open_msg (int, u_int32_t, void *);
  void channel_open_confirmation_msg (int, u_int32_t, void *);
  void channel_open_failure_msg (int, u_int32_t, void *);
  void channel_request_msg (int, u_int32_t, void *);
  void channel_window_adjust_msg (int, u_int32_t, void *);
  void global_request_msg (int, u_int32_t, void *);
  void channel_success_msg (int, u_int32_t, void *);
  void channel_failure_msg (int, u_int32_t, void *);
  void request_success_msg (int, u_int32_t, void *);
  void request_failure_msg (int, u_int32_t, void *);
  void unexpected_msg 
    (int, u_int32_t, void *);

  // unused
  void service_request_msg (int, u_int32_t, void *) {}
  void userauth_request_msg (int, u_int32_t, void *) {}

};
