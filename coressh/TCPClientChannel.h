#pragma once
#include "sessionchannel.h"
#include "subsystem.h"
#include "SubsystemPars.h"
#include "RClientSocketAddress.h"
#include "RConnectedSocket.h"
#include "ClientSocketConnector.h"

struct ChannelPars;

class TCPClientChannel : public SessionChannel
{
  friend ChannelPars;
public:
  enum {PacketDefaultSize = 32 * 1024};
  enum {WindowDefaultSize = 64 * PacketDefaultSize};

  ~TCPClientChannel ();

#if 0
  // Overrides
  bool hasAscendingData () const;

  // Overrides
  HANDLE get_data_ready_event ();

  // Overrides
  void garbage_collect ();

  // Overrides
  void subproc_terminated_notify ();

  // Overrides
  void subproc2ascending ();

  // Overrides
  void descending2subproc ();

  // Send EOF to a subprocess (or close a socket)
  // Overrides
  void put_eof ();
#endif
  // Overrides
  void input_channel_req 
    (u_int32_t seq, 
     const char* rtype, 
     int reply
     )
  {
    THROW_EXCEPTION 
      (SException, L"Channel request for TCP channel");
  }

  // Overrides
  void open ()
  {
    isOpened = true;
  }

  // Overrides
  void channel_post ();

  RClientSocketAddress* csa;

protected:
  TCPClientChannel
   (const std::string& channelType,
    const std::string& channelId,
    CoreConnection* connection,
    const std::string& _hostToConnect,
    int _portToConnect
    );

  // true if no responce to MSG_CHANNEL_OPEN
  // was sent yet
  bool isConnecting; 
};

class TCPClientPars;

class TCPClient : public Subsystem
{
  friend TCPClientPars;
protected:
  TCPClient
    (const std::string &objectId,
     User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* in,
     BusyThreadReadBuffer<Buffer>* out,
     SEvent* terminatedSignal,
     int _channelId,
     const RClientSocketAddress& _csa,
     TCPClientChannel* _channel
     );

  ~TCPClient () {} // FIXME

  // Overrides
  void run (); //TODO tcp client isn't thread

  // Overrides
  void start ();

  ClientSocketConnector connector;

  RConnectedSocket* socket;
  const RClientSocketAddress* csa; // FIXME
  TCPClientChannel* channel; // UGLY
private:
  static Logging log;
};

class TCPClientPars : public SubsystemPars
{
public:
  std::string hostToConnect;
  int portToConnect;
  TCPClientChannel* channel;

  // Overrides
  Subsystem* create_derivation 
    (const Repository<Subsystem, SubsystemPars>::
     ObjectCreationInfo&
     ) const;
};
