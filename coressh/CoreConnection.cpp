#include "StdAfx.h"
#include "sshd.h"
#include "ConnectionFactory.h"
#include "SShutdown.h"
#include "CoreConnection.h"

CoreConnection::CoreConnection 
  (void* repo, 
   RConnectedSocket* cs
   )
   : RConnection (repo, cs), aDatafellows (0)
{
}

void CoreConnection::run ()
{
  try
  {
    coressh::sshd_exchange_identification 
      (get_socket () -> get_socket (),
       get_socket () -> get_socket ());

    for (int i = 0; i < 25; i++) //FIXME to options
    {
      if (SThread::current ().is_stop_requested ())         
      {
        /*::xShuttingDown 
            ("Stop request from the owner thread.");*/

        break; // thread stop is requested
      }

      ::Sleep (1000); 
    }

    //FIXME wrong place!
    LOG4STRM_INFO
      (Logging::Root (),
       oss_ << "Connection timed out with "
       << socket->get_peer_address().get_ip () << ':'
       << socket->get_peer_address().get_port()
       );

    delete socket;
    socket = NULL; //TODO add UT checking working
    // with socket-null objects
  }
  catch (...)
  {
    LOG4STRM_INFO
      (Logging::Root (),
       oss_ << "Terminate connection with "
       << socket->get_peer_address().get_ip () << ':'
       << socket->get_peer_address().get_port()
       );
    ((ConnectionRepository*)repository)->delete_object 
      (this, false); // false means not to delete this
    throw;
  }
  ((ConnectionRepository*)repository)->delete_object 
    (this, false); // false means not to delete this
}

void CoreConnection::datafellows (int df)
{
  aDatafellows = df;
}

int CoreConnection::datafellows () const
{
  return aDatafellows;
}
