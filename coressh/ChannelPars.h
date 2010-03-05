#pragma once

#include <string>
#include "Channel.h"
#include "Repository.h"

struct ChannelPars;

class ChannelRepository 
  : public Repository<Channel, ChannelPars> 
{
public:
  enum {MAX_NUM_OF_CHANNELS = WSA_MAXIMUM_WAIT_EVENTS - 1};
  // in server loop wait: one event for client socket, others
  // are for channels

  ChannelRepository () : Repository (MAX_NUM_OF_CHANNELS) // TODO overflow
  {}

  // Fill the event array by all channel events
  void fill_event_array 
    (HANDLE events[], 
     int chanNums[],
     size_t eventsMaxSize, 
     size_t *eventsSize
     );
};

struct ChannelPars
{
  class UnknownChannelType 
    : public SException
  {
  public:
    UnknownChannelType 
      (const std::string& ctype,
       int _rchan
       )
      : SException 
          (std::string ("Unknown channel type: ") 
           + ctype
           ),
        rchan (_rchan)
    {}

    const int rchan; // remote channel id
  };

  ChannelPars 
    (CoreConnection* con)
    : remote_id (-1), rwindow (0), rmaxpack (0),
      connection (con)
  {
    assert (con);
  }

  virtual ~ChannelPars () {}

  std::string ctype;
  int remote_id;  // peer's id of the channel
  u_int rwindow;  // peer's max window size
  u_int rmaxpack; // peer's max packet size
  CoreConnection* connection;
  //Authctxt* authctxt;

  virtual Channel* create_derivation 
    (const ChannelRepository::ObjectCreationInfo&) const;

  virtual void read_from_packet ();
};
