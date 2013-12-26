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
#include "stdafx.h"
#include "SShutdown.h"


// SShutdown  ========================================================

SShutdown::SShutdown() :
  evt(CreateEvent(0, true, false, 0))
{             // SA manual  init name
  sWinCheck(evt != 0, L"creating event");
}

SShutdown::~SShutdown()
{
  CloseHandle(evt);
}

void SShutdown::shutdown()
{
  sWinCheck(SetEvent(evt), L"Setting event");

  for ( size_t i = 0; i < ports.size(); ++i )
    ports[i]->postEmptyEvt();
}

bool SShutdown::isShuttingDown()
{
  return WaitForSingleObject(evt, 0) == WAIT_OBJECT_0;
}

void SShutdown::registerComplPort( SComplPort & port )
{
  ports.push_back(&port);
}

void SShutdown::unregisterComplPort( SComplPort & port )
{
  for ( size_t i = 0; i < ports.size(); ++i )
    if ( ports[i] == &port ) 
    {
      ports.erase(ports.begin() + i);
      return;
    }
  SCHECK(0);  // unregistering non-registered port
}


// XShuttingDown  ====================================================

XShuttingDown::XShuttingDown( const wstring & act ) :
  Parent(sFormat(L"%s action interrupted by shutdown signal", act.c_str())),
  _action(act)
{
}


//====================================================================

void xShuttingDown( const wstring & act )
{
  throw XShuttingDown(act);
}

void sCheckShuttingDown()
{
  if ( SSHUTDOWN.isShuttingDown() ) xShuttingDown();
}
