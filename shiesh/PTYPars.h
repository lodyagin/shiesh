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
#include "ChannelRequestPars.h"
#include "PTY.h"
#include "Repository.h"

typedef Repository<PTY, PTYPars> PTYRepository;

class CoreConnection;

class PTYPars : public ChannelRequestPars
{
public:
  u_int col;
  u_int row;
  u_int xpixel;
  u_int ypixel;
  std::string term;

  // Different terminal-based subsystems can exist.
  std::string subsystem_name;

  PTYPars
    (const char* _name/*,
     CoreConnection* con*/);

  virtual PTY* create_derivation 
    (const Repository<PTY, PTYPars>::
     ObjectCreationInfo&
     ) const;

  virtual PTY* transform_object
    (PTY* from) const
  {
    return from; // no transformation
  }

  // Overrides
  void outString (std::ostream& out) const;

protected:
  // Overrides
  void read_from_packet (CoreConnection* con);
};

