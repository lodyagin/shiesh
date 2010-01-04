#include "StdAfx.h"
#include "RConnection.h"
#include "..\Server\Options.h" //FIXME
#include "ConnectionFactory.h"

Logging RConnection::log ("RConnection");

RConnection* RConnection::create
    (void* repo, RConnectedSocket* cs)
{
  return new RConnection (repo, cs);
}

RConnection::RConnection 
  (void* repo, 
   RConnectedSocket* cs
   ) : socket (cs), repository (repo)
{
  assert (repo);
  assert (socket);
}

RConnection::~RConnection ()
{
  delete socket;
}

void RConnection::run ()
{
  try
  {
 	  /* Send our protocol version identification. */
    socket->send 
      (Options::instance ()
        .get_protocol_version_exchange_string ()
        );
#if 0
    /* Read other sides version identification. */
    std::string clientVersionId;
    socket->receive (clientVersionId);
    LOG4STRM_DEBUG 
      (log.GetLogger (),
      oss_ << "Client id is received: "
           << clientVersionId
       );
    
    // parse the identification
	  int remote_major, remote_minor;
    std::string remoteVersion;
    char* remote_version = 0;
    try
    {
      remote_version = new char
        [clientVersionId.size () + 5];

	    /*
	     * Check that the versions match. */
      if (sscanf_s(clientVersionId.c_str (), 
          "SSH-%d.%d-%[^\n]\n",
	        &remote_major, 
          &remote_minor, 
          remote_version, clientVersionId.size ()
          ) != 3
          ) 
          THROW_EXCEPTION
            (SException,
            oss_ << "Bad client id: ["
                 << clientVersionId << ']'
             );

      remoteVersion.assign 
        (remote_version, clientVersionId.size ());
    }
    catch (...) {
      delete [] remote_version;
      throw;
    }
    delete [] remote_version;

	  /* Check that the versions match. */
    if (remote_major != 2)
      THROW_EXCEPTION 
        (SException,
         oss_ << "Unsupported protocol version");
#endif
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

    delete socket;
    socket = NULL; //TODO add UT checking working
    // with socket-null objects
  }
  catch (...)
  {
    ((ConnectionRepository*)repository)->delete_object 
      (this, false); // false means not to delete this
    throw;
  }
  ((ConnectionRepository*)repository)->delete_object 
    (this, false); // false means not to delete this
}