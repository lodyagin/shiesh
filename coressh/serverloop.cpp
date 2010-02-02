/* $OpenBSD: serverloop.c,v 1.153 2008/06/30 12:15:39 djm Exp $ */
/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Server main loop for handling the interactive session.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 * SSH2 support by Markus Friedl.
 * Copyright (c) 2000, 2001 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "StdAfx.h"
#include "CoreConnection.h"
#include "Channel.h"
#include "ChannelPars.h"
#include "Session.h"
#include "SessionPars.h"
#include "packet.h"
#include "ssh2.h"

#if 0
extern Authctxt *the_authctxt;
extern int use_privsep;

static Buffer stdin_buffer;	/* Buffer for stdin data. */
static Buffer stdout_buffer;	/* Buffer for stdout data. */
static Buffer stderr_buffer;	/* Buffer for stderr data. */
static int fdin;		/* Descriptor for stdin (for writing) */
static int fdout;		/* Descriptor for stdout (for reading);
				   May be same number as fdin. */
static int fderr;		/* Descriptor for stderr.  May be -1. */
static long stdin_bytes = 0;	/* Number of bytes written to stdin. */
static long stdout_bytes = 0;	/* Number of stdout bytes sent to client. */
static long stderr_bytes = 0;	/* Number of stderr bytes sent to client. */
static long fdout_bytes = 0;	/* Number of stdout bytes read from program. */
static int stdin_eof = 0;	/* EOF message received from client. */
static int fdout_eof = 0;	/* EOF encountered reading from fdout. */
static int fderr_eof = 0;	/* EOF encountered readung from fderr. */
static int fdin_is_tty = 0;	/* fdin points to a tty. */
static int connection_in;	/* Connection to client (input). */
static int connection_out;	/* Connection to client (output). */
static int connection_closed = 0;	/* Connection to client closed. */
static u_int buffer_high;	/* "Soft" max buffer size. */

/*
 * This SIGCHLD kludge is used to detect when the child exits.  The server
 * will exit after that, as soon as forwarded connections have terminated.
 */

static volatile sig_atomic_t child_terminated = 0;	/* The child has terminated. */

/* Cleanup on signals (!use_privsep case only) */
static volatile sig_atomic_t received_sigterm = 0;

/* prototypes */
static void server_init_dispatch(void);

/*
 * we write to this pipe if a SIGCHLD is caught in order to avoid
 * the race between select() and child_terminated
 */
static int notify_pipe[2];
#endif

#if 0
static void
notify_setup(void)
{
	if (pipe(notify_pipe) < 0) {
		error("pipe(notify_pipe) failed %s", strerror(errno));
	} else if ((fcntl(notify_pipe[0], F_SETFD, 1) == -1) ||
	    (fcntl(notify_pipe[1], F_SETFD, 1) == -1)) {
		error("fcntl(notify_pipe, F_SETFD) failed %s", strerror(errno));
		close(notify_pipe[0]);
		close(notify_pipe[1]);
	} else {
		set_nonblock(notify_pipe[0]);
		set_nonblock(notify_pipe[1]);
		return;
	}
	notify_pipe[0] = -1;	/* read end */
	notify_pipe[1] = -1;	/* write end */
}
static void
notify_parent(void)
{
	if (notify_pipe[1] != -1)
		write(notify_pipe[1], "", 1);
}
static void
notify_prepare(fd_set *readset)
{
	if (notify_pipe[0] != -1)
		FD_SET(notify_pipe[0], readset);
}
static void
notify_done(fd_set *readset)
{
	char c;

	if (notify_pipe[0] != -1 && FD_ISSET(notify_pipe[0], readset))
		while (read(notify_pipe[0], &c, 1) != -1)
			debug2("notify_done: reading");
}

/*ARGSUSED*/
static void
sigchld_handler(int sig)
{
	int save_errno = errno;
	child_terminated = 1;
#ifndef _UNICOS
	mysignal(SIGCHLD, sigchld_handler);
#endif
	notify_parent();
	errno = save_errno;
}

/*ARGSUSED*/
static void
sigterm_handler(int sig)
{
	received_sigterm = sig;
}

/*
 * Make packets from buffered stderr data, and buffer it for sending
 * to the client.
 */
