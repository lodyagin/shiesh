#include "StdAfx.h"
#include "sshd.h"
#include "ConnectionFactory.h"
#include "SShutdown.h"
#include "CoreConnection.h"
#include "CoreConnectionPars.h"
#include "compat.h"
#include "mac.h"
#include "misc.h"
#include "ssh2.h"
#include "SensitiveData.h"
#include "canohost.h"
#include "dh.h"
#include "packet.h"
#include "md-sha256.h"
#include "match.h"
#include "auth.h"
#include "SessionChannel.h"

#ifdef PACKET_DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

/* Proposals */

# define KEX_DEFAULT_KEX		\
	"diffie-hellman-group14-sha1," \
	"diffie-hellman-group1-sha1"

// TODO
//	"diffie-hellman-group-exchange-sha256," \
//	"diffie-hellman-group-exchange-sha1," \

#define	KEX_DEFAULT_PK_ALG	"ssh-rsa,ssh-dss"

// FIXME change the priority
#define	KEX_DEFAULT_ENCRYPT \
  "aes256-ctr,aes256-cbc," \
  "aes192-ctr,aes192-cbc," \
  "aes128-ctr,aes128-cbc," \
	"3des-cbc,blowfish-cbc,cast128-cbc," \
	"arcfour256,arcfour128,arcfour," \
	"rijndael-cbc@lysator.liu.se" 
#define	KEX_DEFAULT_MAC \
	"hmac-md5,hmac-sha1,umac-64@openssh.com,hmac-ripemd160," \
	"hmac-ripemd160@openssh.com," \
	"hmac-sha1-96,hmac-md5-96"
#define	KEX_DEFAULT_COMP	"none,zlib@openssh.com,zlib"
#define	KEX_DEFAULT_LANG	""


const static char *const def_proposal[PROPOSAL_MAX] = {
	KEX_DEFAULT_KEX,
	KEX_DEFAULT_PK_ALG,
	KEX_DEFAULT_ENCRYPT,
	KEX_DEFAULT_ENCRYPT,
	KEX_DEFAULT_MAC,
	KEX_DEFAULT_MAC,
	KEX_DEFAULT_COMP,
	KEX_DEFAULT_COMP,
	KEX_DEFAULT_LANG,
	KEX_DEFAULT_LANG
};

//namespace coressh {

class KexDispatcher : public Dispatcher
{
public:
  KexDispatcher (CoreConnection* con);
protected:
  // Overrides
  void kexinit_msg (int, u_int32_t, void *);
  void unexpected_msg (int, u_int32_t, void *);
  void service_request_msg (int, u_int32_t, void *) {}
  void userauth_request_msg (int, u_int32_t, void *) {}
  void channel_close_msg (int, u_int32_t, void *) {}
  void channel_data_msg (int, u_int32_t, void *) {}
  void channel_eof_msg (int, u_int32_t, void *) {}
  void channel_extended_data_msg (int, u_int32_t, void *) {}
  void channel_open_msg (int, u_int32_t, void *) {}
  void channel_open_confirmation_msg (int, u_int32_t, void *) {}
  void channel_open_failure_msg (int, u_int32_t, void *) {}
  void channel_request_msg (int, u_int32_t, void *) {}
  void channel_window_adjust_msg (int, u_int32_t, void *) {}
  void global_request_msg (int, u_int32_t, void *) {}
  void channel_success_msg (int, u_int32_t, void *) {}
  void channel_failure_msg (int, u_int32_t, void *) {}
  void request_success_msg (int, u_int32_t, void *) {}
  void request_failure_msg (int, u_int32_t, void *) {}
};

KexDispatcher::KexDispatcher (CoreConnection* con)
: Dispatcher (con)
{
  dispatch[SSH2_MSG_KEXINIT] = 
    &Dispatcher::kexinit_msg;
}

void KexDispatcher::kexinit_msg
  (int type, u_int32_t seq, void *ctxt)
{
	char *ptr;
	u_int i, dlen;
	Kex *kex = (Kex *)ctxt;

	debug("SSH2_MSG_KEXINIT received");
	if (kex == NULL)
		fatal("kex_input_kexinit: no kex, cannot rekey");

	ptr = (char*) connection->packet_get_raw(&dlen);
  coressh::buffer_append(&kex->peer, ptr, dlen);

	/* discard packet */
	for (i = 0; i < KEX_COOKIE_LEN; i++)
		connection->packet_get_char();
	for (i = 0; i < PROPOSAL_MAX; i++)
		xfree(connection->packet_get_string(NULL));
	(void) connection->packet_get_char();
	(void) connection->packet_get_int();
	packet_check_eom (connection);

	connection->kex_kexinit_finish(kex);
}

void KexDispatcher::unexpected_msg 
  (int type, u_int32_t seq, void *)
{
  	error
      ("Hm, kex protocol error: type %d seq %u", 
      type, seq);
}

CoreConnection::CoreConnection 
  (void* repo, 
   RConnectedSocket* cs,
   const std::string& objId,
   SEvent* connectionTerminated
   )
   : RConnection 
      (repo, 
       cs, 
       objId, 
       5,// each connection can produce 
       // no more than 5 subthreads
       // TODO check on all levels (SThread)
       connectionTerminated
       ), 

   aDatafellows (0),
   packets_initialized (false),
   compression_buffer_ready (0),
   packet_compression (0),
   keep_alive_timeouts (0),
   packet_timeout_ms (-1),
   remote_protocol_flags (0),
   compress_init_send_called (0),
   compress_init_recv_called (0),
   inflate_failed (0),
   deflate_failed (0),
   after_authentication (0),
   extra_pad (0),
   session_id2 (0),
   session_id2_len (0),
   authctxt (0),
   none_auth_method_enabled (1),
   password_authentication (1),
   pubkey_authentication (1),
   method_none 
    ("none", 
     &CoreConnection::userauth_none,
     &none_auth_method_enabled),
   method_passwd
    ("password", 
     &CoreConnection::userauth_passwd,
     &password_authentication),
   /*method_pubkey
    ("publickey", 
     &CoreConnection::userauth_pubkey,
     &pubkey_authentication),*/
   authmethods (0),
   kexDispatcher (0),
   authDispatcher (0),
   srvDispatcher (0),
   xxx_kex (0),
   connection_closed (false),
   subprocTerminated (true), // manual reset
                     // (in wait_until_can_do_something)
   poll2_packet_length (0),
   send2_rekeying (0),
   pd_disconnecting (0)
{
  for (int i = 0; i < PROPOSAL_MAX; i++)
    myproposal[i] = def_proposal[i];

  packet_set_connection ();

  authctxt = new Authctxt;
  //FIXME check alloc

  buffer_clear (&loginmsg);
  authctxt->loginmsg = &loginmsg;
  buffer_init (&loginmsg);

  authmethods = new Authmethod* [3];
  // FIXME check allock
  authmethods[0] = &method_none;
  authmethods[1] = &method_passwd;
  authmethods[2] = NULL;

  kexDispatcher = new KexDispatcher (this);
  authDispatcher = new AuthDispatcher (this);
  //FIXME check alloc
}

CoreConnection::~CoreConnection ()
{
  delete authctxt;
  delete [] authmethods;
  delete kexDispatcher;
  delete authDispatcher;
  delete srvDispatcher;
}


void CoreConnection::run ()
{
  try
  {
    // FIXME stop thread processing
    coressh::sshd_exchange_identification 
      (*get_socket (),
       server_version_string,
       client_version_string);

    do_ssh2_kex(); // FIXME stop thread processing
    do_authentication2 (authctxt); //FIXME stop thread proc.
    logit ("User authenticated, start the session");

    server_loop ();

    //FIXME wrong place!
    /*LOG4STRM_INFO
      (Logging::Root (),
       oss_ << "Connection timed out with "
       << socket->get_peer_address().get_ip () << ':'
       << socket->get_peer_address().get_port()
       );*/

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
    // FIXME no channels/subsystem cleanup

    ((Repository<CoreConnection, CoreConnectionPars>*)repository)->delete_object 
      (this, false); // false means not to delete this
    throw;
  }
  // FIXME no channels/subsystem cleanup
  ((Repository<CoreConnection, CoreConnectionPars>*)repository)->delete_object 
    (this, false); // false means not to delete this
}

void CoreConnection::stop ()
{
  ThreadWithSubthreads<Subsystem, SubsystemPars>::
    for_each 
      (std::mem_fun_ref (&Subsystem::terminate));
  // Parent
  ThreadWithSubthreads<Subsystem, SubsystemPars>::
    stop ();
}

void CoreConnection::datafellows (int df)
{
  aDatafellows = df;
}

int CoreConnection::datafellows () const
{
  return aDatafellows;
}

/* packets interface */

