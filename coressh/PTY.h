#pragma once

#include "CoreConnection.h"

class PTY
{
public:
  PTY (CoreConnection* _con);
  virtual ~PTY(void);
  bool session_pty_req ();
protected:
  void tty_parse_modes(/*int fd,*/ int *n_bytes_ptr);

  CoreConnection* con;
};
