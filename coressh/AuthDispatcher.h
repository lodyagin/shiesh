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

  void kexinit_msg (int, u_int32_t, void *) {}
};

