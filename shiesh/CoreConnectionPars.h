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
#include "SEvent.h"
#include "ThreadWithSubthreads.h"
#include "RConnection.h"
#include "CoreConnection.h"

struct CoreConnectionPars 
{
  RConnectedSocket* socket;
  SEvent* connectionTerminated;

  CoreConnectionPars () 
    : socket (0), connectionTerminated (0)
  {}

  // Overrides
  virtual CoreConnection* 
    create_derivation
      (const Repository<CoreConnection, CoreConnectionPars>::ObjectCreationInfo&) const;

  virtual CoreConnection* transform_object
    (CoreConnection* from) const
  {
    return from; // no transformation
  }

};
