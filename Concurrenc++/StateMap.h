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
#include "HasStringView.h"
#include "Logging.h"
#include "SException.h"
#include <map>
#include <vector>

typedef unsigned int StateIdx;

struct State2Idx
{
  StateIdx idx;
  const char* state;
};

struct StateTransition
{
  const char* from;
  const char* to;
};

class StateMap;

class UniversalState
{
  friend StateMap;

public:
  UniversalState () 
    : state_map (0), state_idx (0) 
  {}

  bool operator==
    (const UniversalState& state2) const
  {
    return state_map == state2.state_map
      && state_idx == state2.state_idx;
  }

protected:
  UniversalState (const StateMap* map, StateIdx idx)
    : state_map (map), state_idx (idx)
  {}

  const StateMap* state_map;
  StateIdx        state_idx;
};

/* Exceptions */

class InvalidStateTransition 
  : public SException
{
public:
  InvalidStateTransition 
    (const std::string& from, 
     const std::string& to)
    : SException 
    (std::string ("Invalid state transition from [")
    + from
    + "] to ["
    + to
    + "]."
    )
  {}
};

class NoStateWithTheName : public SException
{
public:
  NoStateWithTheName ()
    : SException ("No state with the name")
  {}
};

class IncompatibleMap : public SException
{
public:
  IncompatibleMap ()
    : SException ("Incompatible map")
  {}
};

/* StateMap class */

class StateMap : 
   public HasStringView 
   //FIXME pointer comparison
{
public:
  class BadParameters : public SException
  {
  public:
    BadParameters ()
      : SException ("Bad initialization parameters")
    {}

    BadParameters (const std::string& str)
      : SException 
      (std::string ("Bad initialization parameters: ")
       + str)
    {}

  };

  StateMap 
    (const State2Idx new_states[], 
     const StateTransition transitions[]);

  // Return the number of states in the map.
  StateIdx size () const;

  UniversalState create_state (const char* name) const;

  bool there_is_transition 
    (const UniversalState& from,
     const UniversalState& to) const;

  void check_transition
    (const UniversalState& from,
     const UniversalState& to) const;

  bool is_equal
    (const UniversalState& a,
     const UniversalState& b) const;
  
  bool is_compatible 
    (const UniversalState& state) const;

  std::string get_state_name
    (const UniversalState& state) const;

  // overrides
  void outString (std::ostream& out) const;

protected:

  struct IdxTransRec
  {
    IdxTransRec () : from (0), to (0) {}
    StateIdx from;
    StateIdx to;
  };

  typedef std::map<std::string, StateIdx> 
    Name2Idx;
  typedef std::map<StateIdx, std::string> Idx2Name;

  typedef std::vector<std::vector <int>> Trans2Number;
  typedef std::vector<IdxTransRec> Number2Trans;

  Name2Idx     name2idx;
  Idx2Name     idx2name;  
  Trans2Number trans2number;
  Number2Trans number2trans;

  int get_transition_id 
    (const UniversalState& from,
     const UniversalState& to) const;

private:
  static Logging log;

  // Is used from constructor
  // Fills trans2number and number2trans
  void add_transitions 
    (const StateTransition transitions[], 
     int nTransitions);
};
