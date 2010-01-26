#pragma once

#define DISPATCH_MIN	0
#define DISPATCH_MAX	255

class CoreConnection;

class Dispatcher
{
public:
  enum {
	  DISPATCH_BLOCK,
	  DISPATCH_NONBLOCK
  };

  Dispatcher(CoreConnection* con);
  //virtual ~Dispatcher(void);

  void dispatch_run(int, volatile bool *, void *);

  // functions for messages
  virtual void kexinit_msg (int, u_int32_t, void *) = 0;
  virtual void service_request_msg (int, u_int32_t, void *) = 0;
  virtual void userauth_request_msg (int, u_int32_t, void *) = 0;

  // Called when msg should be ignored
  virtual void ignored_msg (int, u_int32_t, void *);

protected:

  // Called when unecpected msg arrived
  virtual void unexpected_msg 
    (int, u_int32_t, void *);

  typedef void (Dispatcher::*dispatch_fn) 
    (int, u_int32_t, void *);
  
  CoreConnection* connection;

  dispatch_fn dispatch[DISPATCH_MAX];

};


