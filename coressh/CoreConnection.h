#pragma once

#include "RConnection.h"

struct CoreConnectionPars;

class CoreConnection : public RConnection
{
  friend CoreConnectionPars;

public:
  static CoreConnection& current ()
  {
    return dynamic_cast <CoreConnection&>
      (SThread::current ());
  }

  void run ();

  void datafellows (int df);
  int datafellows () const;
protected:
  CoreConnection 
    (void* repo, RConnectedSocket* cs);

  int aDatafellows;
};