static void
make_packets_from_stderr_data(void)
{
	u_int len;

	/* Send buffered stderr data to the client. */
	while (buffer_len(&stderr_buffer) > 0 &&
	    packet_not_very_much_data_to_write()) {
		len = buffer_len(&stderr_buffer);
		if (packet_is_interactive()) {
			if (len > 512)
				len = 512;
		} else {
			/* Keep the packets at reasonable size. */
			if (len > packet_get_maxsize())
				len = packet_get_maxsize();
		}
		packet_start(SSH_SMSG_STDERR_DATA);
		packet_put_string(buffer_ptr(&stderr_buffer), len);
		packet_send();
		buffer_consume(&stderr_buffer, len);
		stderr_bytes += len;
	}
}

/*
 * Make packets from buffered stdout data, and buffer it for sending to the
 * client.
 */
static void
make_packets_from_stdout_data(void)
{
	u_int len;

	/* Send buffered stdout data to the client. */
	while (buffer_len(&stdout_buffer) > 0 &&
	    packet_not_very_much_data_to_write()) {
		len = buffer_len(&stdout_buffer);
		if (packet_is_interactive()) {
			if (len > 512)
				len = 512;
		} else {
			/* Keep the packets at reasonable size. */
			if (len > packet_get_maxsize())
				len = packet_get_maxsize();
		}
		packet_start(SSH_SMSG_STDOUT_DATA);
		packet_put_string(buffer_ptr(&stdout_buffer), len);
		packet_send();
		buffer_consume(&stdout_buffer, len);
		stdout_bytes += len;
	}
}

static void
client_alive_check(void)
{
	int channel_id;

	/* timeout, check to see how many we have had */
	if (++keep_alive_timeouts > options.client_alive_count_max) {
		logit("Timeout, client not responding.");
		cleanup_exit(255);
	}

	/*
	 * send a bogus global/channel request with "wantreply",
	 * we should get back a failure
	 */
	if ((channel_id = channel_find_open()) == -1) {
		packet_start(SSH2_MSG_GLOBAL_REQUEST);
		packet_put_cstring("keepalive@openssh.com");
		packet_put_char(1);	/* boolean: want reply */
	} else {
		channel_request_start(channel_id, "keepalive@openssh.com", 1);
	}
	packet_send();
}
#endif

/*
 * Sleep in select() until we can do something.  This will initialize the
 * select masks.  Upon return, the masks will indicate which descriptors
 * have data or can accept data.  Optionally, a maximum time can be specified
 * for the duration of the wait (0 = infinite).
 */
void
CoreConnection::wait_until_can_do_something
  (fd_set **readsetp, fd_set **writesetp,
   u_int max_time_milliseconds)
{
	struct timeval tv, *tvp;
	int ret;
	int client_alive_scheduled = 0;
	int program_alive_scheduled = 0;

#if 0 //FIXME should be enabled
  /*
	 * if using client_alive, set the max timeout accordingly,
	 * and indicate that this particular timeout was for client
	 * alive by setting the client_alive_scheduled flag.
	 *
	 * this could be randomized somewhat to make traffic
	 * analysis more difficult, but we're not doing it yet.
	 */
	if (/*compat20*/ true &&
	    max_time_milliseconds == 0 && options.client_alive_interval) {
		client_alive_scheduled = 1;
		max_time_milliseconds = options.client_alive_interval * 1000;
	}
#endif

	/* Allocate and update select() masks for channel descriptors. */
  // It is not used in coreSSH
	//channel_prepare_select(readsetp, writesetp, maxfdp, nallocp, 0);
  FD_ZERO (*readsetp);
  FD_ZERO (*writesetp);
	FD_SET(connection_in, *readsetp);

	//notify_prepare(*readsetp); //notify pipe

	/*
	 * If we have buffered packet data going to the client, mark that
	 * descriptor.
	 */
	if (buffer_len (&output) != 0)
		FD_SET(connection_out, *writesetp);

	/*
	 * If child has terminated and there is enough buffer space to read
	 * from it, then read as much as is available and exit.
	 */ // TODO
	/*if (child_terminated && packet_not_very_much_data_to_write())
		if (max_time_milliseconds == 0 || client_alive_scheduled)
			max_time_milliseconds = 100;*/

	if (max_time_milliseconds == 0)
		tvp = NULL;
	else {
		tv.tv_sec = max_time_milliseconds / 1000;
		tv.tv_usec = 1000 * (max_time_milliseconds % 1000);
		tvp = &tv;
	}

	/* Wait for something to happen, or the timeout to expire. */
  ret = ::select(0 /*ignored*/, *readsetp, *writesetp, NULL, tvp);

	if (ret == SOCKET_ERROR) {
    const int err = ::WSAGetLastError ();
    FD_ZERO (*readsetp);
		FD_ZERO (*writesetp);
		if (err != WSAEINTR)
			error("select: %.100s", sWinErrMsg (err).c_str ());
        // FIXME UNICODE
	} else {
		if (ret == 0 /*&& client_alive_scheduled*/) //FIXME
			; //FIXME client_alive_check();
	}

	//notify_done(*readsetp); //notify pipe
}

