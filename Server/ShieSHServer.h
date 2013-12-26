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
#ifndef __CSERVER_H
#define __CSERVER_H

#include "SNTService.h"
#include "MainThread.h"
#include "Logging.h"
#include "DbAdapter.h"
#include "Options.h"

class CServer : public SNTService
{
   static Logging m_Logging;
   MainThread* main;
public:

   typedef SNTService Parent;

   CServer();
   virtual ~CServer();

   virtual void run();
   virtual void stop();

protected:

   virtual void error( const std::exception & );

   DbAdapter* dbAdapter;
   Options* options;
};

#endif  // __CSERVER_H