int
CoreConnection::packet_read_poll2(u_int32_t *seqnr_p)
{
  u_char macbuf_storage[EVP_MAX_MD_SIZE];
	u_int padlen, need;
	u_char *macbuf, *cp, type;
	u_int maclen, block_size;
	Enc *enc   = NULL;
	Mac *mac   = NULL;
	Comp *comp = NULL;

	if (newkeys[MODE_IN] != NULL) {
		enc  = &newkeys[MODE_IN]->enc;
		mac  = &newkeys[MODE_IN]->mac;
		comp = &newkeys[MODE_IN]->comp;
	}
	maclen = mac && mac->enabled ? mac->mac_len : 0;
	block_size = enc ? enc->block_size : 8;

	if (poll2_packet_length == 0) {
		/*
		 * check if input size is less than the cipher block size,
		 * decrypt first block and extract length of incoming packet
		 */
		if (buffer_len(&input) < block_size)
			return SSH_MSG_NONE;
		buffer_clear(&incoming_packet);
		cp = (u_char*) buffer_append_space(&incoming_packet, block_size);
		cipher_crypt
      (&receive_context, 
       cp, 
       (const u_char*) buffer_ptr(&input),
		   block_size
       );
		cp = (u_char*) buffer_ptr(&incoming_packet);
		poll2_packet_length = get_u32(cp);
		if (poll2_packet_length < 1 + 4 || poll2_packet_length > 256 * 1024) {
#ifdef PACKET_DEBUG
			buffer_dump(&incoming_packet);
#endif
			packet_disconnect("Bad packet length %-10u",
			    poll2_packet_length);
		}
		DBG(debug("input: packet len %u", poll2_packet_length+4));
		buffer_consume(&input, block_size);
	}
	/* we have a partial packet of block_size bytes */
	need = 4 + poll2_packet_length - block_size;
	DBG(debug("partial packet %d, need %d, maclen %d", block_size,
	    need, maclen));
	if (need % block_size != 0) {
		logit("padding error: need %d block %d mod %d",
		    need, block_size, need % block_size);
		packet_disconnect("Bad packet length %-10u", poll2_packet_length);
	}
	/*
	 * check if the entire packet has been received and
	 * decrypt into incoming_packet
	 */
	if (buffer_len(&input) < need + maclen)
		return SSH_MSG_NONE;
#ifdef PACKET_DEBUG
	fprintf(stderr, "read_poll enc/full: ");
	buffer_dump(&input);
#endif
	cp = (u_char*) buffer_append_space(&incoming_packet, need);
	cipher_crypt
    (&receive_context, 
     cp, 
     (const u_char*) buffer_ptr(&input), 
     need
     );
	buffer_consume(&input, need);
	/*
	 * compute MAC over seqnr and packet,
	 * increment sequence number for incoming packet
	 */
	if (mac && mac->enabled) {
		macbuf = mac_compute
      (mac, 
       p_read.seqnr,
		   (u_char*) buffer_ptr(&incoming_packet),
		   buffer_len(&incoming_packet),
       macbuf_storage,
       sizeof (macbuf_storage)
       );
		if (memcmp(macbuf, buffer_ptr(&input), mac->mac_len) != 0)
			packet_disconnect("Corrupted MAC on input.");
		DBG(debug("MAC #%d ok", p_read.seqnr));
		buffer_consume(&input, mac->mac_len);
	}
	if (seqnr_p != NULL)
		*seqnr_p = p_read.seqnr;
	if (++p_read.seqnr == 0)
		logit("incoming seqnr wraps around");
	if (++p_read.packets == 0)
		if (!(datafellows () & SSH_BUG_NOREKEY))
			fatal("XXX too many packets with same key");
	p_read.blocks += (poll2_packet_length + 4) / block_size;
	p_read.bytes += poll2_packet_length + 4;

	/* get padlen */
	cp = (u_char*) buffer_ptr(&incoming_packet);
	padlen = cp[4];
	DBG(debug("input: padlen %d", padlen));
	if (padlen < 4)
		packet_disconnect("Corrupted padlen %d on input.", padlen);

	/* skip packet size + padlen, discard padding */
	buffer_consume(&incoming_packet, 4 + 1);
	buffer_consume_end(&incoming_packet, padlen);

	DBG(debug("input: len before de-compress %d", buffer_len(&incoming_packet)));
	if (comp && comp->enabled) {
		buffer_clear(&compression_buffer);
		buffer_uncompress(&incoming_packet, &compression_buffer);
		buffer_clear(&incoming_packet);
		buffer_append(&incoming_packet, buffer_ptr(&compression_buffer),
		    buffer_len(&compression_buffer));
		DBG(debug("input: len after de-compress %d",
		    buffer_len(&incoming_packet)));
	}
	/*
	 * get packet type, implies consume.
	 * return length of payload (without type field)
	 */
	type = buffer_get_char(&incoming_packet);
	if (type < SSH2_MSG_MIN || type >= SSH2_MSG_LOCAL_MIN)
		packet_disconnect("Invalid ssh2 packet type: %d", type);
	if (type == SSH2_MSG_NEWKEYS)
		set_newkeys(MODE_IN);
	else if (type == SSH2_MSG_USERAUTH_SUCCESS && !server_side)
		packet_enable_delayed_compress();
#ifdef PACKET_DEBUG
	fprintf(stderr, "read/plain[%d]:\r\n", type);
	buffer_dump(&incoming_packet);
#endif
	/* reset for next packet */
	poll2_packet_length = 0;
	return type;
}

/*
 * Sets the descriptors used for communication.  Disables encryption until
 * packet_set_encryption_key is called.
 */
void
CoreConnection::packet_set_connection(/*int fd_in, int fd_out*/)
{
	Cipher *none = cipher_by_name("none");

	if (none == NULL)
		fatal("packet_set_connection: cannot load cipher 'none'");
	//connection_in = fd_in;
	//connection_out = fd_out;
	cipher_init(&send_context, none, (const u_char *)"",
	    0, NULL, 0, CIPHER_ENCRYPT);
	cipher_init(&receive_context, none, (const u_char *)"",
	    0, NULL, 0, CIPHER_DECRYPT);
	newkeys[MODE_IN] = newkeys[MODE_OUT] = NULL;
	if (!packets_initialized) {
		packets_initialized = true;
		buffer_init(&input);
		buffer_init(&output);
		buffer_init(&outgoing_packet);
		buffer_init(&incoming_packet);
		TAILQ_INIT(&outgoing);
		p_send.packets = p_read.packets = 0;
	}
}

void
CoreConnection::packet_set_timeout(int timeout, int count)
{
	if (timeout == 0 || count == 0) {
		packet_timeout_ms = -1;
		return;
	}
	if ((INT_MAX / 1000) / count < timeout)
		packet_timeout_ms = INT_MAX;
	else
		packet_timeout_ms = timeout * count * 1000;
}

/* Returns 1 if remote host is connected via socket, 0 if not. */

int
CoreConnection::packet_connection_is_on_socket(void)
{
  return 1; // always true for CoreSSH
}

/*
 * Exports an IV from the CipherContext required to export the key
 * state back from the unprivileged child to the privileged parent
 * process.
 */

void
CoreConnection::packet_get_keyiv(int mode, u_char *iv, u_int len)
{
	CipherContext *cc;

	if (mode == MODE_OUT)
		cc = &send_context;
	else
		cc = &receive_context;

	cipher_get_keyiv(cc, iv, len);
}

int
CoreConnection::packet_get_keycontext(int mode, u_char *dat)
{
	CipherContext *cc;

	if (mode == MODE_OUT)
		cc = &send_context;
	else
		cc = &receive_context;

	return (cipher_get_keycontext(cc, dat));
}

void
CoreConnection::packet_set_keycontext(int mode, u_char *dat)
{
	CipherContext *cc;

	if (mode == MODE_OUT)
		cc = &send_context;
	else
		cc = &receive_context;

	cipher_set_keycontext(cc, dat);
}

int
CoreConnection::packet_get_keyiv_len(int mode)
{
	CipherContext *cc;

	if (mode == MODE_OUT)
		cc = &send_context;
	else
		cc = &receive_context;

	return (cipher_get_keyiv_len(cc));
}

void
CoreConnection::packet_set_iv(int mode, u_char *dat)
{
	CipherContext *cc;

	if (mode == MODE_OUT)
		cc = &send_context;
	else
		cc = &receive_context;

	cipher_set_keyiv(cc, dat);
}

int
CoreConnection::packet_get_ssh1_cipher(void)
{
	return (cipher_get_number(receive_context.cipher));
}

void
CoreConnection::packet_get_state(int mode, u_int32_t *seqnr, u_int64_t *blocks, u_int32_t *packets,
    u_int64_t *bytes)
{
	struct packet_state *state;

	state = (mode == MODE_IN) ? &p_read : &p_send;
	if (seqnr)
		*seqnr = state->seqnr;
	if (blocks)
		*blocks = state->blocks;
	if (packets)
		*packets = state->packets;
	if (bytes)
		*bytes = state->bytes;
}

void
CoreConnection::packet_set_state(int mode, u_int32_t seqnr, u_int64_t blocks, u_int32_t packets,
    u_int64_t bytes)
{
	struct packet_state *state;

	state = (mode == MODE_IN) ? &p_read : &p_send;
	state->seqnr = seqnr;
	state->blocks = blocks;
	state->packets = packets;
	state->bytes = bytes;
}

/* returns 1 if connection is via ipv4 */

int
CoreConnection::packet_connection_is_ipv4(void)
{
  return 1; // always true for CoreSSH
}

/* Start constructing a packet to send. */
void
CoreConnection::packet_start(u_char type)
{
	u_char buf[9];
	int len;

	DBG(debug("packet_start[%d]", type));
  LOG4STRM_DEBUG
    (Dispatcher::log.GetLogger (),
    oss_ << "<.  " << Dispatcher::msgNames.at (type));
	len = compat20 ? 6 : 9;
	memset(buf, 0, len - 1);
	buf[len - 1] = type;
	buffer_clear(&outgoing_packet);
	buffer_append(&outgoing_packet, buf, len);
}

/* Returns the descriptor used for writing. */

/*int
CoreConnection::packet_get_connection_out(void)
{
	return connection_out;
}*/

/* Returns the remote protocol flags set earlier by the above function. */

u_int
CoreConnection::packet_get_protocol_flags(void)
{
	return remote_protocol_flags;
}

/*
 * Starts packet compression from the next packet on in both directions.
 * Level is compression level 1 (fastest) - 9 (slow, best) as in gzip.
 */

void
CoreConnection::packet_init_compression(void)
{
	if (compression_buffer_ready == 1)
		return;
	compression_buffer_ready = 1;
	buffer_init(&compression_buffer);
}

void
CoreConnection::packet_start_compression(int level)
{
	if (packet_compression && !compat20)
		fatal("Compression already enabled.");
	packet_compression = 1;
	packet_init_compression();
	buffer_compress_init_send(level);
	buffer_compress_init_recv();
}

#if 0
/*
 * Causes any further packets to be encrypted using the given key.  The same
 * key is used for both sending and reception.  However, both directions are
 * encrypted independently of each other.
 */

void
CoreConnection::packet_set_encryption_key(const u_char *key, u_int keylen,
    int number)
{
	Cipher *cipher = cipher_by_number(number);

	if (cipher == NULL)
		fatal("packet_set_encryption_key: unknown cipher number %d", number);
	if (keylen < 20)
		fatal("packet_set_encryption_key: keylen too small: %d", keylen);
	if (keylen > SSH_SESSION_KEY_LENGTH)
		fatal("packet_set_encryption_key: keylen too big: %d", keylen);
	memcpy(ssh1_key, key, keylen);
	ssh1_keylen = keylen;
	cipher_init(&send_context, cipher, key, keylen, NULL, 0, CIPHER_ENCRYPT);
	cipher_init(&receive_context, cipher, key, keylen, NULL, 0, CIPHER_DECRYPT);
}

u_int
CoreConnection::packet_get_encryption_key(u_char *key)
{
	if (key == NULL)
		return (ssh1_keylen);
	memcpy(key, ssh1_key, ssh1_keylen);
	return (ssh1_keylen);
}
#endif

/* Append payload. */
void
CoreConnection::packet_put_char(int value)
{
	char ch = value;

	buffer_append(&outgoing_packet, &ch, 1);
}

void
CoreConnection::packet_put_int(u_int value)
{
	buffer_put_int(&outgoing_packet, value);
}

void
CoreConnection::packet_put_string(const void *buf, u_int len)
{
	buffer_put_string(&outgoing_packet, buf, len);
}

void
CoreConnection::packet_put_cstring(const char *str)
{
	buffer_put_cstring(&outgoing_packet, str);
}

void
CoreConnection::packet_put_raw(const void *buf, u_int len)
{
	buffer_append(&outgoing_packet, buf, len);
}

void
CoreConnection::packet_put_bignum(BIGNUM * value)
{
	buffer_put_bignum(&outgoing_packet, value);
}

