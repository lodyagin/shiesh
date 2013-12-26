/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Cohors Concurro library.

  This library is free software: you can redistribute
  it and/or modify it under the terms of the GNU Lesser General
  Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License
  for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/
#pragma once
#include "SSingleton.h"
#include "SEvent.h"
#include "Repository.h"
#include "ThreadRepository.h"
#include "RConnectedSocket.h"

/*
All decisions about connection creation
are made by this class.
*/

// ConnectionFactory delegates all calls to
// ThreadRepository (threads)

template<class Connection, class ConnectionPars>
class ConnectionFactory :
  public SSingleton
    <ConnectionFactory<Connection, ConnectionPars>>
{
public:
  ConnectionFactory(ThreadRepository<Connection, ConnectionPars>* _threads)
    : threads (_threads),
      connectionTerminated (false) // automatic reset
  {
    assert (threads);
  }

  Connection* 
    create_new_connection (RConnectedSocket* cs);

  void destroy_terminated_connections ();

  SEvent connectionTerminated;

  typedef ThreadRepository<Connection, ConnectionPars> 
    ConnectionRepository;

protected:
  virtual ConnectionPars*
    create_connection_pars 
      (RConnectedSocket* cs);

  ConnectionRepository* threads;
};

template<class Connection, class ConnectionPars>
Connection* 
ConnectionFactory<Connection, ConnectionPars>::create_new_connection
  (RConnectedSocket* cs)
{
  assert (cs);
  const RSingleprotoSocketAddress& rsa = cs
    -> get_peer_address ();

  LOG4STRM_INFO
    (Logging::Root (),
    oss_ << "New connection from: "
         << rsa.get_ip () << ':'
         << rsa.get_port ()
     );

  Connection* rc = NULL;
  ConnectionPars* cp = create_connection_pars(cs);
  rc = threads->create_object (*cp);
  delete cp; //!
  rc->start ();

  LOG4CXX_DEBUG 
    (Logging::Root (), 
    "New connection is started");
  return rc;
}

template<class Connection, class ConnectionPars>
ConnectionPars* 
ConnectionFactory<Connection, ConnectionPars>::create_connection_pars 
  (RConnectedSocket* cs)
{
  assert (cs);
  ConnectionPars* cp = 
    new ConnectionPars;
  //FIXME check object creation
  cp->socket = cs;
  cp->connectionTerminated = &connectionTerminated;
  return cp;
}

template<class Connection, class ConnectionPars>
void ConnectionFactory<Connection, ConnectionPars>::
  destroy_terminated_connections ()
{
    // Found and destroy the terminated thread
    std::list<int> terminated;
    threads->get_object_ids_by_state
      (std::back_inserter (terminated),
       SThread::terminatedState
       );
    std::for_each 
      (terminated.begin (), terminated.end (),
      ConnectionRepository::Destroy (*threads));
}
