#pragma once
#include "ThreadWithSubthreads.h"
#include "RConnection.h"
#include "CoreConnection.h"
#include "CoreConnectionPars.h"

// This thread creates CoreConnection
// which is also ThreadWithSubthreads
class MainThread : 
  public ThreadWithSubthreads
    <CoreConnection, CoreConnectionPars> 
{
public:
  static MainThread* create ()
  {
    return new MainThread (1); // max 1 connection
  }

protected:
  MainThread (unsigned n) 
    : ThreadWithSubthreads
      <CoreConnection, CoreConnectionPars>
      (n)
  {}

  // overrides
  void run ();
};