void
CoreConnection::packet_put_bignum2(BIGNUM * value)
{
	buffer_put_bignum2(&outgoing_packet, value);
}

void
CoreConnection::set_newkeys(int mode)
{
	Enc *enc;
	Mac *mac;
	Comp *comp;
	CipherContext *cc;
	u_int64_t *max_blocks;
	int crypt_type;

	debug2("set_newkeys: mode %d", mode);

	if (mode == MODE_OUT) {
		cc = &send_context;
		crypt_type = CIPHER_ENCRYPT;
		p_send.packets = 0;
    p_send.blocks = 0;
		max_blocks = &max_blocks_out;
	} else {
		cc = &receive_context;
		crypt_type = CIPHER_DECRYPT;
		p_read.packets = 0;
    p_read.blocks = 0;
		max_blocks = &max_blocks_in;
	}
	if (newkeys[mode] != NULL) {
		debug("set_newkeys: rekeying");
		cipher_cleanup(cc);
		enc  = &newkeys[mode]->enc;
		mac  = &newkeys[mode]->mac;
		comp = &newkeys[mode]->comp;
		mac_clear(mac);
		xfree(enc->name);
		xfree(enc->iv);
		xfree(enc->key);
		xfree(mac->name);
		xfree(mac->key);
		xfree(comp->name);
		xfree(newkeys[mode]);
	}
	newkeys[mode] = kex_get_newkeys(mode);
	if (newkeys[mode] == NULL)
		fatal("newkeys: no keys for mode %d", mode);
	enc  = &newkeys[mode]->enc;
	mac  = &newkeys[mode]->mac;
	comp = &newkeys[mode]->comp;
	if (mac_init(mac) == 0)
		mac->enabled = 1;
	DBG(debug("cipher_init_context: %d", mode));
	cipher_init(cc, enc->cipher, enc->key, enc->key_len,
	    enc->iv, enc->block_size, crypt_type);
	/* Deleting the keys does not gain extra security */
	/* memset(enc->iv,  0, enc->block_size);
	   memset(enc->key, 0, enc->key_len);
	   memset(mac->key, 0, mac->key_len); */
	if ((comp->type == COMP_ZLIB ||
	    (comp->type == COMP_DELAYED && after_authentication)) &&
	    comp->enabled == 0) {
		packet_init_compression();
		if (mode == MODE_OUT)
			buffer_compress_init_send(6);
		else
			buffer_compress_init_recv();
		comp->enabled = 1;
	}
	/*
	 * The 2^(blocksize*2) limit is too expensive for 3DES,
	 * blowfish, etc, so enforce a 1GB limit for small blocksizes.
	 */
	if (enc->block_size >= 16)
		*max_blocks = (u_int64_t)1 << (enc->block_size*2);
	else
		*max_blocks = ((u_int64_t)1 << 30) / enc->block_size;
	if (rekey_limit)
		*max_blocks = MIN(*max_blocks, rekey_limit / enc->block_size);
}

/*
 * Delayed compression for SSH2 is enabled after authentication:
 * This happens on the server side after a SSH2_MSG_USERAUTH_SUCCESS is sent,
 * and on the client side after a SSH2_MSG_USERAUTH_SUCCESS is received.
 */
void
CoreConnection::packet_enable_delayed_compress(void)
{
	Comp *comp = NULL;
	int mode;

	/*
	 * Remember that we are past the authentication step, so rekeying
	 * with COMP_DELAYED will turn on compression immediately.
	 */
	after_authentication = 1;
	for (mode = 0; mode < MODE_MAX; mode++) {
		/* protocol error: USERAUTH_SUCCESS received before NEWKEYS */
		if (newkeys[mode] == NULL)
			continue;
		comp = &newkeys[mode]->comp;
		if (comp && !comp->enabled && comp->type == COMP_DELAYED) {
			packet_init_compression();
			if (mode == MODE_OUT)
				buffer_compress_init_send(6);
			else
				buffer_compress_init_recv();
			comp->enabled = 1;
		}
	}
}

/*
 * Finalize packet in SSH2 format (compress, mac, encrypt, enqueue)
 */
void
CoreConnection::packet_send2_wrapped(void)
{
  u_char macbuf_storage[EVP_MAX_MD_SIZE];
	u_char type, *cp, *macbuf = NULL;
	u_char padlen, pad;
	u_int packet_length = 0;
	u_int i, len;
	u_int32_t rnd = 0;
	Enc *enc   = NULL;
	Mac *mac   = NULL;
	Comp *comp = NULL;
	int block_size;

	if (newkeys[MODE_OUT] != NULL) {
		enc  = &newkeys[MODE_OUT]->enc;
		mac  = &newkeys[MODE_OUT]->mac;
		comp = &newkeys[MODE_OUT]->comp;
	}
	block_size = enc ? enc->block_size : 8;

	cp = (u_char*) buffer_ptr(&outgoing_packet);
	type = cp[5];

#ifdef PACKET_DEBUG
	fprintf(stderr, "plain:     ");
	buffer_dump(&outgoing_packet);
#endif

	if (comp && comp->enabled) {
		len = buffer_len(&outgoing_packet);
		/* skip header, compress only payload */
		buffer_consume(&outgoing_packet, 5);
		buffer_clear(&compression_buffer);
		buffer_compress(&outgoing_packet, &compression_buffer);
		buffer_clear(&outgoing_packet);
		buffer_append(&outgoing_packet, "\0\0\0\0\0", 5);
		buffer_append(&outgoing_packet, buffer_ptr(&compression_buffer),
		    buffer_len(&compression_buffer));
		DBG(debug("compression: raw %d compressed %d", len,
		    buffer_len(&outgoing_packet)));
	}

	/* sizeof (packet_len + pad_len + payload) */
	len = buffer_len(&outgoing_packet);

	/*
	 * calc size of padding, alloc space, get random data,
	 * minimum padding is 4 bytes
	 */
	padlen = block_size - (len % block_size);
	if (padlen < 4)
		padlen += block_size;
	if (extra_pad) {
		/* will wrap if extra_pad+padlen > 255 */
		extra_pad  = roundup(extra_pad, block_size);
		pad = extra_pad - ((len + padlen) % extra_pad);
		debug3("packet_send2: adding %d (len %d padlen %d extra_pad %d)",
		    pad, len, padlen, extra_pad);
		padlen += pad;
		extra_pad = 0;
	}
	cp = (u_char*) buffer_append_space(&outgoing_packet, padlen);
	if (enc && !send_context.plaintext) {
		/* random padding */
		for (i = 0; i < padlen; i++) {
			if (i % 4 == 0)
				rnd = arc4rand.arc4random();
			cp[i] = rnd & 0xff;
			rnd >>= 8;
		}
	} else {
		/* clear padding */
		memset(cp, 0, padlen);
	}
	/* packet_length includes payload, padding and padding length field */
	packet_length = buffer_len(&outgoing_packet) - 4;
	cp = (u_char*) buffer_ptr(&outgoing_packet);
	put_u32(cp, packet_length);
	cp[4] = padlen;
	DBG(debug("send: len %d (includes padlen %d)", packet_length+4, padlen));

	/* compute MAC over seqnr and packet(length fields, payload, padding) */
	if (mac && mac->enabled) {
		macbuf = mac_compute
      (mac, 
       p_send.seqnr,
		   (u_char*) buffer_ptr(&outgoing_packet),
		   buffer_len(&outgoing_packet), 
       macbuf_storage,
       sizeof (macbuf_storage)
       );
		DBG(debug("done calc MAC out #%d", p_send.seqnr));
	}
	/* encrypt packet and append to output buffer. */
	cp = (u_char*) buffer_append_space(&output, buffer_len(&outgoing_packet));
	cipher_crypt(&send_context, cp, (u_char*) buffer_ptr(&outgoing_packet),
	    buffer_len(&outgoing_packet));
	/* append unencrypted MAC */
	if (mac && mac->enabled)
		buffer_append(&output, macbuf, mac->mac_len);
#ifdef PACKET_DEBUG
	fprintf(stderr, "encrypted: ");
	buffer_dump(&output);
#endif
	/* increment sequence number for outgoing packets */
	if (++p_send.seqnr == 0)
		logit("outgoing seqnr wraps around");
	if (++p_send.packets == 0)
		if (!(datafellows () & SSH_BUG_NOREKEY))
			fatal("XXX too many packets with same key");
	p_send.blocks += (packet_length + 4) / block_size;
	p_send.bytes += packet_length + 4;
	buffer_clear(&outgoing_packet);

	if (type == SSH2_MSG_NEWKEYS)
		set_newkeys(MODE_OUT);
	else if (type == SSH2_MSG_USERAUTH_SUCCESS && server_side)
		packet_enable_delayed_compress();
}

void
CoreConnection::packet_send2(void)
{
	struct packet *p;
	u_char type, *cp;

	cp = (u_char*) buffer_ptr(&outgoing_packet);
	type = cp[5];

	/* during rekeying we can only send key exchange messages */
	if (send2_rekeying) {
		if (!((type >= SSH2_MSG_TRANSPORT_MIN) &&
		    (type <= SSH2_MSG_TRANSPORT_MAX))) {
			debug("enqueue packet: %u", type);
			p = (packet*) xmalloc(sizeof(*p));
			p->type = type;
			memcpy(&p->payload, &outgoing_packet, sizeof(Buffer));
			buffer_init(&outgoing_packet);
			TAILQ_INSERT_TAIL(&outgoing, p, next);
			return;
		}
	}

	/* rekeying starts with sending KEXINIT */
	if (type == SSH2_MSG_KEXINIT)
		send2_rekeying = 1;

	packet_send2_wrapped();

	/* after a NEWKEYS message we can send the complete queue */
	if (type == SSH2_MSG_NEWKEYS) {
		send2_rekeying = 0;
		while ((p = TAILQ_FIRST(&outgoing))) {
			type = p->type;
			debug("dequeue packet: %u", type);
			buffer_free(&outgoing_packet);
			memcpy(&outgoing_packet, &p->payload,
			    sizeof(Buffer));
			TAILQ_REMOVE(&outgoing, p, next);
			xfree(p);
			packet_send2_wrapped();
		}
	}
}

void
CoreConnection::packet_send(void)
{
	if (compat20)
		packet_send2();
	else
  {
		fatal ("ssh1 is not supported");
    //packet_send1();
  }
	DBG(debug("packet_send done"));
}

/*
 * Waits until a packet has been received, and returns its type.  Note that
 * no other data is processed until this returns, so this function should not
 * be used during the interactive session.
 */

int
CoreConnection::packet_read(void)
{
	return packet_read_seqnr(NULL);
}

int
CoreConnection::packet_remaining(void)
{
	return buffer_len(&incoming_packet);
}

/*
 * Buffers the given amount of input characters.  This is intended to be used
 * together with packet_read_poll.
 */

/* Returns a character from the packet. */

