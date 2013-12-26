#include "StdAfx.h"
#include "Shell.h"

Shell::Shell
  (const std::string &objectId,
   User *const _pw, 
   BusyThreadWriteBuffer<Buffer>* in,
   BusyThreadReadBuffer<Buffer>* out,
   SEvent* terminatedSignal,
   int _channelId
   )
: Subsystem 
    (objectId, _pw, in, out, 
     terminatedSignal, _channelId
     ),
  procHandle (INVALID_HANDLE_VALUE),
  childInWr (INVALID_HANDLE_VALUE),
  stdoutPipe (0),
  processExits (false)
{
  //buffer_init (&iqueue);
  ZeroMemory
    (&procInfo, sizeof(PROCESS_INFORMATION));
}

Shell::~Shell ()
{
  if (procHandle != INVALID_HANDLE_VALUE)
    ::CloseHandle (procHandle);
  if (childInWr != INVALID_HANDLE_VALUE)
    ::CloseHandle (childInWr);
  delete stdoutPipe;
}

void Shell::run ()
{
  void* inputMsg = 0;
  u_int32_t inputMsgLen;

  HANDLE evts[2] = {
    stdoutPipe->readingIsAvailable.evt (),
    procHandle
  };

  try
  {
	  for (;;) 
    {
      // start read from cmd output
      if (!stdoutPipe->read_started ())
        stdoutPipe->StartRead 
          (&ascendingBuf, sizeof (ascendingBuf));

      // wait until an input message arrived
      inputMsg = fromChannel->get (&inputMsgLen, evts, 2);

      if (inputMsg && inputMsgLen == 0) 
      {
        ssh::xfree (inputMsg); inputMsg = 0;
        assert (fromChannel->n_msgs_in_the_buffer () == 0);

        if (fromChannel->n_msgs_in_the_buffer () != 0)
        {
          LOG4CXX_ERROR 
            (Logging::Root (),
            L"Program error: EOF received but the input buffer"
            L" contains more data to process. The Shell session"
            L" will not be closed (resource leak).");
          continue;
        }

        //FIXME FlushFileBuffers
        // the buffer should be empty
			  return; // exit the thread
		  } 

      if (inputMsg)
      {
        char buf[80];
        int pos = 0;

        for (unsigned i = 0; i < inputMsgLen; i++)
        {
          if (pos >= sizeof (buf) - 1)
          {
            DWORD nBytesWritten = 0;
            sWinCheck 
              (::WriteFile 
                (childInWr,
                 buf,
                 pos,
                 &nBytesWritten,
                 0)
               );
            if (nBytesWritten != pos)
              THROW_EXCEPTION 
                (SException, 
                 L"Not all information is written."
                 );
            pos = 0;
          }

          char c = ((char*) inputMsg) [i];
          if (c != 13)
            buf[pos++] = c;
          else
          {
            buf[pos++] = 13;
            buf[pos++] = 10;
            DWORD nBytesWritten = 0;
            sWinCheck 
              (::WriteFile 
                (childInWr,
                 buf,
                 pos,
                 &nBytesWritten,
                 0)
               );
            if (nBytesWritten != pos)
              THROW_EXCEPTION 
                (SException, 
                 L"Not all information is written."
                 );
            sWinCheck (::FlushFileBuffers (childInWr));
            pos = 0;
          }
          toChannel->put ((char*) inputMsg + i, 1);
        }

        ssh::xfree (inputMsg); inputMsg = 0;

        DWORD nBytesWritten = 0;
        sWinCheck 
          (::WriteFile 
            (childInWr,
             buf,
             pos,
             &nBytesWritten,
             0)
           );
        if (nBytesWritten != pos)
          THROW_EXCEPTION 
            (SException, 
             L"Not all information is written."
             );
      }
      else 
      {
        if (processExits)
          return;

        if (::WaitForSingleObject (evts[1], 0) == WAIT_OBJECT_0)
        { // subprocess exits
          processExits = true; // give read full output
          // UT big output on process exit
        }
        else
        {
          DWORD nBytesRed = 0;
          stdoutPipe->CompleteRead (&nBytesRed);
          if (nBytesRed > 0)
          {
            toChannel->put (ascendingBuf, nBytesRed);
          }
        }
      }
    }
  }
  catch (...)
  {
    if (inputMsg) 
      ssh::xfree (inputMsg);
    throw;
  }
}

void Shell::start ()
{
  HANDLE childInRd; // FIXME close?

  SECURITY_ATTRIBUTES saAttr = {0}; 
  // Set the bInheritHandle flag so 
  // pipe handles are inherited. 
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
  saAttr.bInheritHandle = TRUE; 
  saAttr.lpSecurityDescriptor = NULL; 

  // Create a pipe for the child process's STDIN. 
  sWinCheck 
    (CreatePipe 
      (&childInRd, &childInWr, &saAttr, 0)
     ); 

  // Ensure that write for stdin 
  // is not inherited.
  sWinCheck 
    (SetHandleInformation
      (childInWr, HANDLE_FLAG_INHERIT, 0) 
     );

  std::wostringstream pipeNameStream;
  pipeNameStream << L"ssh_"
    << SThread::id ();
    
  // Create the stdout pipe
  stdoutPipe = new NamedPipe // FIXME check alloc
    (pipeNameStream.str (), NamedPipe::Read);

  // Create the child process. 

  STARTUPINFO siStartInfo;
  BOOL bSuccess = FALSE; 

  ZeroMemory (&siStartInfo, sizeof(STARTUPINFO));

  siStartInfo.cb = sizeof(STARTUPINFO); 
  siStartInfo.hStdError = 
    stdoutPipe->get_client_handle ();
  siStartInfo.hStdOutput = 
    stdoutPipe->get_client_handle ();
  siStartInfo.hStdInput = childInRd;
  siStartInfo.dwFlags = STARTF_USESTDHANDLES;

  // Create the child process. 

  TCHAR szCmdline[]=TEXT("cmd"); 
  sWinCheck
    (CreateProcess
      (NULL, 
       szCmdline,     // command line 
       NULL,          // process security attributes 
       NULL,          // primary thread security attributes 
       TRUE,          // handles are inherited 
       0,             // creation flags 
       NULL,          // the environment 
       fromUTF8 (pw->home_dir ()).c_str (),// the current directory 
       &siStartInfo,  // STARTUPINFO pointer 
       &procInfo)  // receives PROCESS_INFORMATION 
     );
  procHandle = procInfo.hProcess;

  // Close handles to the child process and its primary thread.
  CloseHandle (procInfo.hThread);

  Subsystem::start ();
}

void Shell::stop ()
{
  Subsystem::stop ();
}