/*
 * Processes input from the client and the program.  Input data is stored
 * in buffers and processed later.
 */
void
CoreConnection::process_input(fd_set *readset)
{
	int len;
	char buf[16384];

	/* Read and buffer any input data from the client. */
	if (FD_ISSET(connection_in, readset)) {
    len = ::recv(connection_in, buf, sizeof(buf), 0);
		if (len == 0) {
			verbose("Connection closed by %.100s",
			    socket->get_peer_address ().get_ip ().c_str ());
			connection_closed = true;
		} 
    else if (len < 0) 
    {
      const int err = ::WSAGetLastError ();
			if (err != WSAEINTR &&
			    err != WSAEWOULDBLOCK) 
      {
				verbose("Read error from remote host "
				    "%.100s: %.100s",
            socket->get_peer_address ().get_ip ().c_str (),
            sWinErrMsg (err).c_str ());
        // FIXME UNICODE
				cleanup_exit(255);
			}
		} else {
			/* Buffer any received data. */
			packet_process_incoming(buf, len);
		}
	}
}

/*
 * Sends data from internal buffers to client program stdin.
 */
void
CoreConnection::process_output(fd_set *writeset)
{
	/* Send any buffered packet data to the client. */
	if (FD_ISSET(connection_out, writeset))
		packet_write_poll();
}

#if 0
/*
 * Wait until all buffered output has been sent to the client.
 * This is used when the program terminates.
 */
static void
drain_output(void)
{
	/* Send any buffered stdout data to the client. */
	if (buffer_len(&stdout_buffer) > 0) {
		packet_start(SSH_SMSG_STDOUT_DATA);
		packet_put_string(buffer_ptr(&stdout_buffer),
				  buffer_len(&stdout_buffer));
		packet_send();
		/* Update the count of sent bytes. */
		stdout_bytes += buffer_len(&stdout_buffer);
	}
	/* Send any buffered stderr data to the client. */
	if (buffer_len(&stderr_buffer) > 0) {
		packet_start(SSH_SMSG_STDERR_DATA);
		packet_put_string(buffer_ptr(&stderr_buffer),
				  buffer_len(&stderr_buffer));
		packet_send();
		/* Update the count of sent bytes. */
		stderr_bytes += buffer_len(&stderr_buffer);
	}
	/* Wait until all buffered data has been written to the client. */
	packet_write_wait();
}

static void
collect_children(void)
{
	pid_t pid;
	sigset_t oset, nset;
	int status;

	/* block SIGCHLD while we check for dead children */
	sigemptyset(&nset);
	sigaddset(&nset, SIGCHLD);
	sigprocmask(SIG_BLOCK, &nset, &oset);
	if (child_terminated) {
		debug("Received SIGCHLD.");
		while ((pid = waitpid(-1, &status, WNOHANG)) > 0 ||
		    (pid < 0 && errno == EINTR))
			if (pid > 0)
				session_close_by_pid(pid, status);
		child_terminated = 0;
	}
	sigprocmask(SIG_SETMASK, &oset, NULL);
}

void
server_loop2(Authctxt *authctxt)
{
	fd_set *readset = NULL, *writeset = NULL;
	int rekeying = 0, max_fd, nalloc = 0;

	debug("Entering interactive session for SSH2.");

	mysignal(SIGCHLD, sigchld_handler);
	child_terminated = 0;
	connection_in = packet_get_connection_in();
	connection_out = packet_get_connection_out();

	if (!use_privsep) {
		signal(SIGTERM, sigterm_handler);
		signal(SIGINT, sigterm_handler);
		signal(SIGQUIT, sigterm_handler);
	}

	notify_setup();

	max_fd = MAX(connection_in, connection_out);
	max_fd = MAX(max_fd, notify_pipe[0]);

	server_init_dispatch();

	for (;;) {
		process_buffered_input_packets();

		rekeying = (xxx_kex != NULL && !xxx_kex->done);

		if (!rekeying && packet_not_very_much_data_to_write())
			channel_output_poll();
		wait_until_can_do_something(&readset, &writeset, &max_fd,
		    &nalloc, 0);

		if (received_sigterm) {
			logit("Exiting on signal %d", received_sigterm);
			/* Clean up sessions, utmp, etc. */
			cleanup_exit(255);
		}

		collect_children();
		if (!rekeying) {
			channel_after_select(readset, writeset);
			if (packet_need_rekeying()) {
				debug("need rekeying");
				xxx_kex->done = 0;
				kex_send_kexinit(xxx_kex);
			}
		}
		process_input(readset);
		if (connection_closed)
			break;
		process_output(writeset);
	}
	collect_children();

	if (readset)
		xfree(readset);
	if (writeset)
		xfree(writeset);

	/* free all channels, no more reads and writes */
	channel_free_all();

	/* free remaining sessions, e.g. remove wtmp entries */
	session_destroy_all(NULL);
}