u_int
CoreConnection::packet_get_char(void)
{
	char ch;

	buffer_get(&incoming_packet, &ch, 1);
	return (u_char) ch;
}

/* Returns an integer from the packet data. */

u_int
CoreConnection::packet_get_int(void)
{
	return buffer_get_int(&incoming_packet);
}

/*
 * Returns an arbitrary precision integer from the packet data.  The integer
 * must have been initialized before this call.
 */

void
CoreConnection::packet_get_bignum(BIGNUM * value)
{
	buffer_get_bignum(&incoming_packet, value);
}

void
CoreConnection::packet_get_bignum2(BIGNUM * value)
{
	buffer_get_bignum2(&incoming_packet, value);
}

void *
CoreConnection::packet_get_raw(u_int *length_ptr)
{
	u_int bytes = buffer_len(&incoming_packet);

	if (length_ptr != NULL)
		*length_ptr = bytes;
	return buffer_ptr(&incoming_packet);
}

/*
 * Returns a string from the packet data.  The string is allocated using
 * xmalloc; it is the responsibility of the calling program to free it when
 * no longer needed.  The length_ptr argument may be NULL, or point to an
 * integer into which the length of the string is stored.
 */

char *
CoreConnection::packet_get_string(u_int *length_ptr)
{
	return (char*) buffer_get_string(&incoming_packet, length_ptr);
}

void *
CoreConnection::packet_get_string_ptr(u_int *length_ptr)
{
	return buffer_get_string_ptr(&incoming_packet, length_ptr);
}


int
CoreConnection::packet_read_seqnr(u_int32_t *seqnr_p)
{
	int type, len, ret, ms_remain;
  fd_set set_var;
  fd_set *setp = &set_var;
	char buf[8192];
	struct timeval timeout, start, *timeoutp = NULL;

	DBG(debug("packet_read()"));

	/* Since we are blocking, ensure that all written packets have been sent. */
	packet_write_wait();

	/* Stay in the loop until we have received a complete packet. */
	for (;;) {
		/* Try to read a packet from the buffer. */
		type = packet_read_poll_seqnr(seqnr_p);
		/* If we got a packet, return it. */
		if (type != SSH_MSG_NONE) {
			return type;
		}
		/*
		 * Otherwise, wait for some data to arrive, add it to the
		 * buffer, and try again.
		 */
  	FD_ZERO (setp);

    FD_SET(socket->get_socket (), setp);

		if (packet_timeout_ms > 0) {
			ms_remain = packet_timeout_ms;
			timeoutp = &timeout;
		}
		/* Wait for some data to arrive. */
		for (;;) {
			if (packet_timeout_ms != -1) {
				ms_to_timeval(&timeout, ms_remain);
        coressh::gettimeofday(&start);
        //TODO check daylight saving change
			}
			if ((ret = select(0 /*ignored*/, setp, NULL,
			    NULL, timeoutp)) >= 0)
				break;
      const int err = ::WSAGetLastError ();
		  if (err != WSAEINTR &&
			    err != WSAEWOULDBLOCK)
				break;
			if (packet_timeout_ms == -1)
				continue;
			ms_subtract_diff(&start, &ms_remain);
			if (ms_remain <= 0) {
				ret = 0;
				break;
			}
		}
		if (ret == 0) {
			logit("Connection to %.200s timed out while "
			    "waiting to read", get_remote_ipaddr());
			cleanup_exit(255);
		}
		/* Read data from the socket. */
    len = ::recv(socket->get_socket (), buf, sizeof(buf), 0);
		if (len == 0) {
			logit("Connection closed by %.200s", get_remote_ipaddr());
			cleanup_exit(255);
		}
		if (len < 0)
    {
      sSocketCheck (false);
    }
		/* Append it to the buffer. */
		packet_process_incoming(buf, len);
	}
	/* NOTREACHED */
}

int
CoreConnection::packet_read_poll_seqnr(u_int32_t *seqnr_p)
{
	u_int reason, seqnr;
	u_char type;
	char *msg;

	for (;;) {
    type = packet_read_poll2(seqnr_p);
		if (type) {
			keep_alive_timeouts = 0;
			DBG(debug("received packet type %d", type));
		}
		switch (type) {
		case SSH2_MSG_IGNORE:
			debug3("Received SSH2_MSG_IGNORE");
			break;
		case SSH2_MSG_DEBUG:
			packet_get_char();
			msg = (char*) packet_get_string(NULL);
			debug("Remote: %.900s", msg);
			xfree(msg);
			msg = (char*) packet_get_string(NULL);
			xfree(msg);
			break;
		case SSH2_MSG_DISCONNECT:
			reason = packet_get_int();
			msg = (char*) packet_get_string(NULL);
			logit("Received disconnect from %s: %u: %.400s",
			    get_remote_ipaddr(), reason, msg);
			xfree(msg);
			cleanup_exit(255);
			break;
		case SSH2_MSG_UNIMPLEMENTED:
			seqnr = packet_get_int();
			debug("Received SSH2_MSG_UNIMPLEMENTED for %u",
			    seqnr);
			break;
		default:
			return type;
		}
	}
}

int
CoreConnection::packet_read_poll(void)
{
	return packet_read_poll_seqnr(NULL);
}


void
CoreConnection::packet_process_incoming(const char *buf, u_int len)
{
	buffer_append(&input, buf, len);
}

/*
 * Logs the error plus constructs and sends a disconnect packet, closes the
 * connection, and exits.  This function never returns. The error message
 * should not contain a newline.  The length of the formatted message must
 * not exceed 1024 bytes.
 */

void
CoreConnection::packet_disconnect(const char *fmt,...)
{
	char buf[1024];
	va_list args;

	if (pd_disconnecting)	/* Guard against recursive invocations. */
		fatal("packet_disconnect called recursively.");
	pd_disconnecting = 1;

	/*
	 * Format the message.  Note that the caller must make sure the
	 * message is of limited size.
	 */
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	/* Display the error locally */
	logit("Disconnecting: %.100s", buf);

	/* Send the disconnect message to the other side, and wait for it to get sent. */
	if (true /*compat20*/) {
		packet_start(SSH2_MSG_DISCONNECT);
		packet_put_int(SSH2_DISCONNECT_PROTOCOL_ERROR);
		packet_put_cstring(buf);
		packet_put_cstring("");
	} /*else {
		packet_start(SSH_MSG_DISCONNECT);
		packet_put_cstring(buf);
	}*/
	packet_send();
	packet_write_wait();

	/* Stop listening for connections. */
	//channel_close_all(); //FIXME

	/* Close the connection. */
	packet_close();
	cleanup_exit(255);
}

void
CoreConnection::packet_close(void)
{
	if (!packets_initialized)
		return;
	packets_initialized = false;
	/*if (connection_in == connection_out) {
		shutdown(connection_out, SHUT_RDWR);
		close(connection_out);
	} else {
		close(connection_in);
		close(connection_out);
	}*/ //FIXME to the socket class destructor
	buffer_free(&input);
	buffer_free(&output);
	buffer_free(&outgoing_packet);
	buffer_free(&incoming_packet);
	if (compression_buffer_ready) {
		buffer_free(&compression_buffer);
		buffer_compress_uninit();
	}
	cipher_cleanup(&send_context);
	cipher_cleanup(&receive_context);
}

#define MAX_PACKETS	(1U<<31)
int
CoreConnection::packet_need_rekeying(void)
{
	if (datafellows () & SSH_BUG_NOREKEY)
		return 0;
	return
	    (p_send.packets > MAX_PACKETS) ||
	    (p_read.packets > MAX_PACKETS) ||
	    (max_blocks_out && (p_send.blocks > max_blocks_out)) ||
	    (max_blocks_in  && (p_read.blocks > max_blocks_in));
}


/* end of packets interface */

#if 0
/* dispatch interface */

void
CoreConnection::dispatch_range(u_int from, u_int to, dispatch_fn fn)
{
	u_int i;

	for (i = from; i <= to; i++) {
		if (i >= DISPATCH_MAX)
			break;
		dispatch[i] = fn;
	}
}
void
CoreConnection::dispatch_set(int type, dispatch_fn fn)
{
	dispatch[type] = fn;
}


/* end of dispatch interface */
#endif

/* copress interface */

/*
 * Initializes compression; level is compression level from 1 to 9
 * (as in gzip).
 */

void
CoreConnection::buffer_compress_init_send(int level)
{
	if (compress_init_send_called == 1)
		deflateEnd(&outgoing_stream);
	compress_init_send_called = 1;
  coressh::debug("Enabling compression at level %d.", level);
	if (level < 1 || level > 9)
		coressh::fatal("Bad compression level %d.", level);
	deflateInit(&outgoing_stream, level);
}
void
CoreConnection::buffer_compress_init_recv(void)
{
	if (compress_init_recv_called == 1)
		inflateEnd(&incoming_stream);
	compress_init_recv_called = 1;
	inflateInit(&incoming_stream);
}

/* Frees any data structures allocated for compression. */

void
CoreConnection::buffer_compress_uninit(void)
{
  coressh::debug("compress outgoing: raw data %llu, compressed %llu, factor %.2f",
	    (unsigned long long)outgoing_stream.total_in,
	    (unsigned long long)outgoing_stream.total_out,
	    outgoing_stream.total_in == 0 ? 0.0 :
	    (double) outgoing_stream.total_out / outgoing_stream.total_in);
	coressh::debug("compress incoming: raw data %llu, compressed %llu, factor %.2f",
	    (unsigned long long)incoming_stream.total_out,
	    (unsigned long long)incoming_stream.total_in,
	    incoming_stream.total_out == 0 ? 0.0 :
	    (double) incoming_stream.total_in / incoming_stream.total_out);
	if (compress_init_recv_called == 1 && inflate_failed == 0)
		inflateEnd(&incoming_stream);
	if (compress_init_send_called == 1 && deflate_failed == 0)
		deflateEnd(&outgoing_stream);
}

/*
 * Compresses the contents of input_buffer into output_buffer.  All packets
 * compressed using this function will form a single compressed data stream;
 * however, data will be flushed at the end of every call so that each
 * output_buffer can be decompressed independently (but in the appropriate
 * order since they together form a single compression stream) by the
 * receiver.  This appends the compressed data to the output buffer.
 */

void
CoreConnection::buffer_compress(Buffer * input_buffer, Buffer * output_buffer)
{
	u_char buf[4096];
	int status;

	/* This case is not handled below. */
	if (buffer_len(input_buffer) == 0)
		return;

	/* Input is the contents of the input buffer. */
	outgoing_stream.next_in = (Bytef *) buffer_ptr(input_buffer);
	outgoing_stream.avail_in = buffer_len(input_buffer);

	/* Loop compressing until deflate() returns with avail_out != 0. */
	do {
		/* Set up fixed-size output buffer. */
		outgoing_stream.next_out = buf;
		outgoing_stream.avail_out = sizeof(buf);

		/* Compress as much data into the buffer as possible. */
		status = deflate(&outgoing_stream, Z_PARTIAL_FLUSH);
		switch (status) {
		case Z_OK:
			/* Append compressed data to output_buffer. */
			buffer_append(output_buffer, buf,
			    sizeof(buf) - outgoing_stream.avail_out);
			break;
		default:
			deflate_failed = 1;
			fatal("buffer_compress: deflate returned %d", status);
			/* NOTREACHED */
		}
	} while (outgoing_stream.avail_out == 0);
}

