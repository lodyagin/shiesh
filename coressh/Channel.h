#pragma once

#include "Buffer.h"
#include "StateMap.h"
#include "SNotCopyable.h"
#include "Session.h"
#include <string>

using namespace coressh;

struct SessionChannelPars;

class Channel : public SNotCopyable
{
  friend SessionChannelPars;
  
public:
  // check currentChanState
  bool channelStateIs (const char* stateName);

  // It is a repository id
  const std::string universal_object_id;
  // The same 
  const int self;		

  const std::string ctype;		/* type */

  virtual ~Channel(void);


  // The selectors

  int get_remote_id () const 
  {
    return remote_id;
  }

  u_int get_local_window () const
  {
    return local_window;
  }

  u_int get_local_maxpacket () const
  {
    return local_maxpacket;
  }

protected:

  /* -- States */

  static StateMap* inputStateMap;
  static StateMap* outputStateMap;
  static StateMap* channelStateMap;

  static UniversalState inputOpenState;
  static UniversalState inputWaitDrainState;
  static UniversalState inputClosedState;

  static UniversalState outputOpenState;
  static UniversalState outputWaitDrainState;
  static UniversalState outputClosedState;

  static UniversalState openChanState;
  //static UniversalState closedState;
  static UniversalState larvalChanState;
  //static UniversalState connectingState;
  //static UniversalState zombieState; 
  //FIXME initializing

  static const State2Idx allInputStates[];
  static const StateTransition allInputTrans[];
  static const StateTransition allOutputTrans[];
  static const State2Idx allChanStates[];
  static const StateTransition allChanTrans[];
public:
  static void initializeStates ();
protected:

  /* end States -- */

  Channel
   (const std::string& channelType,
    const std::string& channelId,
    u_int windowSize,
    u_int maxPacketSize,
    const std::string& remoteName
    );

  // the attached session (if any) or NULL
  //Session* session;
  
	Buffer  input;		/* data read from socket, to be sent over
				 * encrypted connection */
	Buffer  output;		/* data received over encrypted connection for
				 * send on socket */
	Buffer  extended;

 	int     remote_id;	/* channel identifier for remote peer */
	//int     flags;		/* close sent/rcvd */
	//int     rfd;		/* read fd */
	//int     wfd;		/* write fd */
	//int     efd;		/* extended fd */
	//int     sock;		/* sock fd */
	//int     ctl_fd;		/* control fd (client sharing) */
	//int     isatty;		/* rfd is a tty */
	//int     wfd_isatty;	/* wfd is a tty */
	//int	client_tty;	/* (client) TTY has been requested */
	int     force_drain;	/* force close on iEOF */
	//int     delayed;		/* fdset hack */

  //	char    path[SSH_CHANNEL_PATH_LEN];
		/* path for unix domain sockets, or host name for forwards */
	//int     listening_port;	/* port being listened for forwards */
	//int     host_port;	/* remote port to connect for forwards */
  std::string remote_name;	/* remote hostname */

	u_int	remote_window;
	u_int	remote_maxpacket;
	u_int	local_window;
	u_int	local_window_max;
	u_int	local_consumed;
	u_int	local_maxpacket;
	//int     extended_usage;
	int	single_connection;

#if 0
	/* callback */
	channel_callback_fn	*open_confirm;
	void			*open_confirm_ctx;
	channel_callback_fn	*detach_user;
	int			detach_close;
	struct channel_confirms	status_confirms;

	/* filter */
	channel_infilter_fn	*input_filter;
	channel_outfilter_fn	*output_filter;
	void			*filter_ctx;
	channel_filter_cleanup_fn *filter_cleanup;
#endif

	/* keep boundaries */
	//int     		datagram;

	/* non-blocking connect */
	//struct channel_connect	connect_ctx;

private:

  UniversalState currentInputState;
  UniversalState currentOutputState;
  UniversalState currentChanState;

};

