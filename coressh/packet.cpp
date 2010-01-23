/* $OpenBSD: packet.c,v 1.157 2008/07/10 18:08:11 markus Exp $ */
/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * This file contains code implementing the packet protocol and communication
 * with the other side.  This same code is used both on client and server side.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 *
 * SSH2 packet format added by Markus Friedl.
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
 
#include "packet.h"
#include "ssh2.h"
#include "cipher.h"
#include "key.h"
#include "kex.h"
#include "canohost.h"
#include "misc.h"
#include "queue.h"

namespace coressh {

#if 0
/* Set to true if the connection is interactive. */
static int interactive_mode = 0;

/* Session key for protocol v1 */
static u_char ssh1_key[SSH_SESSION_KEY_LENGTH];
static u_int ssh1_keylen;

#endif

/* Sets the connection into non-blocking mode. */
#if 0
void
packet_set_nonblocking(void)
{
	/* Set the socket into non-blocking mode. */
	set_nonblock(connection_in);

	if (connection_out != connection_in)
		set_nonblock(connection_out);
}

/* Returns the socket used for reading. */

int
packet_get_connection_in(void)
{
	return connection_in;
}
#endif

/* Closes the connection and clears and frees internal data structures. */

#if 0
/* Sets remote side protocol flags. */

void
packet_set_protocol_flags(u_int protocol_flags)
{
	remote_protocol_flags = protocol_flags;
}

static void
packet_send1(void)
{
	u_char buf[8], *cp;
	int i, padding, len;
	u_int checksum;
	u_int32_t rnd = 0;

	/*
	 * If using packet compression, compress the payload of the outgoing
	 * packet.
	 */
	if (packet_compression) {
		buffer_clear(&compression_buffer);
		/* Skip padding. */
		buffer_consume(&outgoing_packet, 8);
		/* padding */
		buffer_append(&compression_buffer, "\0\0\0\0\0\0\0\0", 8);
		buffer_compress(&outgoing_packet, &compression_buffer);
		buffer_clear(&outgoing_packet);
		buffer_append(&outgoing_packet, buffer_ptr(&compression_buffer),
		    buffer_len(&compression_buffer));
	}
	/* Compute packet length without padding (add checksum, remove padding). */
	len = buffer_len(&outgoing_packet) + 4 - 8;

	/* Insert padding. Initialized to zero in packet_start1() */
	padding = 8 - len % 8;
	if (!send_context.plaintext) {
		cp = buffer_ptr(&outgoing_packet);
		for (i = 0; i < padding; i++) {
			if (i % 4 == 0)
				rnd = arc4random();
			cp[7 - i] = rnd & 0xff;
			rnd >>= 8;
		}
	}
	buffer_consume(&outgoing_packet, 8 - padding);

	/* Add check bytes. */
	checksum = ssh_crc32(buffer_ptr(&outgoing_packet),
	    buffer_len(&outgoing_packet));
	put_u32(buf, checksum);
	buffer_append(&outgoing_packet, buf, 4);

#ifdef PACKET_DEBUG
	fprintf(stderr, "packet_send plain: ");
	buffer_dump(&outgoing_packet);
#endif

	/* Append to output. */
	put_u32(buf, len);
	buffer_append(&output, buf, 4);
	cp = buffer_append_space(&output, buffer_len(&outgoing_packet));
	cipher_crypt(&send_context, cp, buffer_ptr(&outgoing_packet),
	    buffer_len(&outgoing_packet));

#ifdef PACKET_DEBUG
	fprintf(stderr, "encrypted: ");
	buffer_dump(&output);
#endif
	p_send.packets++;
	p_send.bytes += len + buffer_len(&outgoing_packet);
	buffer_clear(&outgoing_packet);

	/*
	 * Note that the packet is now only buffered in output.  It won't be
	 * actually sent until packet_write_wait or packet_write_poll is
	 * called.
	 */
}
#endif

/* Checks if a full packet is available in the data received so far via
 * packet_process_incoming.  If so, reads the packet; otherwise returns
 * SSH_MSG_NONE.  This does not wait for data from the connection.
 *
 * SSH_MSG_DISCONNECT is handled specially here.  Also,
 * SSH_MSG_IGNORE messages are skipped by this function and are never returned
 * to higher levels.
 */

static int
packet_read_poll1(void)
{
	u_int len, padded_len;
	u_char *cp, type;
	u_int checksum, stored_checksum;

	/* Check if input size is less than minimum packet size. */
	if (buffer_len(&input) < 4 + 8)
		return SSH_MSG_NONE;
	/* Get length of incoming packet. */
	cp = buffer_ptr(&input);
	len = get_u32(cp);
	if (len < 1 + 2 + 2 || len > 256 * 1024)
		packet_disconnect("Bad packet length %u.", len);
	padded_len = (len + 8) & ~7;

	/* Check if the packet has been entirely received. */
	if (buffer_len(&input) < 4 + padded_len)
		return SSH_MSG_NONE;

	/* The entire packet is in buffer. */

	/* Consume packet length. */
	buffer_consume(&input, 4);

	/*
	 * Cryptographic attack detector for ssh
	 * (C)1998 CORE-SDI, Buenos Aires Argentina
	 * Ariel Futoransky(futo@core-sdi.com)
	 */
	if (!receive_context.plaintext) {
		switch (detect_attack(buffer_ptr(&input), padded_len)) {
		case DEATTACK_DETECTED:
			packet_disconnect("crc32 compensation attack: "
			    "network attack detected");
		case DEATTACK_DOS_DETECTED:
			packet_disconnect("deattack denial of "
			    "service detected");
		}
	}

	/* Decrypt data to incoming_packet. */
	buffer_clear(&incoming_packet);
	cp = buffer_append_space(&incoming_packet, padded_len);
	cipher_crypt(&receive_context, cp, buffer_ptr(&input), padded_len);

	buffer_consume(&input, padded_len);

#ifdef PACKET_DEBUG
	fprintf(stderr, "read_poll plain: ");
	buffer_dump(&incoming_packet);
#endif

	/* Compute packet checksum. */
	checksum = ssh_crc32(buffer_ptr(&incoming_packet),
	    buffer_len(&incoming_packet) - 4);

	/* Skip padding. */
	buffer_consume(&incoming_packet, 8 - len % 8);

	/* Test check bytes. */
	if (len != buffer_len(&incoming_packet))
		packet_disconnect("packet_read_poll1: len %d != buffer_len %d.",
		    len, buffer_len(&incoming_packet));

	cp = (u_char *)buffer_ptr(&incoming_packet) + len - 4;
	stored_checksum = get_u32(cp);
	if (checksum != stored_checksum)
		packet_disconnect("Corrupted check bytes on input.");
	buffer_consume_end(&incoming_packet, 4);

	if (packet_compression) {
		buffer_clear(&compression_buffer);
		buffer_uncompress(&incoming_packet, &compression_buffer);
		buffer_clear(&incoming_packet);
		buffer_append(&incoming_packet, buffer_ptr(&compression_buffer),
		    buffer_len(&compression_buffer));
	}
	p_read.packets++;
	p_read.bytes += padded_len + 4;
	type = buffer_get_char(&incoming_packet);
	if (type < SSH_MSG_MIN || type > SSH_MSG_MAX)
		packet_disconnect("Invalid ssh1 packet type: %d", type);
	return type;
}

static int
packet_read_poll2(u_int32_t *seqnr_p)
{
	static u_int packet_length = 0;
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

	if (packet_length == 0) {
		/*
		 * check if input size is less than the cipher block size,
		 * decrypt first block and extract length of incoming packet
		 */
		if (buffer_len(&input) < block_size)
			return SSH_MSG_NONE;
		buffer_clear(&incoming_packet);
		cp = buffer_append_space(&incoming_packet, block_size);
		cipher_crypt(&receive_context, cp, buffer_ptr(&input),
		    block_size);
		cp = buffer_ptr(&incoming_packet);
		packet_length = get_u32(cp);
		if (packet_length < 1 + 4 || packet_length > 256 * 1024) {
#ifdef PACKET_DEBUG
			buffer_dump(&incoming_packet);
#endif
			packet_disconnect("Bad packet length %-10u",
			    packet_length);
		}
		DBG(debug("input: packet len %u", packet_length+4));
		buffer_consume(&input, block_size);
	}
	/* we have a partial packet of block_size bytes */
	need = 4 + packet_length - block_size;
	DBG(debug("partial packet %d, need %d, maclen %d", block_size,
	    need, maclen));
	if (need % block_size != 0) {
		logit("padding error: need %d block %d mod %d",
		    need, block_size, need % block_size);
		packet_disconnect("Bad packet length %-10u", packet_length);
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
	cp = buffer_append_space(&incoming_packet, need);
	cipher_crypt(&receive_context, cp, buffer_ptr(&input), need);
	buffer_consume(&input, need);
	/*
	 * compute MAC over seqnr and packet,
	 * increment sequence number for incoming packet
	 */
	if (mac && mac->enabled) {
		macbuf = mac_compute(mac, p_read.seqnr,
		    buffer_ptr(&incoming_packet),
		    buffer_len(&incoming_packet));
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
		if (!(datafellows & SSH_BUG_NOREKEY))
			fatal("XXX too many packets with same key");
	p_read.blocks += (packet_length + 4) / block_size;
	p_read.bytes += packet_length + 4;

	/* get padlen */
	cp = buffer_ptr(&incoming_packet);
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
	packet_length = 0;
	return type;
}

/*
 * Sends a diagnostic message from the server to the client.  This message
 * can be sent at any time (but not while constructing another message). The
 * message is printed immediately, but only if the client is being executed
 * in verbose mode.  These messages are primarily intended to ease debugging
 * authentication problems.   The length of the formatted message must not
 * exceed 1024 bytes.  This will automatically call packet_write_wait.
 */

void
packet_send_debug(const char *fmt,...)
{
	char buf[1024];
	va_list args;

	if (compat20 && (datafellows & SSH_BUG_DEBUG))
		return;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (compat20) {
		packet_start(SSH2_MSG_DEBUG);
		packet_put_char(0);	/* bool: always display */
		packet_put_cstring(buf);
		packet_put_cstring("");
	} else {
		packet_start(SSH_MSG_DEBUG);
		packet_put_cstring(buf);
	}
	packet_send();
	packet_write_wait();
}


/* Returns true if there is not too much data to write to the connection. */

int
packet_not_very_much_data_to_write(void)
{
	if (interactive_mode)
		return buffer_len(&output) < 16384;
	else
		return buffer_len(&output) < 128 * 1024;
}


static void
packet_set_tos(int interactive)
{
#if defined(IP_TOS) && !defined(IP_TOS_IS_BROKEN)
	int tos = interactive ? IPTOS_LOWDELAY : IPTOS_THROUGHPUT;

	if (!packet_connection_is_on_socket() ||
	    !packet_connection_is_ipv4())
		return;
	if (setsockopt(connection_in, IPPROTO_IP, IP_TOS, &tos,
	    sizeof(tos)) < 0)
		error("setsockopt IP_TOS %d: %.100s:",
		    tos, strerror(errno));
#endif
}

/* Informs that the current session is interactive.  Sets IP flags for that. */

void
packet_set_interactive(int interactive)
{
	static int called = 0;

	if (called)
		return;
	called = 1;

	/* Record that we are in interactive mode. */
	interactive_mode = interactive;

	/* Only set socket options if using a socket.  */
	if (!packet_connection_is_on_socket())
		return;
	set_nodelay(connection_in);
	packet_set_tos(interactive);
}

/* Returns true if the current connection is interactive. */

int
packet_is_interactive(void)
{
	return interactive_mode;
}

int
packet_set_maxsize(u_int s)
{
	static int called = 0;

	if (called) {
		logit("packet_set_maxsize: called twice: old %d new %d",
		    max_packet_size, s);
		return -1;
	}
	if (s < 4 * 1024 || s > 1024 * 1024) {
		logit("packet_set_maxsize: bad size %d", s);
		return -1;
	}
	called = 1;
	debug("packet_set_maxsize: setting to %d", s);
	max_packet_size = s;
	return s;
}

/* roundup current message to pad bytes */
void
packet_add_padding(u_char pad)
{
	extra_pad = pad;
}

/*
 * 9.2.  Ignored Data Message
 *
 *   byte      SSH_MSG_IGNORE
 *   string    data
 *
 * All implementations MUST understand (and ignore) this message at any
 * time (after receiving the protocol version). No implementation is
 * required to send them. This message can be used as an additional
 * protection measure against advanced traffic analysis techniques.
 */
void
packet_send_ignore(int nbytes)
{
	u_int32_t rnd = 0;
	int i;

	packet_start(compat20 ? SSH2_MSG_IGNORE : SSH_MSG_IGNORE);
	packet_put_int(nbytes);
	for (i = 0; i < nbytes; i++) {
		if (i % 4 == 0)
			rnd = arc4random();
		packet_put_char((u_char)rnd & 0xff);
		rnd >>= 8;
	}
}

#define MAX_PACKETS	(1U<<31)
int
packet_need_rekeying(void)
{
	if (datafellows & SSH_BUG_NOREKEY)
		return 0;
	return
	    (p_send.packets > MAX_PACKETS) ||
	    (p_read.packets > MAX_PACKETS) ||
	    (max_blocks_out && (p_send.blocks > max_blocks_out)) ||
	    (max_blocks_in  && (p_read.blocks > max_blocks_in));
}

void
packet_set_rekey_limit(u_int32_t bytes)
{
	rekey_limit = bytes;
}

void
packet_set_server(void)
{
	server_side = 1;
}

void
packet_set_authenticated(void)
{
	after_authentication = 1;
}

}
