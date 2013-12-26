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
#include "smutex.h"
#include "snotcopyable.h"
#include "sexception.h"
#include "SShutdown.h"
#include "SEvent.h"
#include "SThread.h"
#include "StateMap.h"
#include <vector>
#include <algorithm>

class InvalidObjectParameters : public SException
{
public:
  InvalidObjectParameters ()
    : SException 
      ("Invalid parameters for repository object creation")
  {}
};

// TODO separate read and write lock

template<class Object, class Parameter>
class Repository : public SNotCopyable
{
protected:
  typedef std::vector<Object*> ObjectMap;

public:
  typedef typename ObjectMap::size_type ObjectId;

  Repository (int maxNumberOfObjects);
  virtual ~Repository ();

  virtual Object* create_object (const Parameter& param);

  virtual void delete_object 
    (Object* obj, bool freeMemory);

  virtual void delete_object_by_id 
    (ObjectId id, bool freeMemory);

  virtual Object* get_object_by_id (ObjectId id) const;

  // Replace the old object by new one
  // The old object is deleted
  virtual Object* replace_object 
    (ObjectId id, 
     const Parameter& param,
     bool freeMemory
     );

  // return ids of objects selected by a predicate
  template<class Out, class Pred>
  Out get_object_ids_by_pred (Out res, Pred p);

  // return ids of objects selected by 
  // an UniversalState
  template<class Out, class State>
  Out get_object_ids_by_state
    (Out res, const State& state);

  template<class Op>
  void for_each (Op& f);

  template<class Op>
  void for_each (Op& f) const;

  // It is used for object creation
  struct ObjectCreationInfo
  {
    //const Parameter* info;
    Repository* repository;
    std::string objectId;
  };

  class Destroy 
    : public std::unary_function<int, void>
  {
  public:
    Destroy (Repository& _repo) : repo (_repo) {}
    void operator () (int objId)
    {
      repo.delete_object_by_id (objId, true);
    }
  protected:
    Repository& repo;
  };

protected:
  ObjectId get_first_unused_object_id (ObjectMap&);

  enum {startMapSize = 10, mapSizeStep = 10};

  ObjectMap* objects;
  SMutex objectsM;
  //SSemaphore semaphore;
};

template<class Object, class Parameter>
Repository<Object, Parameter>::Repository 
  (int maxNumberOfObjects)
: objects (NULL)/*, 
  semaphore (maxNumberOfObjects, maxNumberOfObjects)*/
{
  objects = new ObjectMap ();
  objects->reserve (startMapSize);
  //FIXME check obj creation
  objects->push_back (0); // obj id 0 is not used
}

template<class Object, class Parameter>
class Destructor : 
  public std::unary_function<Object*, void>
{
public:
  Destructor (Repository<Object, Parameter>* _repo)
    : repo (_repo)
  {}

  void operator () (Object* obj)
  { 
    if (obj) 
      repo->delete_object (obj, true); 
  }

protected:
  Repository<Object, Parameter>* repo;
};

template<class Object, class Parameter>
Repository<Object, Parameter>::~Repository ()
{
  std::for_each
    (objects->begin (), 
     objects->end (),
     Destructor<Object, Parameter> (this)
     );
  delete objects;
}

template<class Object, class Parameter>
Object* Repository<Object, Parameter>::create_object 
  (const Parameter& param)
{
  //semaphore.wait ();

  /*if (SThread::current ().is_stop_requested ())
       ::xShuttingDown 
        (L"Stop request from the owner thread.");*/

  { 
    SMutex::Lock lock (objectsM);

    const ObjectId objId = get_first_unused_object_id 
      (*objects);

    std::string uniId;
    toString (objId, uniId);
    const ObjectCreationInfo cinfo =
      { this, uniId };

    Object* obj = param.create_derivation (cinfo);
    //FIXME check creation

    assert (obj);
    objects->at (objId) = obj;
    return obj;
  }
}

