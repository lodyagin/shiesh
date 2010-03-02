#pragma once
#include "subsystem.h"
#include "NamedPipe.h"

class ShellPars;

class Shell : public Subsystem
{
  friend ShellPars;
protected:
  Shell
    (const std::string &objectId,
     User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* in,
     BusyThreadReadBuffer<Buffer>* out,
     SEvent* terminatedSignal,
     Session* _session
     );

  ~Shell ();

  // Overrides
  void run ();

  // Overrides
  // disable thread stopping, 
  // should flush all buffers.
  // Use BusyThreadWriteBuffer::put_eof ()
  // for flushing buffers and terminating SFTP thread.
  void stop ();

  void start ();

  //Buffer iqueue;

  HANDLE childInWr;

  PROCESS_INFORMATION procInfo; 

  NamedPipe* stdoutPipe;

  char ascendingBuf[80];
};