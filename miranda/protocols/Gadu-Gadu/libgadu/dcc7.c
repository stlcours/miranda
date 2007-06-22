/* $Id: dcc7.c,v 1.1 2007-06-03 15:46:09 wojtekka Exp $ */

/*
 *  (C) Copyright 2001-2007 Wojtek Kaniewski <wojtekka@irc.pl>
 *                          Tomasz Chili�ski <chilek@chilan.com>
 *                          Adam Wysocki <gophi@ekg.chmurka.net>
 *  
 *  Thanks to Jakub Zawadzki <darkjames@darkjames.ath.cx>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License Version
 *  2.1 as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110,
 *  USA.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef sun
#  include <sys/filio.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "compat.h"
#include "libgadu.h"

static int gg_dcc7_session_add(struct gg_session *sess, struct gg_dcc7 *dcc)
{
	if (!sess || !dcc || dcc->next) {
		errno = EINVAL;
		return -1;
	}

	dcc->next = sess->dcc7_list;
	sess->dcc7_list = dcc;

	return 0;
}

static int gg_dcc7_session_remove(struct gg_session *sess, struct gg_dcc7 *dcc)
{
	struct gg_dcc7 *tmp;

	if (!sess || !dcc) {
		errno = EINVAL;
		return -1;
	}

	if (sess->dcc7_list == dcc) {
		sess->dcc7_list = dcc->next;
		dcc->next = NULL;
		return 0;
	}

	for (tmp = sess->dcc7_list; tmp; tmp = tmp->next) {
		if (tmp->next == dcc) {
			tmp = dcc->next;
			dcc->next = NULL;
			return 0;
		}
	}

	errno = ENOENT;
	return -1;
}

static struct gg_dcc7 *gg_dcc7_session_find(struct gg_session *sess, gg_dcc7_id_t id)
{
	struct gg_dcc7 *tmp;

	for (tmp = sess->dcc7_list; tmp; tmp = tmp->next) {
		if (!memcmp(&tmp->cid, &id, sizeof(id)))
			return tmp;
	}

	return NULL;
}

static int gg_dcc7_connect(struct gg_session *sess, struct gg_dcc7 *dcc)
{
#ifdef SO_SNDTIMEO
	struct timeval tv;
#endif

	if (!sess || !dcc) {
		errno = EINVAL;
		return -1;
	}

	if ((dcc->fd = gg_connect(&dcc->remote_addr, dcc->remote_port, 1)) == -1) {
		gg_debug_session(sess, GG_DEBUG_MISC, "// gg_dcc7_connect() connection failed\n");
		return -1;
	}

	dcc->state = GG_STATE_CONNECTING;
	dcc->check = GG_CHECK_WRITE;
	dcc->timeout = GG_DEFAULT_TIMEOUT;

	// XXX to pewnie nie zadzia�a, zrobi� software'owy timeout

#ifdef SO_SNDTIMEO
	tv.tv_sec = GG_DCC7_TIMEOUT_CONNECT;
	tv.tv_usec = 0;

	setsockopt(dcc->fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv));
#endif

	return 0;
}

static int gg_dcc7_socket(struct gg_dcc7 *dcc, uint16_t port)
{
	struct sockaddr_in sin;
	int fd, bound = 0;

	if (!dcc) {
		errno = EINVAL;
		return -1;
	}

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_socket() can't create socket (%s)\n", strerror(errno));
		return -1;
	}

	// XXX losowa� porty?
	
	if (!port)
		port = GG_DEFAULT_DCC_PORT;

	while (!bound) {
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(port);

		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_socket() trying port %d\n", port);
		if (!bind(fd, (struct sockaddr*) &sin, sizeof(sin)))
			bound = 1;
		else {
			if (++port == 65535) {
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_socket() no free port found\n");
				close(fd);
				errno = ENOENT;
				return -1;
			}
		}
	}

	if (listen(fd, 1)) {
		int errsv = errno;
		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_socket() unable to listen (%s)\n", strerror(errno));
		close(fd);
		errno = errsv;
		return -1;
	}

	dcc->fd = fd;
	dcc->local_port = port;
	dcc->type = GG_SESSION_DCC7_SOCKET;
	
	dcc->state = GG_STATE_LISTENING;
	dcc->check = GG_CHECK_READ;
	dcc->timeout = GG_DCC7_TIMEOUT_FILE_ACK;

	return 0;
}

static int gg_dcc7_request_id(struct gg_session *sess, uint32_t type)
{
	struct gg_dcc7_id_request pkt;

	if (!sess) {
		errno = EFAULT;
		return -1;
	}

	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	if (type != GG_DCC7_TYPE_VOICE && type != GG_DCC7_TYPE_FILE) {
		errno = EINVAL;
		return -1;
	}
	
	memset(&pkt, 0, sizeof(pkt));
	pkt.type = gg_fix32(type);

	return gg_send_packet(sess, GG_DCC7_ID_REQUEST, &pkt, sizeof(pkt), NULL);
}

struct gg_dcc7 *gg_dcc7_send_file(struct gg_session *sess, uin_t rcpt, const char *filename, const char *filename1250, const char *hash)
{
	struct gg_dcc7 *dcc = NULL;
	const char *tmp;
	struct stat st;
#ifdef GG_CONFIG_MIRANDA
	FILE *fd = NULL;
#else
	int fd = -1;
#endif

	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc7_send_file(%p, %d, \"%s\", %p);\n", sess, rcpt, filename, hash);

	if (!sess || !rcpt || !filename) {
		errno = EINVAL;
		goto fail;
	}

	if (!filename1250)
		filename1250 = filename;

	if (!(dcc = malloc(sizeof(struct gg_dcc7)))) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_send_file() not enough memory\n");
		goto fail;
	}

	if (stat(filename, &st) == -1) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_send_file() stat() failed (%s)\n", strerror(errno));
		goto fail;
	}

	if ((st.st_mode & S_IFDIR)) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_send_file() that's a directory\n");
		errno = EINVAL;
		goto fail;
	}

#ifdef GG_CONFIG_MIRANDA
	if ((fd = fopen(filename, "rb")) == NULL) {
#else
	if ((fd = open(filename, O_RDONLY)) == -1) {
#endif
		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_send_file() open() failed (%s)\n", strerror(errno));
		goto fail;
	}

	if (gg_dcc7_request_id(sess, GG_DCC7_TYPE_FILE) == -1)
		goto fail;

	if ((tmp = strrchr(filename1250, '/')))
		filename1250 = tmp + 1;

	memset(dcc, 0, sizeof(struct gg_dcc7));
	dcc->type = GG_SESSION_DCC7_SEND;
	dcc->dcc_type = GG_DCC7_TYPE_FILE;
	dcc->state = GG_STATE_REQUESTING_ID;
	dcc->timeout = GG_DEFAULT_TIMEOUT;
	dcc->sess = sess;
	dcc->fd = -1;
	dcc->uin = sess->uin;
	dcc->peer_uin = rcpt;
	dcc->file_fd = fd;
	dcc->size = st.st_size;

	strncpy((char*) dcc->filename, filename1250, GG_DCC7_FILENAME_LEN - 1);
	dcc->filename[GG_DCC7_FILENAME_LEN] = 0;

	if (hash) {
		memcpy(dcc->hash, hash, GG_DCC7_HASH_LEN);
	} else {
		if (gg_file_hash_sha1(fd, dcc->hash) == -1)
			goto fail;
	}

	if (gg_dcc7_session_add(sess, dcc) == -1)
		goto fail;

	return dcc;

fail:
#ifdef GG_CONFIG_MIRANDA
	if (fd != NULL) {
		int errsv = errno;
		fclose(fd);
#else
	if (fd != -1) {
		int errsv = errno;
		close(fd);
#endif
		errno = errsv;
	}

	free(dcc);

	return NULL;
}

int gg_dcc7_accept(struct gg_dcc7 *dcc, unsigned int offset)
{
	struct gg_dcc7_accept pkt1;
	struct gg_dcc7_info pkt2;

	if (!dcc->sess) {
		errno = EFAULT;
		return -1;
	}

	// XXX da� mo�liwo�� konfiguracji?
	
	dcc->local_addr = dcc->sess->client_addr;

	if (gg_dcc7_socket(dcc, 0) == -1)
		return -1;

	memset(&pkt1, 0, sizeof(pkt1));
	pkt1.uin = gg_fix32(dcc->peer_uin);
	pkt1.id = dcc->cid;
	pkt1.offset = gg_fix32(offset);

	if (gg_send_packet(dcc->sess, GG_DCC7_ACCEPT, &pkt1, sizeof(pkt1), NULL) == -1)
		return -1;

	memset(&pkt2, 0, sizeof(pkt2));
	pkt2.uin = gg_fix32(dcc->peer_uin);
	pkt2.type = GG_DCC7_TYPE_P2P;
	pkt2.id = dcc->cid;
	snprintf((char*) pkt2.info, sizeof(pkt2.info), "%s %d", inet_ntoa(*((struct in_addr*) &dcc->local_addr)), dcc->local_port);

	return gg_send_packet(dcc->sess, GG_DCC7_INFO, &pkt2, sizeof(pkt2), NULL);
}

int gg_dcc7_reject(struct gg_dcc7 *dcc, int reason)
{
	struct gg_dcc7_reject pkt;

	if (!dcc->sess) {
		errno = EFAULT;
		return -1;
	}

	memset(&pkt, 0, sizeof(pkt));
	pkt.uin = gg_fix32(dcc->peer_uin);
	pkt.id = dcc->cid;
	pkt.reason = gg_fix32(reason);

	return gg_send_packet(dcc->sess, GG_DCC7_REJECT, &pkt, sizeof(pkt), NULL);
}

int gg_dcc7_handle_id(struct gg_session *sess, struct gg_event *e, void *payload, int len)
{
	struct gg_dcc7_id_reply *p = payload;
	struct gg_dcc7 *tmp;

	gg_debug_session(sess, GG_DEBUG_MISC, "// sess %p dcc_list %p\n", sess, sess->dcc7_list);

	for (tmp = sess->dcc7_list; tmp; tmp = tmp->next) {
		gg_debug_session(sess, GG_DEBUG_MISC, "// checking dcc %p, state %d, type %d\n", tmp, tmp->state, tmp->dcc_type);

		if (tmp->state != GG_STATE_REQUESTING_ID || tmp->dcc_type != (int)gg_fix32(p->type))
			continue;
		
		tmp->cid = p->id;

		switch (tmp->dcc_type) {
			case GG_DCC7_TYPE_FILE:
			{
				struct gg_dcc7_new s;

				memset(&s, 0, sizeof(s));
				s.id = tmp->cid;
				s.type = gg_fix32(GG_DCC7_TYPE_FILE);
				s.uin_from = gg_fix32(tmp->uin);
				s.uin_to = gg_fix32(tmp->peer_uin);
				s.size = gg_fix32(tmp->size);

				strncpy((char*) s.filename, (char*) tmp->filename, GG_DCC7_FILENAME_LEN);

				tmp->state = GG_STATE_WAITING_FOR_ACCEPT;
				tmp->timeout = GG_DCC7_TIMEOUT_FILE_ACK;

				return gg_send_packet(sess, GG_DCC7_NEW, &s, sizeof(s), NULL);
			}
		}
	}

	return 0;
}

int gg_dcc7_handle_accept(struct gg_session *sess, struct gg_event *e, void *payload, int len)
{
	struct gg_dcc7_accept *p = payload;
	struct gg_dcc7 *dcc;

	if (!(dcc = gg_dcc7_session_find(sess, p->id))) {
		// XXX wys�a� reject?
		return 0;
	}

	if (dcc->state != GG_STATE_WAITING_FOR_ACCEPT) {
		// XXX poinformowa�?
		return 0;
	}
	
	// XXX czy dla odwrotnego po��czenia powinni�my wywo�a� ju� zdarzenie GG_DCC7_ACCEPT?
	
	dcc->offset = gg_fix32(p->offset);
	dcc->state = GG_STATE_WAITING_FOR_INFO;

	return 0;
}

int gg_dcc7_handle_info(struct gg_session *sess, struct gg_event *e, void *payload, int len)
{
	struct gg_dcc7_info *p = payload;
	struct gg_dcc7 *dcc;
	char *tmp;

	if (!(dcc = gg_dcc7_session_find(sess, p->id)))
		return 0;
	
	if (p->type != GG_DCC7_TYPE_P2P) {
		// XXX poinformowa�?
		return 0;
	}

	if ((dcc->remote_addr = inet_addr(p->info)) == INADDR_NONE) {
		// XXX poinformowa�?
		return 0;
	}

	if (!(tmp = strchr(p->info, ' ')) || !(dcc->remote_port = atoi(tmp + 1))) {
		// XXX poinformowa�?
		return 0;
	}

	// je�li nadal czekamy na po��czenie przychodz�ce, a druga strona nie
	// daje rady i oferuje namiary na siebie, bierzemy co daj�.

	if (dcc->state != GG_STATE_WAITING_FOR_INFO && dcc->state != GG_STATE_LISTENING) {
		// XXX poinformowa�?
		return 0;
	}

	if (dcc->state == GG_STATE_LISTENING) {
		close(dcc->fd);
		dcc->fd = -1;
	}
	
	if (dcc->type == GG_SESSION_DCC7_SEND) {
		e->type = GG_EVENT_DCC7_ACCEPT;
		e->event.dcc7_accept.dcc7 = dcc;
		e->event.dcc7_accept.type = gg_fix32(p->type);
		e->event.dcc7_accept.remote_ip = dcc->remote_addr;
		e->event.dcc7_accept.remote_port = dcc->remote_port;
	}

	if (gg_dcc7_connect(sess, dcc) == -1) {
		// XXX poinformowa�?
		return 0;
	}

	return 0;
}

int gg_dcc7_handle_reject(struct gg_session *sess, struct gg_event *e, void *payload, int len)
{
	struct gg_dcc7_reject *p = payload;
	struct gg_dcc7 *dcc;

	if (!(dcc = gg_dcc7_session_find(sess, p->id)))
		return 0;
	
	if (dcc->state != GG_STATE_WAITING_FOR_ACCEPT) {
		// XXX poinformowa�?
		return 0;
	}

	e->type = GG_EVENT_DCC7_REJECT;
	e->event.dcc7_reject.dcc7 = dcc;
	e->event.dcc7_reject.reason = gg_fix32(p->reason);

	// XXX ustawi� state na rejected?

	return 0;
}

int gg_dcc7_handle_new(struct gg_session *sess, struct gg_event *e, void *payload, int len)
{
	struct gg_dcc7_new *p = payload;
	struct gg_dcc7 *dcc;

	switch (gg_fix32(p->type)) {
		case GG_DCC7_TYPE_FILE:
			if (!(dcc = malloc(sizeof(struct gg_dcc7)))) {
				gg_debug_session(sess, GG_DEBUG_MISC, "// gg_dcc7_handle_new() not enough memory\n");
				return -1;
			}
			
			memset(dcc, 0, sizeof(struct gg_dcc7));
			dcc->type = GG_SESSION_DCC7_GET;
			dcc->dcc_type = GG_DCC7_TYPE_FILE;
			dcc->fd = -1;
#ifdef GG_CONFIG_MIRANDA
			dcc->file_fd = NULL;
#else
			dcc->file_fd = -1;
#endif
			dcc->uin = sess->uin;
			dcc->peer_uin = gg_fix32(p->uin_from);
			dcc->cid = p->id;
			dcc->sess = sess;

			if (gg_dcc7_session_add(sess, dcc) == -1) {
				gg_debug_session(sess, GG_DEBUG_MISC, "// gg_dcc7_handle_new() unable to add to session\n");
				gg_dcc7_free(dcc);
				return -1;
			}

			dcc->size = gg_fix32(p->size);
			strncpy((char*) dcc->filename, (char*) p->filename, GG_DCC7_FILENAME_LEN - 1);
			dcc->filename[GG_DCC7_FILENAME_LEN] = 0;
			memcpy(dcc->hash, p->hash, GG_DCC7_HASH_LEN);

			e->type = GG_EVENT_DCC7_NEW;
			e->event.dcc7_new = dcc;

			break;

		case GG_DCC7_TYPE_VOICE:
			if (!(dcc = malloc(sizeof(struct gg_dcc7)))) {
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_handle_packet() not enough memory\n");
				return -1;
			}
			
			memset(dcc, 0, sizeof(struct gg_dcc7));

			dcc->type = GG_SESSION_DCC7_VOICE;
			dcc->dcc_type = GG_DCC7_TYPE_VOICE;
			dcc->fd = -1;
#ifdef GG_CONFIG_MIRANDA
			dcc->file_fd = NULL;
#else
			dcc->file_fd = -1;
#endif
			dcc->uin = sess->uin;
			dcc->peer_uin = gg_fix32(p->uin_from);
			dcc->cid = p->id;
			dcc->sess = sess;

			if (gg_dcc7_session_add(sess, dcc) == -1) {
				gg_debug_session(sess, GG_DEBUG_MISC, "// gg_dcc7_handle_new() unable to add to session\n");
				gg_dcc7_free(dcc);
				return -1;
			}

			e->type = GG_EVENT_DCC7_NEW;
			e->event.dcc7_new = dcc;

			break;

		default:
			gg_debug_session(sess, GG_DEBUG_MISC, "// gg_dcc7_handle_new() unknown dcc type (%04x) from %ld\n", gg_fix32(p->type), gg_fix32(p->uin_from));

			break;
	}

	return 0;
}

struct gg_event *gg_dcc7_watch_fd(struct gg_dcc7 *dcc)
{
	struct gg_event *e;

//	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc7_watch_fd(%p);\n", dcc);

	if (!dcc || (dcc->type != GG_SESSION_DCC7_SEND && dcc->type != GG_SESSION_DCC7_GET && dcc->type != GG_SESSION_DCC7_VOICE)) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() invalid argument\n");
		errno = EINVAL;
		return NULL;
	}

	if (!(e = malloc(sizeof(struct gg_event)))) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() not enough memory\n");
		return NULL;
	}

	memset(e, 0, sizeof(struct gg_event));
	e->type = GG_EVENT_NONE;

	switch (dcc->state) {
		case GG_STATE_LISTENING:
		{
			struct sockaddr_in sin;
			int fd, one = 1;
			unsigned int sin_len = sizeof(sin);

			gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() GG_STATE_LISTENING\n");

			if ((fd = accept(dcc->fd, (struct sockaddr*) &sin, &sin_len)) == -1) {
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() accept() failed (%s)\n", strerror(errno));
				return e;
			}

			gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() connection from %s:%d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));

#ifdef FIONBIO
			if (ioctl(fd, FIONBIO, &one) == -1) {
#else
			if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
#endif
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() can't set nonblocking (%s)\n", strerror(errno));
				close(fd);
				e->type = GG_EVENT_DCC7_ERROR;
				e->event.dcc_error = GG_ERROR_DCC7_HANDSHAKE;
				return e;
			}

			close(dcc->fd);
			dcc->fd = fd;

			dcc->state = GG_STATE_READING_ID;
			dcc->check = GG_CHECK_READ;
			dcc->timeout = GG_DEFAULT_TIMEOUT;
			dcc->incoming = 1;

			dcc->remote_port = ntohs(sin.sin_port);
			dcc->remote_addr = sin.sin_addr.s_addr;

			e->type = GG_EVENT_DCC7_CONNECTED;
			e->event.dcc7_connected.dcc7 = dcc;

			return e;
		}

		case GG_STATE_CONNECTING:
		{
			int res, error = 0;
			unsigned int error_size = sizeof(error);

			gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() GG_STATE_CONNECTING\n");

			if ((res = getsockopt(dcc->fd, SOL_SOCKET, SO_ERROR, (char *)&error, &error_size)) == -1 || error != 0) {
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() connection failed (%s)\n", (res == -1) ? strerror(errno) : strerror(error));
				e->type = GG_EVENT_DCC7_ERROR;
				e->event.dcc_error = GG_ERROR_DCC7_HANDSHAKE;
				return e;
			}

			gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() connected, sending id\n");

			dcc->state = GG_STATE_SENDING_ID;
			dcc->check = GG_CHECK_WRITE;
			dcc->timeout = GG_DEFAULT_TIMEOUT;
			dcc->incoming = 0;

			return e;
		}

		case GG_STATE_READING_ID:
		{
			gg_dcc7_id_t id;
			int res;

			gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() GG_STATE_READING_ID\n");

			if ((res = read(dcc->fd, &id, sizeof(id))) != sizeof(id)) {
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() read() failed (%d, %s)\n", res, strerror(errno));
				e->type = GG_EVENT_DCC7_ERROR;
				e->event.dcc_error = GG_ERROR_DCC7_HANDSHAKE;
				return e;
			}

			if (memcmp(&id, &dcc->cid, sizeof(id))) {
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() invalid id\n");
				e->type = GG_EVENT_DCC7_ERROR;
				e->event.dcc_error = GG_ERROR_DCC7_HANDSHAKE;
				return e;
			}

			if (dcc->incoming) {
				dcc->state = GG_STATE_SENDING_ID;
				dcc->check = GG_CHECK_WRITE;
				dcc->timeout = GG_DEFAULT_TIMEOUT;
			} else {
				dcc->state = GG_STATE_SENDING_FILE;
				dcc->check = GG_CHECK_WRITE;
				dcc->timeout = GG_DEFAULT_TIMEOUT;
			}

			return e;
		}

		case GG_STATE_SENDING_ID:
		{
			int res;

			gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() GG_SENDING_ID\n");

			if ((res = write(dcc->fd, &dcc->cid, sizeof(dcc->cid))) != sizeof(dcc->cid)) {
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() write() failed (%d, %s)", res, strerror(errno));
				e->type = GG_EVENT_DCC7_ERROR;
				e->event.dcc_error = GG_ERROR_DCC7_HANDSHAKE;
				return e;
			}

			if (dcc->incoming) {
				dcc->state = GG_STATE_GETTING_FILE;
				dcc->check = GG_CHECK_READ;
				dcc->timeout = GG_DEFAULT_TIMEOUT;
			} else {
				dcc->state = GG_STATE_READING_ID;
				dcc->check = GG_CHECK_READ;
				dcc->timeout = GG_DEFAULT_TIMEOUT;
			}

			return e;
		}

		case GG_STATE_SENDING_FILE:
		{
			char buf[1024];
			int chunk, res;

//			gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() GG_STATE_SENDING_FILE (offset=%d, size=%d)\n", dcc->offset, dcc->size);

#ifdef GG_CONFIG_MIRANDA
			if (fseek(dcc->file_fd, dcc->offset, SEEK_SET) == (off_t) -1) {
#else
			if (lseek(dcc->file_fd, dcc->offset, SEEK_SET) == (off_t) -1) {
#endif
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() lseek() failed (%s)\n", strerror(errno));
				e->type = GG_EVENT_DCC7_ERROR;
				e->event.dcc_error = GG_ERROR_DCC7_FILE;
				return e;
			}

			if ((chunk = dcc->size - dcc->offset) > sizeof(buf))
				chunk = sizeof(buf);

#ifdef GG_CONFIG_MIRANDA
			if ((res = fread(buf, 1, chunk, dcc->file_fd)) < 1) {
#else
			if ((res = read(dcc->file_fd, buf, chunk)) < 1) {
#endif
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() read() failed (res=%d, %s)\n", res, strerror(errno));
				e->type = GG_EVENT_DCC7_ERROR;
				e->event.dcc_error = (res == -1) ? GG_ERROR_DCC7_FILE : GG_ERROR_DCC7_EOF;
				return e;
			}

			if ((res = write(dcc->fd, buf, res)) == -1) {
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() write() failed (%s)\n", strerror(errno));
				e->type = GG_EVENT_DCC7_ERROR;
				e->event.dcc_error = GG_ERROR_DCC7_NET;
				return e;
			}

			dcc->offset += res;

			if (dcc->offset >= dcc->size) {
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() finished\n");
				e->type = GG_EVENT_DCC7_DONE;
				return e;
			}

			dcc->state = GG_STATE_SENDING_FILE;
			dcc->check = GG_CHECK_WRITE;
			dcc->timeout = GG_DCC7_TIMEOUT_SEND;

			return e;
		}

		case GG_STATE_GETTING_FILE:
		{
			char buf[1024];
			int res, wres;

//			gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() GG_STATE_GETTING_FILE (offset=%d, size=%d)\n", dcc->offset, dcc->size);

			if ((res = read(dcc->fd, buf, sizeof(buf))) < 1) {
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() read() failed (fd=%d, res=%d, %s)\n", dcc->fd, res, strerror(errno));
				e->type = GG_EVENT_DCC7_ERROR;
				e->event.dcc_error = (res == -1) ? GG_ERROR_DCC7_NET : GG_ERROR_DCC7_EOF;
				return e;
			}

			// XXX zapisywa� do skutku?

#ifdef GG_CONFIG_MIRANDA
			if ((wres = fwrite(buf, 1, res, dcc->file_fd)) < res) {
#else
			if ((wres = write(dcc->file_fd, buf, res)) < res) {
#endif
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() write() failed (fd=%d, res=%d, %s)\n", dcc->file_fd, wres, strerror(errno));
				e->type = GG_EVENT_DCC7_ERROR;
				e->event.dcc_error = GG_ERROR_DCC7_FILE;
				return e;
			}

			dcc->offset += res;

			if (dcc->offset >= dcc->size) {
				gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() finished\n");
				e->type = GG_EVENT_DCC7_DONE;
				return e;
			}

			dcc->state = GG_STATE_GETTING_FILE;
			dcc->check = GG_CHECK_READ;
			dcc->timeout = GG_DCC7_TIMEOUT_GET;

			return e;
		}

		default:
		{
			gg_debug(GG_DEBUG_MISC, "// gg_dcc7_watch_fd() GG_STATE_???\n");
			e->type = GG_EVENT_DCC7_ERROR;
			e->event.dcc_error = GG_ERROR_DCC7_HANDSHAKE;

			return e;
		}
	}

	return e;
}

void gg_dcc7_free(struct gg_dcc7 *dcc)
{
	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc7_free(%p);\n", dcc);

	if (!dcc)
		return;

	if (dcc->fd != -1)
		close(dcc->fd);

#ifdef GG_CONFIG_MIRANDA
	if (dcc->file_fd != NULL)
		fclose(dcc->file_fd);
#else
	if (dcc->file_fd != -1)
		close(dcc->file_fd);
#endif

	if (dcc->sess)
		gg_dcc7_session_remove(dcc->sess, dcc);

	free(dcc);
}

