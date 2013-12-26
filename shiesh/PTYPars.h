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

