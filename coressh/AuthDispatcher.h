// Realizes dispatches in "pre-auth" state.

#pragma once

#include "Dispatcher.h"

class AuthDispatcher : public Dispatcher
{
public:
  AuthDispatcher (CoreConnection* con);

  void enable_userauth_request_msg ();
  void disable_userauth_request_msg ();

protected:
  // Overrides
  void service_request_msg (int, u_int32_t, void *);
  void userauth_request_msg (int, u_int32_t, void *);

  void unexpected_msg 
    (int, u_int32_t, void *);

  void kexinit_msg (int, u_int32_t, void *) {}
  void channel_close_msg (int, u_int32_t, void *) {}
  void channel_data_msg (int, u_int32_t, void *) {}
  void channel_eof_msg (int, u_int32_t, void *) {}
  void channel_extended_data_msg (int, u_int32_t, void *) {}
  void channel_open_msg (int, u_int32_t, void *) {}
  void channel_open_confirmation_msg (int, u_int32_t, void *) {}
  void channel_open_failure_msg (int, u_int32_t, void *) {}
  void channel_request_msg (int, u_int32_t, void *) {}
  void channel_window_adjust_msg (int, u_int32_t, void *) {}
  void global_request_msg (int, u_int32_t, void *) {}
  void channel_success_msg (int, u_int32_t, void *) {}
  void channel_failure_msg (int, u_int32_t, void *) {}
  void request_success_msg (int, u_int32_t, void *) {}
  void request_failure_msg (int, u_int32_t, void *) {}
};