template<class Object, class Parameter>
Object* Repository<Object, Parameter>::replace_object 
  (ObjectId id, 
   const Parameter& param,
   bool freeMemory
   )
{
  SMutex::Lock lock (objectsM);

  Object* obj = objects->at (id);
  if (!obj)
    THROW_EXCEPTION
      (SException, oss_ << L"Program error");

  (*objects)[id] = param.transform_object (obj);
  assert ((*objects)[id]);

  if ((*objects)[id] == obj)
    return obj; // no transformation

  if (freeMemory)
    delete obj;

  return (*objects)[id];
}

template<class Object, class Parameter>
void Repository<Object, Parameter>::delete_object 
  (Object* obj, 
   bool freeMemory
   )
{
  assert (obj);
  const ObjectId objId = fromString<ObjectId> (obj->universal_object_id);

  delete_object_by_id (objId, freeMemory);
}

template<class Object, class Parameter>
void Repository<Object, Parameter>::delete_object_by_id 
    (ObjectId id, bool freeMemory)
{
  Object* ptr = 0;
  {
    SMutex::Lock lock (objectsM);

    ObjectMap::reference r = objects->at (id);
    ptr = r;
    if (r == 0)
    {
      THROW_EXCEPTION
      (SException, oss_ << L"Program error");
    }

    r = 0;
  }

  //semaphore.release ();

  if (freeMemory)
    delete ptr;
}

template<class Object, class Parameter>
Object* Repository<Object, Parameter>::get_object_by_id 
  (typename Repository<Object, Parameter>::ObjectId id) const
{
  { 
    SMutex::Lock lock (objectsM);

    if (id < 1 || id >= objects->size ())
      return 0;
    else
      return objects->at (id);
  }
}

template<class Object, class Parameter>
typename Repository<Object, Parameter>::ObjectId 
Repository<Object, Parameter>::get_first_unused_object_id
  (ObjectMap& m)
{ //TODO UT
  SMutex::Lock lock (objectsM);

  // TODO change to stack
  for (ObjectId id = 1; id < m.size (); id++)
  {
    if (!m[id]) return id;
  }

  if (m.size () == m.capacity ())
    m.reserve (m.size () + mapSizeStep);
  // TODO check the stepping

  m.push_back (0);
  return m.size () - 1;
}

template<class Object, class Parameter> 
  template<class Out, class Pred>
Out Repository<Object, Parameter>::
  get_object_ids_by_pred (Out res, Pred p)
{
  SMutex::Lock lock (objectsM);

  for (ObjectId i = 0; i < objects->size (); i++)
    if ((*objects)[i] && p (*(*objects)[i]))
      *res++ = i;
  return res;
}

template<class Object, class State>
class StateMatch 
  : public std::unary_function<const Object&, bool>
{
public:
  StateMatch (const State& _state) : state (_state) {}
  bool operator () (const Object& obj) const
  { 
    return State::state_is (obj, state);
  }
protected:
  State state;
};

template<class Object, class Parameter>
  template<class Out, class State>
Out Repository<Object, Parameter>::
  get_object_ids_by_state (Out res, const State& state)
{
  return get_object_ids_by_pred<Out, StateMatch<Object, State>> (res, StateMatch<Object, State> (state));
}

template<class Object, class Parameter>
  template<class Op>
void Repository<Object, Parameter>::for_each (Op& f)
{
  SMutex::Lock lock (objectsM);

  for (ObjectId i = 0; i < objects->size (); i++)
    if ((*objects)[i])
      f (*(*objects)[i]);
}

template<class Object, class Parameter>
  template<class Op>
void Repository<Object, Parameter>::for_each (Op& f) const
{
  SMutex::Lock lock (objectsM);

  for (ObjectId i = 0; i < objects->size (); i++)
    if ((*objects)[i])
      f (*(*objects)[i]);
}