static void
server_input_keep_alive(int type, u_int32_t seq, void *ctxt)
{
	debug("Got %d/%u for keepalive", type, seq);
	/*
	 * reset timeout, since we got a sane answer from the client.
	 * even if this was generated by something other than
	 * the bogus CHANNEL_REQUEST we send for keepalives.
	 */
	keep_alive_timeouts = 0;
}

static void
server_input_stdin_data(int type, u_int32_t seq, void *ctxt)
{
	char *data;
	u_int data_len;

	/* Stdin data from the client.  Append it to the buffer. */
	/* Ignore any data if the client has closed stdin. */
	if (fdin == -1)
		return;
	data = packet_get_string(&data_len);
	packet_check_eom();
	buffer_append(&stdin_buffer, data, data_len);
	memset(data, 0, data_len);
	xfree(data);
}

static void
server_input_eof(int type, u_int32_t seq, void *ctxt)
{
	/*
	 * Eof from the client.  The stdin descriptor to the
	 * program will be closed when all buffered data has
	 * drained.
	 */
	debug("EOF received for stdin.");
	packet_check_eom();
	stdin_eof = 1;
}

static void
server_input_window_size(int type, u_int32_t seq, void *ctxt)
{
	u_int row = packet_get_int();
	u_int col = packet_get_int();
	u_int xpixel = packet_get_int();
	u_int ypixel = packet_get_int();

	debug("Window change received.");
	packet_check_eom();
	if (fdin != -1)
		pty_change_window_size(fdin, row, col, xpixel, ypixel);
}

static Channel *
server_request_direct_tcpip(void)
{
	Channel *c;
	char *target, *originator;
	int target_port, originator_port;

	target = packet_get_string(NULL);
	target_port = packet_get_int();
	originator = packet_get_string(NULL);
	originator_port = packet_get_int();
	packet_check_eom();

	debug("server_request_direct_tcpip: originator %s port %d, target %s "
	    "port %d", originator, originator_port, target, target_port);

	/* XXX check permission */
	c = channel_connect_to(target, target_port,
	    "direct-tcpip", "direct-tcpip");

	xfree(originator);
	xfree(target);

	return c;
}

static Channel *
server_request_tun(void)
{
	Channel *c = NULL;
	int mode, tun;
	int sock;

	mode = packet_get_int();
	switch (mode) {
	case SSH_TUNMODE_POINTOPOINT:
	case SSH_TUNMODE_ETHERNET:
		break;
	default:
		packet_send_debug("Unsupported tunnel device mode.");
		return NULL;
	}
	if ((options.permit_tun & mode) == 0) {
		packet_send_debug("Server has rejected tunnel device "
		    "forwarding");
		return NULL;
	}

	tun = packet_get_int();
	if (forced_tun_device != -1) {
		if (tun != SSH_TUNID_ANY && forced_tun_device != tun)
			goto done;
		tun = forced_tun_device;
	}
	sock = tun_open(tun, mode);
	if (sock < 0)
		goto done;
	c = channel_new("tun", SSH_CHANNEL_OPEN, sock, sock, -1,
	    CHAN_TCP_WINDOW_DEFAULT, CHAN_TCP_PACKET_DEFAULT, 0, "tun", 1);
	c->datagram = 1;
#if defined(SSH_TUN_FILTER)
	if (mode == SSH_TUNMODE_POINTOPOINT)
		channel_register_filter(c->self, sys_tun_infilter,
		    sys_tun_outfilter, NULL, NULL);
#endif

 done:
	if (c == NULL)
		packet_send_debug("Failed to open the tunnel device.");
	return c;
}
#endif

