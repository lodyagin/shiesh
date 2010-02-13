#pragma once

#include "RConnection.h"
#include "defines.h"
#include "cipher.h"
#include "buffer.h"
#include "key.h"
#include "kex.h"
#include "queue.h"
#include "Arc4Random.h"
#include "auth.h"
#include "Dispatcher.h"
#include "AuthDispatcher.h"
#include "ServerMainDispatcher.h"
#include "ChannelPars.h"
#include "SessionPars.h"
#include "Subsystem.h"
#include "SubsystemPars.h"
#include "ThreadWithSubthreads.h"
#include "zlib.h"

using namespace coressh;

struct CoreConnectionPars;

class KexDispatcher;

class CoreConnection 
  : public RConnection 
      <ThreadWithSubthreads<Subsystem, SubsystemPars>>, 
      // It means: a CoreConnection thread 
      // produces Session threads

    public ChannelRepository, //TODO (public)
    public SessionRepository
{
  friend CoreConnectionPars;
  friend Dispatcher;
  friend KexDispatcher;
  friend AuthDispatcher;
  friend ServerMainDispatcher;

public:
  ~CoreConnection ();

  static CoreConnection& current ()
  {
    return dynamic_cast <CoreConnection&>
      (SThread::current ());
  }

  void run ();

  // Overrides
  void stop ();

  void datafellows (int df);
  int datafellows () const;

  /* packets interface */

  void     packet_start(u_char);

  void     packet_put_char(int ch);
  void     packet_put_int(u_int value);
  void     packet_put_bignum(BIGNUM * value);
  void     packet_put_bignum2(BIGNUM * value);
  void     packet_put_string(const void *buf, u_int len);
  void     packet_put_cstring(const char *str);
  void     packet_put_raw(const void *buf, u_int len);
  void     packet_send(void);

  u_int	 packet_get_char(void);
  u_int	 packet_get_int(void);
  void     packet_get_bignum(BIGNUM * value);
  void     packet_get_bignum2(BIGNUM * value);
  void	*packet_get_raw(u_int *length_ptr);
  char	*packet_get_string(u_int *length_ptr);
  void	*packet_get_string_ptr(u_int *length_ptr);

  int	   packet_remaining(void);
  void   packet_disconnect(const char *fmt,...);

  /* end of packets interface */

  // TODO move somewhere
  SEvent subsystemTerminated;

protected:
  Arc4Random arc4rand;

  CoreConnection 
    (void* repo, 
     RConnectedSocket* cs,
     const std::string& objId
     );

  void server_loop ();

  /* packets interface */

  void     packet_set_connection();
  void     packet_set_timeout(int, int);
  void     packet_set_nonblocking(void);
  int      packet_get_connection_in(void);
  int      packet_get_connection_out(void);
  void     packet_close(void);
  void	 packet_set_encryption_key(const u_char *, u_int, int);
  u_int	 packet_get_encryption_key(u_char *);
  void     packet_set_protocol_flags(u_int);
  u_int	 packet_get_protocol_flags(void);
  void     packet_start_compression(int);
  void     packet_set_interactive(int);
  int      packet_is_interactive(void);
  void     packet_set_server(void);
  void     packet_set_authenticated(void);

  int      packet_read(void);
  void     packet_read_expect(int type);
  int      packet_read_poll(void);
  void     packet_process_incoming(const char *buf, u_int len);
  int      packet_read_seqnr(u_int32_t *seqnr_p);
  int      packet_read_poll_seqnr(u_int32_t *seqnr_p);

  void     packet_send_debug(const char *fmt,...);

  void	 set_newkeys(int mode);
  int	 packet_get_keyiv_len(int);
  void	 packet_get_keyiv(int, u_char *, u_int);
  int	 packet_get_keycontext(int, u_char *);
  void	 packet_set_keycontext(int, u_char *);
  void	 packet_get_state(int, u_int32_t *, u_int64_t *, u_int32_t *, u_int64_t *);
  void	 packet_set_state(int, u_int32_t, u_int64_t, u_int32_t, u_int64_t);
  int	 packet_get_ssh1_cipher(void);
  void	 packet_set_iv(int, u_char *);

  void     packet_write_poll(void);
  void     packet_write_wait(void);
  int      packet_have_data_to_write(void);
  int      packet_not_very_much_data_to_write(void);

  int	 packet_connection_is_on_socket(void);
  int	 packet_connection_is_ipv4(void);
  void	 packet_send_ignore(int);
  void	 packet_add_padding(u_char);

  /* end of packets interface */

  /* dispatch interface */

  /* Key exch interface */

  Kex	*kex_setup(char *[PROPOSAL_MAX]);
  void	 kex_finish(Kex *);

  void	 kex_send_kexinit(Kex *);
  void	 kex_input_kexinit(int, u_int32_t, void *);
  void	 kex_derive_keys(Kex *, u_char *, u_int, BIGNUM *);

  Newkeys *kex_get_newkeys(int);

  void	 kexdh_client(Kex *);
  void	 kexdh_server(Kex *);
  void	 kexgex_client(Kex *);
  void	 kexgex_server(Kex *);

  void
  derive_ssh1_session_id(BIGNUM *, BIGNUM *, u_int8_t[8], u_int8_t[16]);

  /* end key exch interface */

#if 0
  void	 dispatch_init(dispatch_fn);
  void	 dispatch_set(int, dispatch_fn);
  void	 dispatch_range(u_int, u_int, dispatch_fn);
  void	 dispatch_protocol_error(int, u_int32_t, void *);
  //void	 dispatch_protocol_ignore(int, u_int32_t, void *);

  /* end of dispatch interface */
#endif

  /* compress interface */

  void	 buffer_compress_init_send(int);
  void	 buffer_compress_init_recv(void);
  void   buffer_compress_uninit(void);
  void   buffer_compress(Buffer *, Buffer *);
  void   buffer_uncompress(Buffer *, Buffer *);

  /* end of compress interface */

  /* kex */

  void do_ssh2_kex(void);

  /* end of kex */

  /* authentication */

  // TODO to descendant
  Buffer loginmsg;

  Authctxt* authctxt;

  void	do_authentication2(Authctxt *);

  /* end of authentication */

  /* channel requests */

  void server_input_channel_open
    (int type, u_int32_t seq, void *ctxt);

  void server_input_channel_req
    (int type, u_int32_t seq, void *ctxt);

  void channel_input_window_adjust
    (int type, u_int32_t seq, void *ctxt);

  /* end of channel requests */

  int aDatafellows;

  std::string client_version_string;
  std::string server_version_string;

  // dispatcher for the Kex protocol
  KexDispatcher* kexDispatcher;

  // dispatcher for the Auth protocol
  AuthDispatcher* authDispatcher;

  ServerMainDispatcher* srvDispatcher;

private:

  /* packets */

  /* session identifier (ssh2)*/
  u_char *session_id2;
  u_int session_id2_len;

  // Kex for rekeying
  Kex* xxx_kex;

  /* Set to true if we are the server side. */
  static const int server_side = 0; //FIXME const

    /* Flag indicating whether this module has been initialized. */
  bool packets_initialized;

  /* Encryption context for receiving data.  This is only used for decryption. */
  CipherContext receive_context;

  /* Encryption context for sending data.  This is only used for encryption. */
  CipherContext send_context;

  /* Buffer for raw input data from the socket. */
  Buffer input;

  /* Buffer for raw output data going to the socket. */
  Buffer output;

  /* Buffer for the partial outgoing packet being constructed. */
  Buffer outgoing_packet;

  /* Buffer for the incoming packet currently being processed. */
  Buffer incoming_packet;

  /* Scratch buffer for packet compression/decompression. */
  Buffer compression_buffer;
  int compression_buffer_ready;

  /* Flag indicating whether packet compression/decompression is enabled. */
  int packet_compression;

  /* default maximum packet size */
  static /*!!*/ const u_int max_packet_size = 32768;

  /* Session key information for Encryption and MAC */
  Newkeys *newkeys[MODE_MAX];

  Newkeys *current_keys[MODE_MAX];

  struct packet_state {
    packet_state ()
      : seqnr (0), packets (0),
        blocks (0), bytes (0)
    {}

	  u_int32_t seqnr;
	  u_int32_t packets;
	  u_int64_t blocks;
	  u_int64_t bytes;
  } p_read, p_send;

  u_int64_t max_blocks_in, max_blocks_out;
  u_int32_t rekey_limit;

  /* roundup current message to extra_pad bytes */
  u_char extra_pad;

  struct packet {
	  TAILQ_ENTRY(packet) next;
	  u_char type;
	  Buffer payload;
  };
  TAILQ_HEAD(, packet) outgoing;

  int keep_alive_timeouts;

  /* Set to the maximum time that we will wait to 
     send or receive a packet */
  int packet_timeout_ms;

  /* Protocol flags for the remote side. */
  u_int remote_protocol_flags;

  /* Set to true if we are authenticated. */
  int after_authentication; //TODO to states?

  //SOCKET connection_in;
  //SOCKET connection_out;

  void packet_init_compression(void);
  void packet_enable_delayed_compress(void);
  void packet_send2_wrapped(void);
  void packet_send2(void);
  int packet_read_poll2(u_int32_t *seqnr_p);
  int packet_need_rekeying(void);

  /* end of packets */

  /* compress module data */

  z_stream incoming_stream;
  z_stream outgoing_stream;
  int compress_init_send_called;
  int compress_init_recv_called;
  int inflate_failed;
  int deflate_failed;

  /* end of compress module data */

  /* kex data and private functions */
  char *myproposal[PROPOSAL_MAX];

  char ** kex_buf2prop
    (Buffer *raw, int *first_kex_follows);
  void kex_prop_free(char **proposal);
  void kex_prop2buf(Buffer *b, char *proposal[PROPOSAL_MAX]);
  //void kex_protocol_error(int type, u_int32_t seq, void *ctxt);
  //void kex_reset_dispatch(void);
  u_char *derive_key(Kex *kex, int id, u_int need, u_char *hash, u_int hashlen,
    BIGNUM *shared_secret);
  void kex_kexinit_finish(Kex *kex);
  void kex_choose_conf(Kex *kex);
  void choose_enc(Enc *enc, char *client, char *server);
  void choose_mac(Mac *mac, char *client, char *server);
  void choose_comp(Comp *comp, char *client, char *server);
  void choose_kex(Kex *k, char *client, char *server);
  void choose_hostkeyalg(Kex *k, char *client, char *server);
  int proposals_match(char *my[PROPOSAL_MAX], char *peer[PROPOSAL_MAX]);

  /* authentication */

  Authmethod method_none;
  Authmethod method_passwd;
  //Authmethod method_pubkey;
  Authmethod **authmethods;

  /* "none" is allowed only one time */
  int none_auth_method_enabled; 

  //TODO to options
  int password_authentication;
  int pubkey_authentication;

  // protocol
  void input_service_request(int, u_int32_t, void *);
  void input_userauth_request(int, u_int32_t, void *);

  void userauth_finish(Authctxt *authctxt, int authenticated, char *method);

  // the methods
  int userauth_none(Authctxt *authctxt);
  int userauth_passwd(Authctxt *authctxt);
  //int userauth_pubkey(Authctxt *authctxt);


  // servants
  char * authmethods_get(void);
  Authmethod *authmethod_lookup(const char *name);

  /* end of authentication */

  /* channels & sessions */

  int no_more_sessions ; /* Disallow further sessions. */

  Channel* server_request_session 
    (const ChannelPars& chPars);

  void channel_input_data
    (int type, u_int32_t seq, void *ctxt);

  void channel_input_eof
    (int type, u_int32_t seq, void *ctxt);

 /* end of channels & sessions */

 /* serverloop routines */
  void wait_until_can_do_something
    (HANDLE eventArray[], 
     int nEvents,
     u_int max_time_milliseconds,
     bool signalled[]
     );

  void process_input(long networkEvents);
  void process_output();

  bool connection_closed;

  bool lastSendBlocks;

  void all_channels_output_poll ();
  void all_channel_post_open ();

};