/*
 * Uncompresses the contents of input_buffer into output_buffer.  All packets
 * uncompressed using this function will form a single compressed data
 * stream; however, data will be flushed at the end of every call so that
 * each output_buffer.  This must be called for the same size units that the
 * buffer_compress was called, and in the same order that buffers compressed
 * with that.  This appends the uncompressed data to the output buffer.
 */

void
CoreConnection::buffer_uncompress(Buffer * input_buffer, Buffer * output_buffer)
{
	u_char buf[4096];
	int status;

	incoming_stream.next_in = (Bytef *) buffer_ptr(input_buffer);
	incoming_stream.avail_in = buffer_len(input_buffer);

	for (;;) {
		/* Set up fixed-size output buffer. */
		incoming_stream.next_out = buf;
		incoming_stream.avail_out = sizeof(buf);

		status = inflate(&incoming_stream, Z_PARTIAL_FLUSH);
		switch (status) {
		case Z_OK:
			buffer_append(output_buffer, buf,
			    sizeof(buf) - incoming_stream.avail_out);
			break;
		case Z_BUF_ERROR:
			/*
			 * Comments in zlib.h say that we should keep calling
			 * inflate() until we get an error.  This appears to
			 * be the error that we get.
			 */
			return;
		default:
			inflate_failed = 1;
			fatal("buffer_uncompress: inflate returned %d", status);
			/* NOTREACHED */
		}
	}
}

/* end of compress interface */

/*
 * SSH2 key exchange: diffie-hellman-group1-sha1
 */
void
CoreConnection::do_ssh2_kex(void)
{
  SensitiveData& sens = SensitiveData::instance ();

	Kex *kex;

	/*if (options.ciphers != NULL) {
		myproposal[PROPOSAL_ENC_ALGS_CTOS] =
		myproposal[PROPOSAL_ENC_ALGS_STOC] = options.ciphers;
	}*/
	myproposal[PROPOSAL_ENC_ALGS_CTOS] =
	    compat_cipher_proposal(myproposal[PROPOSAL_ENC_ALGS_CTOS]);
	myproposal[PROPOSAL_ENC_ALGS_STOC] =
	    compat_cipher_proposal(myproposal[PROPOSAL_ENC_ALGS_STOC]);

	/*if (options.macs != NULL) {
		myproposal[PROPOSAL_MAC_ALGS_CTOS] =
		myproposal[PROPOSAL_MAC_ALGS_STOC] = options.macs;
	}*/
	//if (options.compression == COMP_NONE) {
		myproposal[PROPOSAL_COMP_ALGS_CTOS] =
		myproposal[PROPOSAL_COMP_ALGS_STOC] = "none"; // No compression !
	/*} else if (options.compression == COMP_DELAYED) {
		myproposal[PROPOSAL_COMP_ALGS_CTOS] =
		myproposal[PROPOSAL_COMP_ALGS_STOC] = "none,zlib@openssh.com";
	}*/

	myproposal[PROPOSAL_SERVER_HOST_KEY_ALGS] = sens.list_hostkey_types();

	/* start key exchange */
	kex = kex_setup(myproposal);
  kex->kex[KEX_DH_GRP1_SHA1] = 
    &CoreConnection::kexdh_server;
  kex->kex[KEX_DH_GRP14_SHA1] = 
    &CoreConnection::kexdh_server;
  kex->kex[KEX_DH_GEX_SHA1] = 
    &CoreConnection::kexgex_server;
  kex->kex[KEX_DH_GEX_SHA256] = 
    &CoreConnection::kexgex_server;
	kex->server = 1;
	kex->client_version_string=client_version_string;
	kex->server_version_string=server_version_string;
  kex->load_host_key=
    &SensitiveData::get_hostkey_by_type;
	kex->host_key_index=
    &SensitiveData::get_hostkey_index;

	xxx_kex = kex;

  // Run kex 
	kexDispatcher->dispatch_run
    (Dispatcher::DISPATCH_BLOCK, 
     &kex->done, 
     kex
     );

	session_id2 = kex->session_id;
	session_id2_len = kex->session_id_len;

#ifdef DEBUG_KEXDH
	/* send 1st encrypted/maced/compressed message */
	packet_start(SSH2_MSG_IGNORE);
	packet_put_cstring("markus");
	packet_send();
	packet_write_wait();
#endif
	debug("KEX done");
}

void
CoreConnection::kexdh_server(Kex *kex)
{
  SensitiveData &sens = SensitiveData::instance ();

	BIGNUM *shared_secret = NULL, *dh_client_pub = NULL;
	DH *dh;
	Key *server_host_key;
	u_char *kbuf, *hash, *signature = NULL, *server_host_key_blob = NULL;
	u_int sbloblen, klen, hashlen, slen;
	int kout;

	/* generate server DH public key */
	switch (kex->kex_type) {
	case KEX_DH_GRP1_SHA1:
		dh = dh_new_group1();
		break;
	case KEX_DH_GRP14_SHA1:
		dh = dh_new_group14();
		break;
	default:
		fatal("%s: Unexpected KEX type %d", __FUNCTION__, kex->kex_type);
	}
	dh_gen_key(dh, kex->we_need * 8);

	debug("expecting SSH2_MSG_KEXDH_INIT");
	packet_read_expect(SSH2_MSG_KEXDH_INIT);

	if (kex->load_host_key == NULL)
		fatal("Cannot load hostkey");
  //load_host_key_fn lhk = kex->load_host_key;
	//server_host_key = (sens.*lhk)(kex->hostkey_type);
	server_host_key = (sens.*(kex->load_host_key))
    (kex->hostkey_type);

	if (server_host_key == NULL)
		fatal("Unsupported hostkey type %d", kex->hostkey_type);

	/* key, cert */
	if ((dh_client_pub = BN_new()) == NULL)
		fatal("dh_client_pub == NULL");
	packet_get_bignum2(dh_client_pub);
	packet_check_eom(this);

#ifdef DEBUG_KEXDH
	fprintf(stderr, "dh_client_pub= ");
	BN_print_fp(stderr, dh_client_pub);
	fprintf(stderr, "\n");
	debug("bits %d", BN_num_bits(dh_client_pub));
#endif

#ifdef DEBUG_KEXDH
	DHparams_print_fp(stderr, dh);
	fprintf(stderr, "pub= ");
	BN_print_fp(stderr, dh->pub_key);
	fprintf(stderr, "\n");
#endif
	if (!dh_pub_is_valid(dh, dh_client_pub))
		packet_disconnect("bad client public DH value");

	klen = DH_size(dh);
	kbuf = (u_char*) xmalloc(klen);
	if ((kout = DH_compute_key(kbuf, dh_client_pub, dh)) < 0)
		fatal("DH_compute_key: failed");
#ifdef DEBUG_KEXDH
	dump_digest("shared secret", kbuf, kout);
#endif
	if ((shared_secret = BN_new()) == NULL)
		fatal("kexdh_server: BN_new failed");
	if (BN_bin2bn(kbuf, kout, shared_secret) == NULL)
		fatal("kexdh_server: BN_bin2bn failed");
	memset(kbuf, 0, klen);
	xfree(kbuf);

	key_to_blob(server_host_key, &server_host_key_blob, &sbloblen);

	/* calc H */
	kex_dh_hash(
      kex->client_version_string.c_str (),
	    kex->server_version_string.c_str (),
	    (char*) buffer_ptr(&kex->peer), 
      buffer_len(&kex->peer),
	    (char*) buffer_ptr(&kex->my), buffer_len(&kex->my),
	    server_host_key_blob, sbloblen,
	    dh_client_pub,
	    dh->pub_key,
	    shared_secret,
	    &hash, &hashlen
	);
	BN_clear_free(dh_client_pub);

	/* save session id := H */
	if (kex->session_id == NULL) {
		kex->session_id_len = hashlen;
		kex->session_id = (u_char*) xmalloc(kex->session_id_len);
		memcpy(kex->session_id, hash, kex->session_id_len);
	}

	/* sign H */
	PRIVSEP(key_sign(server_host_key, &signature, &slen, hash, hashlen));

	/* destroy_sensitive_data(); */

	/* send server hostkey, DH pubkey 'f' and singed H */
	packet_start(SSH2_MSG_KEXDH_REPLY);
	packet_put_string(server_host_key_blob, sbloblen);
	packet_put_bignum2(dh->pub_key);	/* f */
	packet_put_string(signature, slen);
	packet_send();

	xfree(signature);
	xfree(server_host_key_blob);
	/* have keys, free DH */
	DH_free(dh);

	kex_derive_keys(kex, hash, hashlen, shared_secret);
	BN_clear_free(shared_secret);
	kex_finish(kex);
}

