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
#include "stdafx.h"
#include "Options.h"

const std::string Options::version ("1.4d2");
const std::string Options::progName ("ShieSSH");

const std::string 
Options::get_protocol_version_exchange_string () const
{
  return "SSH-2.0-"
    + prog_name_version ()
    + " obihod\r\n";
}

int Options::get_max_login_attempts () const
{
  return 5;
}

const std::wstring
Options::get_terminfo_db_path () const
{
  return L"C:\\ssh\\data\\terminfoDB";
}

const std::wstring
Options::get_config_db_path () const
{
  return L"C:\\ssh\\data\\ssh.dat";
}

