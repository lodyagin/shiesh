#pragma once
#include "Repository.h"
#include "ChannelRequest.h"

class PTY;
class PTYPars;
class CoreConnection;

class PTY : public ChannelRequest
{
  friend PTYPars;
  friend Repository<PTY, PTYPars>;
protected:
  PTY (const std::string &objectId);
  virtual ~PTY(void);

  static void tty_parse_modes
    (CoreConnection* con, int *n_bytes_ptr);

  u_int col;
  u_int row;
  u_int xpixel;
  u_int ypixel;
  std::string term;
};