void
CoreConnection::kexgex_server(Kex *kex)
{
  SensitiveData& sens = SensitiveData::instance ();

	BIGNUM *shared_secret = NULL, *dh_client_pub = NULL;
	Key *server_host_key;
	DH *dh;
	u_char *kbuf, *hash, *signature = NULL, *server_host_key_blob = NULL;
	u_int sbloblen, klen, slen, hashlen;
	int min = -1, max = -1, nbits = -1, type, kout;

	if (kex->load_host_key == NULL)
		fatal("Cannot load hostkey");

	server_host_key = (sens.*(kex->load_host_key))
    (kex->hostkey_type);

	if (server_host_key == NULL)
		fatal("Unsupported hostkey type %d", kex->hostkey_type);

	type = packet_read();
	switch (type) {
	case SSH2_MSG_KEX_DH_GEX_REQUEST:
		debug("SSH2_MSG_KEX_DH_GEX_REQUEST received");
		min = packet_get_int();
		nbits = packet_get_int();
		max = packet_get_int();
		min = MAX(DH_GRP_MIN, min);
		max = MIN(DH_GRP_MAX, max);
		break;
	case SSH2_MSG_KEX_DH_GEX_REQUEST_OLD:
		debug("SSH2_MSG_KEX_DH_GEX_REQUEST_OLD received");
		nbits = packet_get_int();
		min = DH_GRP_MIN;
		max = DH_GRP_MAX;
		/* unused for old GEX */
		break;
	default:
		fatal("protocol error during kex, no DH_GEX_REQUEST: %d", type);
	}
	packet_check_eom(this);

	if (max < min || nbits < min || max < nbits)
		fatal("DH_GEX_REQUEST, bad parameters: %d !< %d !< %d",
		    min, nbits, max);

	/* Contact privileged parent */
	dh = PRIVSEP(choose_dh(min, nbits, max));
	if (dh == NULL)
		packet_disconnect("Protocol error: no matching DH grp found");

	debug("SSH2_MSG_KEX_DH_GEX_GROUP sent");
	packet_start(SSH2_MSG_KEX_DH_GEX_GROUP);
	packet_put_bignum2(dh->p);
	packet_put_bignum2(dh->g);
	packet_send();

	/* flush */
	packet_write_wait();

	/* Compute our exchange value in parallel with the client */
	dh_gen_key(dh, kex->we_need * 8);

	debug("expecting SSH2_MSG_KEX_DH_GEX_INIT");
	packet_read_expect(SSH2_MSG_KEX_DH_GEX_INIT);

	/* key, cert */
	if ((dh_client_pub = BN_new()) == NULL)
		fatal("dh_client_pub == NULL");
	packet_get_bignum2(dh_client_pub);
	packet_check_eom(this);

#ifdef DEBUG_KEXDH
	fprintf(stderr, "dh_client_pub= ");
	BN_print_fp(stderr, dh_client_pub);
	fprintf(stderr, "\n");
	debug("bits %d", BN_num_bits(dh_client_pub));
#endif

#ifdef DEBUG_KEXDH
	DHparams_print_fp(stderr, dh);
	fprintf(stderr, "pub= ");
	BN_print_fp(stderr, dh->pub_key);
	fprintf(stderr, "\n");
#endif
	if (!dh_pub_is_valid(dh, dh_client_pub))
		packet_disconnect("bad client public DH value");

	klen = DH_size(dh);
	kbuf = (u_char*) xmalloc(klen);
	if ((kout = DH_compute_key(kbuf, dh_client_pub, dh)) < 0)
		fatal("DH_compute_key: failed");
#ifdef DEBUG_KEXDH
	dump_digest("shared secret", kbuf, kout);
#endif
	if ((shared_secret = BN_new()) == NULL)
		fatal("kexgex_server: BN_new failed");
	if (BN_bin2bn(kbuf, kout, shared_secret) == NULL)
		fatal("kexgex_server: BN_bin2bn failed");
	memset(kbuf, 0, klen);
	xfree(kbuf);

	key_to_blob(server_host_key, &server_host_key_blob, &sbloblen);

	if (type == SSH2_MSG_KEX_DH_GEX_REQUEST_OLD)
		min = max = -1;

	/* calc H */
	kexgex_hash(
	    kex->evp_md,
	    kex->client_version_string.c_str (),
	    kex->server_version_string.c_str (),
	    (char*) buffer_ptr(&kex->peer), buffer_len(&kex->peer),
	    (char*) buffer_ptr(&kex->my), buffer_len(&kex->my),
	    server_host_key_blob, sbloblen,
	    min, nbits, max,
	    dh->p, dh->g,
	    dh_client_pub,
	    dh->pub_key,
	    shared_secret,
	    &hash, &hashlen
	);
	BN_clear_free(dh_client_pub);

	/* save session id := H */
	if (kex->session_id == NULL) {
		kex->session_id_len = hashlen;
		kex->session_id = (u_char*) xmalloc(kex->session_id_len);
		memcpy(kex->session_id, hash, kex->session_id_len);
	}

	/* sign H */
	PRIVSEP(key_sign(server_host_key, &signature, &slen, hash, hashlen));

	/* destroy_sensitive_data(); */

	/* send server hostkey, DH pubkey 'f' and singed H */
	debug("SSH2_MSG_KEX_DH_GEX_REPLY sent");
	packet_start(SSH2_MSG_KEX_DH_GEX_REPLY);
	packet_put_string(server_host_key_blob, sbloblen);
	packet_put_bignum2(dh->pub_key);	/* f */
	packet_put_string(signature, slen);
	packet_send();

	xfree(signature);
	xfree(server_host_key_blob);
	/* have keys, free DH */
	DH_free(dh);

	kex_derive_keys(kex, hash, hashlen, shared_secret);
	BN_clear_free(shared_secret);

	kex_finish(kex);
}

Newkeys *
CoreConnection::kex_get_newkeys(int mode)
{
	Newkeys *ret;

	ret = current_keys[mode];
	current_keys[mode] = NULL;
	return ret;
}


/* put algorithm proposal into buffer */
void
CoreConnection::kex_prop2buf
  (Buffer *b, const char *proposal[PROPOSAL_MAX])
{
	u_int i;

	buffer_clear(b);
	/*
	 * add a dummy cookie, the cookie will be overwritten by
	 * kex_send_kexinit(), each time a kexinit is set
	 */
	for (i = 0; i < KEX_COOKIE_LEN; i++)
		buffer_put_char(b, 0);
	for (i = 0; i < PROPOSAL_MAX; i++)
		buffer_put_cstring(b, proposal[i]);
	buffer_put_char(b, 0);			/* first_kex_packet_follows */
	buffer_put_int(b, 0);			/* uint32 reserved */
}

/* parse buffer and return algorithm proposal */
char **
CoreConnection::kex_buf2prop(Buffer *raw, int *first_kex_follows)
{
	Buffer b;
	u_int i;
	char **proposal;

	proposal = (char**) xcalloc(PROPOSAL_MAX, sizeof(char *));

	buffer_init(&b);
	buffer_append(&b, buffer_ptr(raw), buffer_len(raw));
	/* skip cookie */
	for (i = 0; i < KEX_COOKIE_LEN; i++)
		buffer_get_char(&b);
	/* extract kex init proposal strings */
	for (i = 0; i < PROPOSAL_MAX; i++) {
		proposal[i] = (char*) buffer_get_string(&b,NULL);
		debug2("kex_parse_kexinit: %s", proposal[i]);
	}
	/* first kex follows / reserved */
	i = buffer_get_char(&b);
	if (first_kex_follows != NULL)
		*first_kex_follows = i;
	debug2("kex_parse_kexinit: first_kex_follows %d ", i);
	i = buffer_get_int(&b);
	debug2("kex_parse_kexinit: reserved %u ", i);
	buffer_free(&b);
	return proposal;
}

void
CoreConnection::kex_prop_free(char **proposal)
{
	u_int i;

	for (i = 0; i < PROPOSAL_MAX; i++)
		xfree(proposal[i]);
	xfree(proposal);
}

Kex *
CoreConnection::kex_setup
  (const char *proposal[PROPOSAL_MAX])
{
	Kex *kex;

	kex = (Kex*) xcalloc(1, sizeof(*kex));
	buffer_init(&kex->peer);
	buffer_init(&kex->my);
	kex_prop2buf(&kex->my, proposal);
	kex->done = 0;

	kex_send_kexinit(kex);					/* we start */
	//kex_reset_dispatch();

	return kex;
}

#define NKEYS	6

void
CoreConnection::kex_derive_keys(Kex *kex, u_char *hash, u_int hashlen, BIGNUM *shared_secret)
{
	u_char *keys[NKEYS];
	u_int i, mode, ctos;

	for (i = 0; i < NKEYS; i++) {
		keys[i] = derive_key(kex, 'A'+i, kex->we_need, hash, hashlen,
		    shared_secret);
	}

	debug2("kex_derive_keys");
	for (mode = 0; mode < MODE_MAX; mode++) {
		current_keys[mode] = kex->newkeys[mode];
		kex->newkeys[mode] = NULL;
		ctos = (!kex->server && mode == MODE_OUT) ||
		    (kex->server && mode == MODE_IN);
		current_keys[mode]->enc.iv  = keys[ctos ? 0 : 1];
		current_keys[mode]->enc.key = keys[ctos ? 2 : 3];
		current_keys[mode]->mac.key = keys[ctos ? 4 : 5];
	}
}

u_char *
CoreConnection::derive_key(Kex *kex, int id, u_int need, u_char *hash, u_int hashlen,
    BIGNUM *shared_secret)
{
	Buffer b;
	EVP_MD_CTX md;
	char c = id;
	u_int have;
	int mdsz;
	u_char *digest;

	if ((mdsz = EVP_MD_size(kex->evp_md)) <= 0)
		fatal("bad kex md size %d", mdsz);
	digest = (u_char*) xmalloc(roundup(need, mdsz));

	buffer_init(&b);
	buffer_put_bignum2(&b, shared_secret);

	/* K1 = HASH(K || H || "A" || session_id) */
	EVP_DigestInit(&md, kex->evp_md);
	if (!(datafellows () & SSH_BUG_DERIVEKEY))
		EVP_DigestUpdate(&md, buffer_ptr(&b), buffer_len(&b));
	EVP_DigestUpdate(&md, hash, hashlen);
	EVP_DigestUpdate(&md, &c, 1);
	EVP_DigestUpdate(&md, kex->session_id, kex->session_id_len);
	EVP_DigestFinal(&md, digest, NULL);

	/*
	 * expand key:
	 * Kn = HASH(K || H || K1 || K2 || ... || Kn-1)
	 * Key = K1 || K2 || ... || Kn
	 */
	for (have = mdsz; need > have; have += mdsz) {
		EVP_DigestInit(&md, kex->evp_md);
		if (!(datafellows () & SSH_BUG_DERIVEKEY))
			EVP_DigestUpdate(&md, buffer_ptr(&b), buffer_len(&b));
		EVP_DigestUpdate(&md, hash, hashlen);
		EVP_DigestUpdate(&md, digest, have);
		EVP_DigestFinal(&md, digest + have, NULL);
	}
	buffer_free(&b);
#ifdef DEBUG_KEX
	fprintf(stderr, "key '%c'== ", c);
	dump_digest("key", digest, need);
#endif
	return digest;
}

void
CoreConnection::kex_finish(Kex *kex)
{
	//kex_reset_dispatch();

	packet_start(SSH2_MSG_NEWKEYS);
	packet_send();
	/* packet_write_wait(); */
	debug("SSH2_MSG_NEWKEYS sent");

	debug("expecting SSH2_MSG_NEWKEYS");
	packet_read_expect(SSH2_MSG_NEWKEYS);
	packet_check_eom(this);
	debug("SSH2_MSG_NEWKEYS received");

	kex->done = 1;
	buffer_clear(&kex->peer);
	/* buffer_clear(&kex->my); */
	kex->flags &= ~KEX_INIT_SENT;
	xfree(kex->name);
	kex->name = NULL;
}

void
CoreConnection::kex_kexinit_finish(Kex *kex)
{
	if (!(kex->flags & KEX_INIT_SENT))
		kex_send_kexinit(kex);

	kex_choose_conf(kex);

	if (kex->kex_type >= 0 && kex->kex_type < KEX_MAX &&
	    kex->kex[kex->kex_type] != NULL) {
		(this->*(kex->kex[kex->kex_type]))(kex);
	} else {
		fatal("Unsupported key exchange %d", kex->kex_type);
	}
}

