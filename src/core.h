/*
 * gaim
 *
 * Copyright (C) 1998-1999, Mark Spencer <markster@marko.net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _CORE_H_
#define _CORE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <time.h>
#include <glib.h>
#ifdef GAIM_PLUGINS
#include <gmodule.h>
#endif

#include "multi.h"

enum gaim_event {
	event_signon = 0,
	event_signoff,
	event_away,
	event_back,
	event_im_recv,
	event_im_send,
	event_buddy_signon,
	event_buddy_signoff,
	event_buddy_away,
	event_buddy_back,
	event_buddy_idle,
	event_buddy_unidle,
	event_blist_update,
	event_chat_invited,
	event_chat_join,
	event_chat_leave,
	event_chat_buddy_join,
	event_chat_buddy_leave,
	event_chat_recv,
	event_chat_send,
	event_warned,
	event_error,
	event_quit,
	event_new_conversation,
	event_set_info,
	event_draw_menu,
	event_im_displayed_sent,
	event_im_displayed_rcvd,
	event_chat_send_invite,
	/* any others? it's easy to add... */
};

#ifdef GAIM_PLUGINS

struct gaim_plugin {
	GModule *handle;
	char *name;
	char *description;
};

struct gaim_callback {
	GModule *handle;
	enum gaim_event event;
	void *function;
	void *data;
};

extern GList *plugins;
extern GList *callbacks;
#endif

struct buddy {
	int edittype; /* CUI: this is really a GUI function and we need to put this in ui.h */
	char name[80];
	char show[80];
        int present;
	int evil;
	time_t signon;
	time_t idle;
        int uc;
	gushort caps; /* woohoo! */
	void *proto_data; /* what a hack */
	struct gaim_connection *gc; /* the connection it belongs to */
};

struct group {
	int edittype; /* CUI: this is really a GUI function and we need to put this in ui.h */
	char name[80];
	GSList *members;
	struct gaim_connection *gc; /* the connection it belongs to */
};

/* Functions in buddy.c */
extern struct buddy *find_buddy(struct gaim_connection *, char *);
extern struct group *find_group(struct gaim_connection *, char *);
extern struct group *find_group_by_buddy(struct gaim_connection *, char *);
extern struct buddy *add_buddy(struct gaim_connection *, char *, char *, char *);
extern void remove_buddy(struct gaim_connection *, struct group *, struct buddy *);
extern struct group *add_group(struct gaim_connection *, char *);
extern void remove_group(struct gaim_connection *, struct group *);
extern void do_export(struct gaim_connection *);
extern void do_import(struct gaim_connection *, char *);
extern int bud_list_cache_exists(struct gaim_connection *);

/* Functions in server.c */
extern void serv_got_update(struct gaim_connection *, char *, int, int, time_t, time_t, int, gushort);
extern void serv_got_im(struct gaim_connection *, char *, char *, guint32, time_t);
extern void serv_got_eviled(struct gaim_connection *, char *, int);
extern void serv_got_chat_invite(struct gaim_connection *, char *, char *, char *, GList *);
extern struct conversation *serv_got_joined_chat(struct gaim_connection *, int, char *);
extern void serv_got_chat_left(struct gaim_connection *, int);
extern void serv_got_chat_in(struct gaim_connection *, int, char *, int, char *, time_t);
extern void serv_finish_login();

#endif /* _CORE_H_ */
