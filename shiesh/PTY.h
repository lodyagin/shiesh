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
#include "Repository.h"
#include "ChannelRequest.h"
#include "ti.h"

class PTY;
class PTYPars;
class CoreConnection;

class PTY : public ChannelRequest
{
  friend PTYPars;
  friend Repository<PTY, PTYPars>;
protected:
  PTY 
    (const std::string &objectId,
     const std::string termName,
     u_int width,
     u_int height
     );
  virtual ~PTY(void);

  static void tty_parse_modes
    (CoreConnection* con, int *n_bytes_ptr);

  utio::CTerminfo termInfo;

  //u_int col;
  //u_int row;
  //u_int xpixel;
  //u_int ypixel;
  //std::string term;
};
