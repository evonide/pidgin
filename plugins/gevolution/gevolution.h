/*
 * Evolution integration plugin for Gaim
 *
 * Copyright (C) 2003 Christian Hammond.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifndef _GEVOLUTION_H_
#define _GEVOLUTION_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define EBOOK_CHECK_VERSION(major, minor, micro) \
	(LIBEBOOK_MAJOR_VER > (major) || \
	 (LIBEBOOK_MAJOR_VER == (major) && LIBEBOOK_MINOR_VER > (minor)) || \
	 (LIBEBOOK_MAJOR_VER == (major) && LIBEBOOK_MINOR_VER == (minor) && \
	  LIBEBOOK_MICRO_VER >= (micro)))

#include <libebook/e-book.h>

#if !EBOOK_CHECK_VERSION(0, 0, 95)
# include <libebook/e-book-async.h>
#endif

typedef struct
{
	GaimAccount *account;
	char *username;

	EBook *book;

	GtkWidget *win;
	GtkWidget *treeview;
	GtkWidget *addressbooks_menu;
	GtkWidget *search_field;
	GtkWidget *group_combo;
	GtkWidget *select_button;
	GtkWidget *account_optmenu;
	GtkListStore *model;

	GList *contacts;

} GevoAddBuddyDialog;

typedef struct
{
	gboolean person_only;

	GaimAccount *account;
	GaimBuddy *buddy;

	EContact *contact;

	GtkWidget *win;
	GtkWidget *accounts_menu;
	GtkWidget *screenname;
	GtkWidget *firstname;
	GtkWidget *lastname;
	GtkWidget *email;
	GtkWidget *group_combo;
	GtkWidget *add_button;

	char *buddy_icon;

} GevoNewPersonDialog;

typedef struct
{
	GaimBuddy *buddy;

	EBook *book;

	GtkWidget *win;
	GtkWidget *treeview;
	GtkWidget *addressbooks_menu;
	GtkWidget *search_field;
	GtkWidget *assoc_button;
	GtkWidget *imhtml;
	GtkListStore *model;

	GList *contacts;

} GevoAssociateBuddyDialog;

void gevo_add_buddy_dialog_show(GaimAccount *account, const char *username,
								const char *group, const char *alias);
void gevo_add_buddy_dialog_add_person(GevoAddBuddyDialog *dialog,
									  EContact *contact,
									  const char *name, GaimAccount *account,
									  const char *screenname);

void gevo_new_person_dialog_show(EContact *contact, GaimAccount *account,
								 const char *username, const char *group,
								 GaimBuddy *buddy, gboolean person_only);

void gevo_add_buddy(GaimAccount *account, const char *group_name,
					const char *screenname, const char *alias);
GList *gevo_get_groups(void);

EContactField gevo_prpl_get_field(GaimAccount *account, GaimBuddy *buddy);
gboolean gevo_prpl_is_supported(GaimAccount *account, GaimBuddy *buddy);
gboolean gevo_load_addressbook(EBook **book, GError **error);

GevoAssociateBuddyDialog *gevo_associate_buddy_dialog_new(GaimBuddy *buddy);

#endif /* _GEVOLUTION_H_ */
