/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Cohors Concurro library.

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
/*
It is a connection created by the socket.
*/

#pragma once

#include "sthread.h"
#include "RConnectedSocket.h"
#include "Logging.h"

template<class Thread>
class RConnection : public Thread
{ //TODO add states
public:

  const std::string universal_object_id;
  
  RConnectedSocket* get_socket ()
  {
    assert (socket);
    return socket;
  }

  ~RConnection ()
  {
    delete socket;
  }

protected:
  // Usually it is called by ConnectionFactory
  // RConnection takes the socket ownership
  // and will destroy it.
  RConnection 
    (void* repo, 
     RConnectedSocket* cs,
     const std::string& objId,
     const typename Thread::ConstrPar& par,
     SEvent* connectionTerminated
     )
   : Thread (connectionTerminated, par),
     socket (cs), 
     repository (repo), 
     universal_object_id (objId)
  {
    assert (repo);
    assert (socket);
  }

  RConnectedSocket* socket;
  void* repository;
private:
  static Logging log;
};