void
CoreConnection::kex_choose_conf(Kex *kex)
{
	Newkeys *newkeys;
	char **my, **peer;
	char **cprop, **sprop;
	int nenc, nmac, ncomp;
	u_int mode, ctos, need;
	int first_kex_follows, type;

	my   = kex_buf2prop(&kex->my, NULL);
	peer = kex_buf2prop(&kex->peer, &first_kex_follows);

	if (kex->server) {
		cprop=peer;
		sprop=my;
	} else {
		cprop=my;
		sprop=peer;
	}

	/* Algorithm Negotiation */
	for (mode = 0; mode < MODE_MAX; mode++) {
		newkeys = (Newkeys*) xcalloc(1, sizeof(*newkeys));
		kex->newkeys[mode] = newkeys;
		ctos = (!kex->server && mode == MODE_OUT) ||
		    (kex->server && mode == MODE_IN);
		nenc  = ctos ? PROPOSAL_ENC_ALGS_CTOS  : PROPOSAL_ENC_ALGS_STOC;
		nmac  = ctos ? PROPOSAL_MAC_ALGS_CTOS  : PROPOSAL_MAC_ALGS_STOC;
		ncomp = ctos ? PROPOSAL_COMP_ALGS_CTOS : PROPOSAL_COMP_ALGS_STOC;
		choose_enc (&newkeys->enc,  cprop[nenc],  sprop[nenc]);
		choose_mac (&newkeys->mac,  cprop[nmac],  sprop[nmac]);
		choose_comp(&newkeys->comp, cprop[ncomp], sprop[ncomp]);
		logit("kex: %s cpiher=%s mac=%s compression=%s",
		    ctos ? "client->server" : "server->client",
		    newkeys->enc.name,
		    newkeys->mac.name,
		    newkeys->comp.name);
	}
	choose_kex(kex, cprop[PROPOSAL_KEX_ALGS], sprop[PROPOSAL_KEX_ALGS]);
	choose_hostkeyalg(kex, cprop[PROPOSAL_SERVER_HOST_KEY_ALGS],
	    sprop[PROPOSAL_SERVER_HOST_KEY_ALGS]);
	need = 0;
	for (mode = 0; mode < MODE_MAX; mode++) {
		newkeys = kex->newkeys[mode];
		if (need < newkeys->enc.key_len)
			need = newkeys->enc.key_len;
		if (need < newkeys->enc.block_size)
			need = newkeys->enc.block_size;
		if (need < newkeys->mac.key_len)
			need = newkeys->mac.key_len;
	}
	/* XXX need runden? */
	kex->we_need = need;

	/* ignore the next message if the proposals do not match */
	if (first_kex_follows && !proposals_match(my, peer) &&
	    !(datafellows () & SSH_BUG_FIRSTKEX)) {
		type = packet_read();
		debug2("skipping next packet (type %u)", type);
	}

	kex_prop_free(my);
	kex_prop_free(peer);
}

void
CoreConnection::choose_enc(Enc *enc, char *client, char *server)
{
	char *name = match_list(client, server, NULL);
	if (name == NULL)
		fatal("no matching cipher found: client %s server %s",
		    client, server);
	if ((enc->cipher = cipher_by_name(name)) == NULL)
		fatal("matching cipher is not supported: %s", name);
	enc->name = name;
	enc->enabled = 0;
	enc->iv = NULL;
	enc->key = NULL;
	enc->key_len = cipher_keylen(enc->cipher);
	enc->block_size = cipher_blocksize(enc->cipher);
}

void
CoreConnection::choose_mac(Mac *mac, char *client, char *server)
{
	char *name = match_list(client, server, NULL);
	if (name == NULL)
		fatal("no matching mac found: client %s server %s",
		    client, server);
	if (mac_setup(mac, name) < 0)
		fatal("unsupported mac %s", name);
	/* truncate the key */
	if (datafellows () & SSH_BUG_HMAC)
		mac->key_len = 16;
	mac->name = name;
	mac->key = NULL;
	mac->enabled = 0;
}

void
CoreConnection::choose_comp(Comp *comp, char *client, char *server)
{
	char *name = match_list(client, server, NULL);
	if (name == NULL)
		fatal("no matching comp found: client %s server %s", client, server);
	if (strcmp(name, "zlib@openssh.com") == 0) {
		comp->type = COMP_DELAYED;
	} else if (strcmp(name, "zlib") == 0) {
		comp->type = COMP_ZLIB;
	} else if (strcmp(name, "none") == 0) {
		comp->type = COMP_NONE;
	} else {
		fatal("unsupported comp %s", name);
	}
	comp->name = name;
}

void
CoreConnection::choose_kex(Kex *k, char *client, char *server)
{
	k->name = match_list(client, server, NULL);
	if (k->name == NULL)
		fatal("Unable to negotiate a key exchange method");
	if (strcmp(k->name, KEX_DH1) == 0) {
		k->kex_type = KEX_DH_GRP1_SHA1;
		k->evp_md = EVP_sha1();
	} else if (strcmp(k->name, KEX_DH14) == 0) {
		k->kex_type = KEX_DH_GRP14_SHA1;
		k->evp_md = EVP_sha1();
	} else if (strcmp(k->name, KEX_DHGEX_SHA1) == 0) {
		k->kex_type = KEX_DH_GEX_SHA1;
		k->evp_md = EVP_sha1();
#if OPENSSL_VERSION_NUMBER >= 0x00907000L
	} else if (strcmp(k->name, KEX_DHGEX_SHA256) == 0) {
		k->kex_type = KEX_DH_GEX_SHA256;
    k->evp_md = coressh::evp_ssh_sha256();
#endif
#ifdef GSSAPI
	} else if (strncmp(k->name, KEX_GSS_GEX_SHA1_ID,
	    sizeof(KEX_GSS_GEX_SHA1_ID) - 1) == 0) {
		k->kex_type = KEX_GSS_GEX_SHA1;
		k->evp_md = EVP_sha1();
	} else if (strncmp(k->name, KEX_GSS_GRP1_SHA1_ID,
	    sizeof(KEX_GSS_GRP1_SHA1_ID) - 1) == 0) {
		k->kex_type = KEX_GSS_GRP1_SHA1;
		k->evp_md = EVP_sha1();
	} else if (strncmp(k->name, KEX_GSS_GRP14_SHA1_ID,
	    sizeof(KEX_GSS_GRP14_SHA1_ID) - 1) == 0) {
		k->kex_type = KEX_GSS_GRP14_SHA1;
		k->evp_md = EVP_sha1();
#endif
	} else
		fatal("bad kex alg %s", k->name);
}

void
CoreConnection::choose_hostkeyalg(Kex *k, char *client, char *server)
{
	char *hostkeyalg = match_list(client, server, NULL);
	if (hostkeyalg == NULL)
		fatal("no hostkey alg");
	k->hostkey_type = key_type_from_name(hostkeyalg);
	if (k->hostkey_type == KEY_UNSPEC)
		fatal("bad hostkey alg '%s'", hostkeyalg);
	xfree(hostkeyalg);
}

int
CoreConnection::proposals_match(char *my[PROPOSAL_MAX], char *peer[PROPOSAL_MAX])
{
	const static int check[] = {
		PROPOSAL_KEX_ALGS, PROPOSAL_SERVER_HOST_KEY_ALGS, -1
	};
	const int *idx;
	char *p;

	for (idx = &check[0]; *idx != -1; idx++) {
		if ((p = strchr(my[*idx], ',')) != NULL)
			*p = '\0';
		if ((p = strchr(peer[*idx], ',')) != NULL)
			*p = '\0';
		if (strcmp(my[*idx], peer[*idx]) != 0) {
			debug2("proposal mismatch: my %s peer %s",
			    my[*idx], peer[*idx]);
			return (0);
		}
	}
	debug2("proposals match");
	return (1);
}

void
CoreConnection::kex_send_kexinit(Kex *kex)
{
	u_int32_t rnd = 0;
	u_char *cookie;
	u_int i;

	if (kex == NULL) {
		error("kex_send_kexinit: no kex, cannot rekey");
		return;
	}
	if (kex->flags & KEX_INIT_SENT) {
		debug("KEX_INIT_SENT");
		return;
	}
	kex->done = 0;

	/* generate a random cookie */
	if (buffer_len(&kex->my) < KEX_COOKIE_LEN)
		fatal("kex_send_kexinit: kex proposal too short");
	cookie = (u_char*) buffer_ptr(&kex->my);
	for (i = 0; i < KEX_COOKIE_LEN; i++) {
		if (i % 4 == 0)
			rnd = arc4rand.arc4random();
		cookie[i] = rnd;
		rnd >>= 8;
	}
	packet_start(SSH2_MSG_KEXINIT);
	packet_put_raw(buffer_ptr(&kex->my), buffer_len(&kex->my));
	packet_send();
	debug("SSH2_MSG_KEXINIT sent");
	kex->flags |= KEX_INIT_SENT;
}



/*
 * Waits until a packet has been received, verifies that its type matches
 * that given, and gives a fatal error and exits if there is a mismatch.
 */

void
CoreConnection::packet_read_expect(int expected_type)
{
	int type;

	type = packet_read();
	if (type != expected_type)
		packet_disconnect("Protocol error: expected packet type %d, got %d",
		    expected_type, type);
}

/*
 * Calls packet_write_poll repeatedly until all pending output data has been
 * written.
 */

void
CoreConnection::packet_write_wait(void)
{
  fd_set set_var;
	fd_set *setp = &set_var;
	int ret, ms_remain;
	struct timeval start, timeout, *timeoutp = NULL;

	packet_write_poll();
	while (packet_have_data_to_write()) {
    FD_ZERO (setp);
    FD_SET(socket->get_socket (), setp);

		if (packet_timeout_ms > 0) {
			ms_remain = packet_timeout_ms;
			timeoutp = &timeout;
		}
		for (;;) {
			if (packet_timeout_ms != -1) {
				ms_to_timeval(&timeout, ms_remain);
        coressh::gettimeofday(&start);
			}
			if ((ret = select(0 /*ignored*/, NULL, setp,
			    NULL, timeoutp)) >= 0)
				break;
      const int err = ::WSAGetLastError ();
	   	if (err != WSAEINTR &&
		    err != WSAEWOULDBLOCK)
			break;
			if (packet_timeout_ms == -1)
				continue;
			ms_subtract_diff(&start, &ms_remain);
			if (ms_remain <= 0) {
				ret = 0;
				break;
			}
		}
		if (ret == 0) {
			logit("Connection to %.200s timed out while "
			    "waiting to write", get_remote_ipaddr());
			cleanup_exit(255);
		}
		packet_write_poll();
	}
	//xfree(setp);
}

/* Returns true if there is buffered data to write to the connection. */

int
CoreConnection::packet_have_data_to_write(void)
{
	return buffer_len(&output) != 0;
}