Channel *
CoreConnection::server_request_session 
  (const ChannelPars& chPars)
{
	Channel *c;

	debug("input_session_request");
	packet_check_eom(this);

	if (no_more_sessions) {
		packet_disconnect("Possible attack: attempt to open a session "
		    "after additional sessions disabled");
	}

  // create a channel
  c = ChannelRepository::create_object (chPars);

  // create a session associated with the channel
  SessionPars sessPars;
  sessPars.authctxt = authctxt;
  sessPars.chanid = c->self;
  sessPars.connection = this;
  (void) SessionRepository::create_object (sessPars); 
  //TODO session creation fail conditions from OpenSSH
  //{
	//	debug("session open failed, free channel %d", c->self);
	//	channel_free(c);
	//	return NULL;
	//}

  // FIXME: check this:
	/*
	 * A server session has no fd to read or write until a
	 * CHANNEL_REQUEST for a shell is made, so we set the type to
	 * SSH_CHANNEL_LARVAL.  Additionally, a callback for handling all
	 * CHANNEL_REQUEST messages is registered.
	 */

	//channel_register_cleanup(c->self, session_close_by_channel, 0);
	return c;
}

void 
CoreConnection::server_input_channel_open
  (int type, u_int32_t seq, void *ctxt)
{
	Channel *c = NULL;
	char *ctype;
	//int rchan;
	u_int /*rmaxpack, rwindow,*/ len;
  SessionChannelPars chPars;

	ctype = packet_get_string(&len);
  chPars.remote_id = packet_get_int();
	chPars.rwindow = packet_get_int();
	chPars.rmaxpack = packet_get_int();
  chPars.connection = this;

	debug("server_input_channel_open: ctype %s rchan %d win %d max %d",
	    ctype, 
      (int) chPars.remote_id, 
      (int) chPars.rwindow, 
      (int) chPars.rmaxpack
      );

	if (strcmp(ctype, "session") == 0) {
		c = server_request_session(chPars);
	} 
  /*else if (strcmp(ctype, "direct-tcpip") == 0) {
		c = server_request_direct_tcpip();
	} */
	if (c != NULL) {
		debug("server_input_channel_open: confirm %s", ctype);
		// FIXME if (c->type != SSH_CHANNEL_CONNECTING) {
			packet_start(SSH2_MSG_CHANNEL_OPEN_CONFIRMATION);
			packet_put_int(c->get_remote_id ());
			packet_put_int(c->self);
			packet_put_int(c->get_local_window ());
			packet_put_int(c->get_local_maxpacket ());
			packet_send();
		//}
	/*} else {
		debug("server_input_channel_open: failure %s", ctype);
		packet_start(SSH2_MSG_CHANNEL_OPEN_FAILURE);
		packet_put_int(rchan);
		packet_put_int(SSH2_OPEN_ADMINISTRATIVELY_PROHIBITED);
		if (!(datafellows & SSH_BUG_OPENFAILURE)) {
			packet_put_cstring("open failed");
			packet_put_cstring("");
		}
		packet_send();*/
	}
	xfree(ctype);
}

#if 0
static void
server_input_global_request(int type, u_int32_t seq, void *ctxt)
{
	char *rtype;
	int want_reply;
	int success = 0;

	rtype = packet_get_string(NULL);
	want_reply = packet_get_char();
	debug("server_input_global_request: rtype %s want_reply %d", rtype, want_reply);

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
	} else if (strcmp(rtype, "no-more-sessions@openssh.com") == 0) {
		no_more_sessions = 1;
		success = 1;
	}
	if (want_reply) {
		packet_start(success ?
		    SSH2_MSG_REQUEST_SUCCESS : SSH2_MSG_REQUEST_FAILURE);
		packet_send();
		packet_write_wait();
	}
	xfree(rtype);
}
#endif

void
CoreConnection::server_input_channel_req
  (int type, u_int32_t seq, void *ctxt)
{
	Channel *c;
	int id, reply, success = 0;
	char *rtype;

	id = packet_get_int();
	rtype = packet_get_string(NULL);
	reply = packet_get_char();

	debug("server_input_channel_req: channel %d request %s reply %d",
	    id, rtype, reply);

  //TODO doesn't check private channels (see OpenSSH)
  if ((c = ChannelRepository::get_object_by_id (id)) == NULL)
		packet_disconnect("server_input_channel_req: "
		    "unknown channel %d", id);
	
  if (
      (
       c->channelStateIs ("larval") ||
	     c->channelStateIs ("open")
       ) 
       && c->ctype == "session"
      )
  {
    success = Session::session_input_channel_req 
      (this, c, rtype);
  }

	if (reply) 
  {
		packet_start(success ?
		    SSH2_MSG_CHANNEL_SUCCESS : SSH2_MSG_CHANNEL_FAILURE);
		packet_put_int(c->get_remote_id ());
		packet_send();
	}
	xfree(rtype);
}

