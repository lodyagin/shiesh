#pragma once
#include "Channel.h"
#include "Subsystem.h"
#include "SubsystemParsFactory.h"
#include "PTYPars.h"

struct SessionChannelPars;

class SessionChannel :
  public Channel
{
  friend ChannelPars;
public:

  enum {PacketDefaultSize = 32 * 1024};
  enum {WindowDefaultSize = 64 * PacketDefaultSize};

  // true if size of ascending + toChannel
  // buffers > 0 or session isn't terminated
  // Overrides
  bool hasAscendingData () const;

  // Overrides
  void subproc_terminated_notify ();

protected:
  SessionChannel
   (const std::string& channelType,
    const std::string& channelId,
    u_int windowSize,
    CoreConnection* connection
    );
  
  ~SessionChannel ();

  HANDLE get_data_ready_event ()
  {
    return toChannel->dataReady.evt ();
  }

  virtual void open ()
  {
    Channel::open (WindowDefaultSize);
  }

  // Overrides
  void input_channel_req 
    (u_int32_t seq, 
     const char* rtype, 
     int reply
     );

  // Overrides
  void garbage_collect ();

  // descending has a complete packet
  // it can be send to "subsystem process"
  // Now it is only used for SFTP because SFTP
  // packets can be larger than SSH packets.
  /*virtual bool is_complete_packet_in_descending ()
  {
    return true;
  }*/

  // Overrides
  void subproc2ascending ();

  // Overrides
  void descending2subproc ();
  
  // Overrides
  void put_eof ()
  {
    fromChannel->put_eof ();
  }

  void session_close_by_channel ();

  Subsystem* subsystem;
  
  SubsystemParsFactory* subsParsFact;

  PTYRepository* ptys;

//public:
  // from channel to subsystem
  BusyThreadWriteBuffer<Buffer>* fromChannel; 

  // from subsystem to channel
  BusyThreadReadBuffer<Buffer>* toChannel;   

  //BusyThreadReadBuffer<Buffer> fromChannelExt;
};