/* Checks if there is any buffered output, and tries to write some of the output. */

void
CoreConnection::packet_write_poll(void)
{
	int len = buffer_len(&output);
  int err = 0;

	if (len > 0) {
    len = socket->send(buffer_ptr(&output), len, &err);
    if (len == -1)
    {
      if (err == WSAEINTR || 
			    err == WSAEWOULDBLOCK
          )
          return;
      fatal ("Write failed, errno = %d", err);
    }
    if (len == 0)
			fatal("Write connection closed");
		buffer_consume(&output, len);
	}
}

class SendExitMsg 
  : public std::unary_function<int, void>
{
public:
  SendExitMsg (CoreConnection& _con) : con (_con) {}
  void operator () (int subsystemId)
  {
    Subsystem *s = con.OverSubsystemThread::
      get_object_by_id (subsystemId);
    if (s) 
     s->get_channel()->subproc_terminated_notify();
    // FIXME no channel termination for not SessionChannel-s
  }
protected:
  CoreConnection& con;
};

void CoreConnection::server_loop ()
{
  HANDLE eventArray[WSA_MAXIMUM_WAIT_EVENTS] = {0};
  bool signalled[WSA_MAXIMUM_WAIT_EVENTS] = {0};

  // put channel num in accordance to each channel event
  int chanNums[WSA_MAXIMUM_WAIT_EVENTS] = {0}; 
 
  size_t nChannelEvents;
  DWORD socketEvents = 0;

  // the thread stop event
  eventArray[StopEvt] = SThread::current ()
    .get_stop_event ().evt ();

  // the socket event
  eventArray[SocketEvt] = socket->get_event_object ();

  // the subsystem termination event
  eventArray[SubsystemTermEvt] = subprocTerminated.evt ();

  if (!srvDispatcher)
  {
    srvDispatcher = new ServerMainDispatcher (this);
    //FIXME check alloc
  }

  //long roundNum = 0;
  for (;;)
  {
#if 0
    roundNum++;
    debug
      ("server_loop: round %ld | input = %d ^ output = %d",
      roundNum, 
      (int) buffer_len (&input),
      (int) buffer_len (&output)
      );
#endif

    // [input] -> [descending]

    // process buffered input packets
    // NONBLOCK returns with no action if no packets
    // were collected in wait_until_can_do_something
    srvDispatcher->dispatch_run 
      (Dispatcher::DISPATCH_NONBLOCK, 
       NULL,
       xxx_kex
       );
    
    // rekey request set done = 0
    const bool rekeying = (xxx_kex && !xxx_kex->done);
    // FIXME test rekeying

    // [ascending] -> [output]
    if (!rekeying) // FIXME see OpenSSH packet_not_very_much_data_to_write
        all_channels_output_poll (); 

    // Fill the event array with the all channel data ready events
    if (!rekeying)
      // pre handlers are here
      fill_event_array 
        (eventArray + FirstDescendingEvt, 
         chanNums + FirstDescendingEvt,
         WSA_MAXIMUM_WAIT_EVENTS - FirstDescendingEvt,
         &nChannelEvents
         );
    else
      nChannelEvents = 0;
    //FIXME add event of channel creation to the bunch!

    //debug ("server_loop: start waiting %d events", 
    //       (int) nChannelEvents + 1);
		wait_until_can_do_something
      (eventArray, 
       nChannelEvents + FirstDescendingEvt, 
       0, signalled);

    if (signalled[StopEvt]) 
      // the thread stop requested         
      break; 

		// search terminated subthreads (collect_children)
    if (signalled[SubsystemTermEvt])
    {
      // Get the list of terminated threads (subsystems)
      std::list<int> terminated;
      ThreadWithSubthreads::get_object_ids_by_state
        (std::back_inserter (terminated),
         SThread::terminatedState
         );

      // Send exit message
      std::for_each 
        (terminated.begin (), terminated.end (),
         SendExitMsg (*this));
    }

		if (!rekeying) {

      // [toChannel]   -> [ascending]
      // [fromChannel] <- [descending]

      // posthandlers are here
      all_channel_post (); // FIXME selection

			if (packet_need_rekeying()) {
				debug("need rekeying"); //UT rekeying
				xxx_kex->done = 0;
				kex_send_kexinit(xxx_kex);
			}
		}

    if (signalled[SocketEvt])
      socketEvents = socket->get_events ();
    else
      socketEvents = 0;

    // socket -> buffer "CoreConnection::input"
    process_input(socketEvents);

    if (connection_closed)
			break;

    // buffer "CoreConnection::output" -> socket
		process_output();
  }
	//FIXME collect_children();

	/* free all channels, no more reads and writes */
	//FIXME! channel_free_all();

	/* free remaining sessions, e.g. remove wtmp entries */
	//FIXME! session_destroy_all(NULL);

}

/* -- protocol input */


void
CoreConnection::channel_input_data
  (int type, u_int32_t seq, void *ctxt)
{
	int id;
	char *data;
	u_int data_len;
	Channel *c = 0;

	/* Get the channel number and verify it. */
	id = packet_get_int();
  c = ChannelRepository::get_object_by_id (id);
	if (c == NULL)
		packet_disconnect
      ("Received data for nonexistent channel %d.", id);

	/* Ignore any data for non-open channels (might happen on close) */
	if (!c->channelStateIs ("open"))
		return;

	/* Get the data. */
	data = (char*) packet_get_string_ptr(&data_len);

	/*
	 
	 * The sending side is reducing its window as it sends
	 * data, so we must 'fake' consumption of the data in order to ensure
	 * that window updates are sent back.  Otherwise the connection might
	 * deadlock.
	 */
  if (!c->outputStateIs ("open")) 
  {
			c->local_window -= data_len;
			c->local_consumed += data_len;
      return;
	}

	if (data_len > c->local_maxpacket) {
		logit("channel %d: rcvd big packet %d, maxpack %d",
		    c->self, data_len, c->local_maxpacket);
	}
	if (data_len > c->local_window) {
		logit("channel %d: rcvd too much data %d, win %d",
		    c->self, data_len, c->local_window);
		return;
	}
	c->local_window -= data_len;

  // put raw data into the descending buffer
  c->put_raw_data (data, data_len);
	packet_check_eom(this);
}

void 
CoreConnection::channel_input_eof 
  (int type, u_int32_t seq, void *ctxt)
{
	int id;
	Channel *c = 0;

	/* Get the channel number and verify it. */
	id = packet_get_int();
  packet_check_eom (this);
  c = ChannelRepository::get_object_by_id (id);
	if (c == NULL)
		packet_disconnect
      ("Received ieof for nonexistent channel %d.", id);

  c->rcvd_ieof ();
}

void 
CoreConnection::channel_input_oclose
  (int type, u_int32_t seq, void *ctxt)
{
	const int id = packet_get_int();
	Channel *c = ChannelRepository::get_object_by_id (id);

	packet_check_eom (this);
	if (c == NULL)
		packet_disconnect
      ("Received oclose for nonexistent channel %d.", id);

	debug2("channel %d: rcvd close", c->self);
	
  if (c->closeRcvd)
		error
      ("channel %d: protocol error: close rcvd twice", 
       c->self
       );

	c->closeRcvd = true;
  if (c->channelStateIs("larval")) 
  {
		/* tear down larval channels immediately */
    c->currentOutputState = c->outputClosedState;
    c->currentInputState = c->inputClosedState;
		return;
	}

  // CHANNEL STATES <4>

  if (c->outputStateIs ("open"))
    c->currentOutputState = c->outputWaitDrainState;

	if (c->inputStateIs ("open")) 
  {
    c->currentInputState = c->inputClosedState;
  }
  else if (c->inputStateIs ("waitDrain"))
  {
    c->sendEOF ();
    c->currentInputState = c->inputClosedState;
  }
}

void CoreConnection::all_channels_output_poll ()
{
  ChannelRepository::for_each 
    (std::mem_fun_ref_t<void, Channel> 
      (&Channel::channel_output_poll)
     );
}

void CoreConnection::all_channel_post ()
{
  ChannelRepository::for_each 
    (std::mem_fun_ref_t<void, Channel> 
      (&Channel::channel_post)
     );
}

void CoreConnection::channel_input_window_adjust
  (int type, u_int32_t seq, void *ctxt)
{
	int id;
	u_int adjust;

	/* Get the channel number and verify it. */
	id = packet_get_int();
  Channel* c = ChannelRepository::get_object_by_id (id);

	if (c == NULL) {
		logit("Received window adjust for non-open channel %d.", id);
		return;
	}
	adjust = packet_get_int();
	packet_check_eom(this);
#ifdef SLOW_DEBUG
	debug2("channel %d: rcvd adjust %u", id, adjust);
#endif
	c->remote_window_adjust (adjust);
}

void
CoreConnection::server_input_global_request
  (int type, u_int32_t seq, void *ctxt)
{
	char *rtype;
	int want_reply;
	bool success = false;

	rtype = packet_get_string(NULL);
	want_reply = packet_get_char();
	debug("server_input_global_request: rtype %s want_reply %d", rtype, want_reply);

#if 0
  /* -R style forwarding */
	if (strcmp(rtype, "tcpip-forward") == 0) {
		struct passwd *pw;
		char *listen_address;
		u_short listen_port;

		pw = the_authctxt->pw;
		if (pw == NULL || !the_authctxt->valid)
			fatal("server_input_global_request: no/invalid user");
		listen_address = packet_get_string(NULL);
		listen_port = (u_short)packet_get_int();
		debug("server_input_global_request: tcpip-forward listen %s port %d",
		    listen_address, listen_port);

		/* check permissions */
		if (!options.allow_tcp_forwarding ||
		    no_port_forwarding_flag
#ifndef NO_IPPORT_RESERVED_CONCEPT
		    || (listen_port < IPPORT_RESERVED && pw->pw_uid != 0)
#endif
		    ) {
			success = 0;
			packet_send_debug("Server has disabled port forwarding.");
		} else {
			/* Start listening on the port */
			success = channel_setup_remote_fwd_listener(
			    listen_address, listen_port, options.gateway_ports);
		}
		xfree(listen_address);
	} else if (strcmp(rtype, "cancel-tcpip-forward") == 0) {
		char *cancel_address;
		u_short cancel_port;

		cancel_address = packet_get_string(NULL);
		cancel_port = (u_short)packet_get_int();
		debug("%s: cancel-tcpip-forward addr %s port %d", __func__,
		    cancel_address, cancel_port);

		success = channel_cancel_rport_listener(cancel_address,
		    cancel_port);
		xfree(cancel_address);
	} 
#endif

	if (want_reply) {
		packet_start(success ?
		    SSH2_MSG_REQUEST_SUCCESS : SSH2_MSG_REQUEST_FAILURE);
		packet_send();
		packet_write_wait();
	}
	xfree(rtype);
}



