/**
 * @file gtkpounce.c GTK+ Buddy Pounce API
 * @ingroup gtkui
 *
 * gaim
 *
 * Gaim is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
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
#include "gtkinternal.h"

#include "account.h"
#include "conversation.h"
#include "debug.h"
#include "notify.h"
#include "prpl.h"
#include "server.h"
#include "sound.h"

#include "gtkblist.h"
#include "gtkpounce.h"
#include "gtkutils.h"

#include "ui.h"

typedef struct
{
	/* Pounce data */
	GaimPounce  *pounce;
	GaimAccount *account;

	/* The window */
	GtkWidget *window;

	/* Pounce Who */
	GtkWidget *account_menu;
	GtkWidget *buddy_entry;

	/* Pounce When */
	GtkWidget *signon;
	GtkWidget *signoff;
	GtkWidget *away;
	GtkWidget *away_return;
	GtkWidget *idle;
	GtkWidget *idle_return;
	GtkWidget *typing;
	GtkWidget *stop_typing;

	/* Pounce Action */
	GtkWidget *open_win;
	GtkWidget *popup;
	GtkWidget *send_msg;
	GtkWidget *send_msg_entry;
	GtkWidget *exec_cmd;
	GtkWidget *exec_cmd_entry;
	GtkWidget *exec_cmd_browse;
	GtkWidget *play_sound;
	GtkWidget *play_sound_entry;
	GtkWidget *play_sound_browse;
	GtkWidget *play_sound_test;

	GtkWidget *save_pounce;

	/* Buttons */
	GtkWidget *save_button;

} GaimGtkPounceDialog;

/**************************************************************************
 * Callbacks
 **************************************************************************/
static gint
delete_win_cb(GtkWidget *w, GdkEventAny *e, GaimGtkPounceDialog *dialog)
{
	gtk_widget_destroy(dialog->window);
	g_free(dialog);

	return TRUE;
}

static void
delete_cb(GtkWidget *w, GaimGtkPounceDialog *dialog)
{
	gaim_pounce_destroy(dialog->pounce);

	delete_win_cb(NULL, NULL, dialog);
}

static void
cancel_cb(GtkWidget *w, GaimGtkPounceDialog *dialog)
{
	delete_win_cb(NULL, NULL, dialog);
}

static void
pounce_update_entryfields(GtkWidget *w, gpointer data)
{
	const char *filename;
	GHashTable *args;
	GtkFileSelection *filesel;

	args = (GHashTable *)data;
	filesel = GTK_FILE_SELECTION(g_hash_table_lookup(args, "filesel"));

	filename = gtk_file_selection_get_filename(filesel);

	if (gaim_gtk_check_if_dir(filename, filesel))
		return;

	if (filename != NULL)
		gtk_entry_set_text(GTK_ENTRY(g_hash_table_lookup(args, "entry")),
						   filename);

	gtk_widget_destroy(GTK_WIDGET(filesel));
	g_hash_table_destroy(args);
}

static void
filesel(GtkWidget *w, gpointer data)
{
	GtkWidget *filesel;
	GtkWidget *entry;
	GHashTable *args;

	entry = (GtkWidget *)data;

	filesel = gtk_file_selection_new(_("Select a file"));
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel),
									gtk_entry_get_text(GTK_ENTRY(entry)));
	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(filesel));
	gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(filesel), FALSE);

	args = g_hash_table_new(g_str_hash, g_str_equal);
	g_hash_table_insert(args, "filesel", filesel);
	g_hash_table_insert(args, "entry",   entry);

	g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
					 "clicked",
					 G_CALLBACK(pounce_update_entryfields), args);
	g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
							 "clicked",
							 G_CALLBACK(g_hash_table_destroy), args);
	g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
							 "clicked",
							 G_CALLBACK(gtk_widget_destroy), filesel);

	gtk_widget_show(filesel);
}

static void
pounce_test_sound(GtkWidget *w, GtkWidget *entry)
{
	const char *filename;

	filename = gtk_entry_get_text(GTK_ENTRY(entry));

	if (filename != NULL && *filename != '\0')
		gaim_sound_play_file((char *) filename);
	else
		gaim_sound_play_event(GAIM_SOUND_POUNCE_DEFAULT);
}

static void
save_pounce_cb(GtkWidget *w, GaimGtkPounceDialog *dialog)
{
	const char *name;
	const char *message, *command, *sound;
	GaimBuddyList *blist;
	GaimGtkBuddyList *gtkblist;
	GaimPounceEvent events = GAIM_POUNCE_NONE;

	name = gtk_entry_get_text(GTK_ENTRY(dialog->buddy_entry));

	if (*name == '\0') {
		gaim_notify_error(NULL, NULL,
						  _("Please enter a buddy to pounce."), NULL);
		return;
	}

	/* Events */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->signon)))
		events |= GAIM_POUNCE_SIGNON;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->signoff)))
		events |= GAIM_POUNCE_SIGNOFF;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->away)))
		events |= GAIM_POUNCE_AWAY;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->away_return)))
		events |= GAIM_POUNCE_AWAY_RETURN;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->idle)))
		events |= GAIM_POUNCE_IDLE;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->idle_return)))
		events |= GAIM_POUNCE_IDLE_RETURN;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->typing)))
		events |= GAIM_POUNCE_TYPING;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->stop_typing)))
		events |= GAIM_POUNCE_TYPING_STOPPED;

	/* Data fields */
	message = gtk_entry_get_text(GTK_ENTRY(dialog->send_msg_entry));
	command = gtk_entry_get_text(GTK_ENTRY(dialog->exec_cmd_entry));
	sound   = gtk_entry_get_text(GTK_ENTRY(dialog->play_sound_entry));

	if (*message == '\0') message = NULL;
	if (*command == '\0') command = NULL;
	if (*sound   == '\0') sound   = NULL;

	if (dialog->pounce == NULL) {
		dialog->pounce = gaim_pounce_new(GAIM_GTK_UI, dialog->account,
										 name, events);
	}
	else {
		gaim_pounce_set_events(dialog->pounce, events);
		gaim_pounce_set_pouncer(dialog->pounce, dialog->account);
		gaim_pounce_set_pouncee(dialog->pounce, name);
	}

	/* Actions*/
	gaim_pounce_action_set_enabled(dialog->pounce, "open-window",
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->open_win)));
	gaim_pounce_action_set_enabled(dialog->pounce, "popup-notify",
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->popup)));
	gaim_pounce_action_set_enabled(dialog->pounce, "send-message",
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->send_msg)));
	gaim_pounce_action_set_enabled(dialog->pounce, "execute-command",
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->exec_cmd)));
	gaim_pounce_action_set_enabled(dialog->pounce, "play-sound",
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->play_sound)));

	gaim_pounce_action_set_attribute(dialog->pounce, "send-message",
									 "message", message);
	gaim_pounce_action_set_attribute(dialog->pounce, "execute-command",
									 "command", command);
	gaim_pounce_action_set_attribute(dialog->pounce, "play-sound",
									 "filename", sound);

	gaim_pounce_set_save(dialog->pounce,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->save_pounce)));

	delete_win_cb(NULL, NULL, dialog);

	gaim_pounces_sync();

	/* Rebuild the pounce menu */
	blist = gaim_get_blist();

	if (GAIM_IS_GTK_BLIST(blist))
	{
		gtkblist = GAIM_GTK_BLIST(blist);

		gaim_gtkpounce_menu_build(gtkblist->bpmenu);
	}
}

static void
pounce_choose_cb(GtkWidget *item, GaimAccount *account,
				 GaimGtkPounceDialog *dialog)
{
	dialog->account = account;
}

static void
buddy_changed_cb(GtkEntry *entry, GaimGtkPounceDialog *dialog)
{
	if (dialog->save_button == NULL)
		return;

	gtk_widget_set_sensitive(dialog->save_button,
			*gtk_entry_get_text(entry) != '\0');
}

void
gaim_gtkpounce_dialog_show(GaimAccount *account, const char *name,
						   GaimPounce *cur_pounce)
{
	GaimGtkPounceDialog *dialog;
	GtkWidget *window;
	GtkWidget *label;
	GtkWidget *bbox;
	GtkWidget *vbox1, *vbox2;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *sep;
	GtkSizeGroup *sg;
	GPtrArray *sound_widgets;
	GPtrArray *exec_widgets;

	dialog = g_new0(GaimGtkPounceDialog, 1);

	if (cur_pounce != NULL) {
		dialog->pounce  = cur_pounce;
		dialog->account = gaim_pounce_get_pouncer(cur_pounce);
	}
	else if (account != NULL) {
		dialog->pounce  = NULL;
		dialog->account = account;
	}
	else {
		dialog->pounce  = NULL;
		dialog->account = gaim_connection_get_account((GaimConnection *)gaim_connections_get_all()->data);
	}

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	/* Create the window. */
	dialog->window = window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_role(GTK_WINDOW(window), "buddy_pounce");
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_window_set_title(GTK_WINDOW(window),
						 (cur_pounce == NULL
						  ? _("New Buddy Pounce") : _("Edit Buddy Pounce")));

	gtk_container_set_border_width(GTK_CONTAINER(window), 12);
	gtk_widget_realize(window);

	g_signal_connect(G_OBJECT(window), "delete_event",
					 G_CALLBACK(delete_win_cb), dialog);

	/* Create the parent vbox for everything. */
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_add(GTK_CONTAINER(window), vbox1);
	gtk_widget_show(vbox1);

	/* Create the vbox that will contain all the prefs stuff. */
	vbox2 = gtk_vbox_new(FALSE, 18);
	gtk_box_pack_start(GTK_BOX(vbox1), vbox2, TRUE, TRUE, 0);

	/* Create the "Pounce Who" frame. */
	frame = gaim_gtk_make_frame(vbox2, _("Pounce Who"));

	/* Account: */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new_with_mnemonic(_("_Account:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	gtk_size_group_add_widget(sg, label);

	dialog->account_menu =
		gaim_gtk_account_option_menu_new(dialog->account, FALSE,
										 G_CALLBACK(pounce_choose_cb),
										 NULL, dialog);

	gtk_box_pack_start(GTK_BOX(hbox), dialog->account_menu, FALSE, FALSE, 0);
	gtk_widget_show(dialog->account_menu);
	gaim_set_accessible_label (dialog->account_menu, label);

	/* Buddy: */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new_with_mnemonic(_("_Buddy Name:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	gtk_size_group_add_widget(sg, label);

	dialog->buddy_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), dialog->buddy_entry, TRUE, TRUE, 0);
	gtk_widget_show(dialog->buddy_entry);

	g_signal_connect(G_OBJECT(dialog->buddy_entry), "changed",
			 G_CALLBACK(buddy_changed_cb), dialog);
	gaim_set_accessible_label (dialog->buddy_entry, label);

	if (cur_pounce != NULL) {
		gtk_entry_set_text(GTK_ENTRY(dialog->buddy_entry),
						   gaim_pounce_get_pouncee(cur_pounce));
	}
	else if (name != NULL) {
		gtk_entry_set_text(GTK_ENTRY(dialog->buddy_entry), name);
	}

	/* Create the "Pounce When" frame. */
	frame = gaim_gtk_make_frame(vbox2, _("Pounce When"));

	table = gtk_table_new(2, 4, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_widget_show(table);

	dialog->signon =
		gtk_check_button_new_with_mnemonic(_("_Sign on"));
	dialog->signoff =
		gtk_check_button_new_with_mnemonic(_("Sign _off"));
	dialog->away =
		gtk_check_button_new_with_mnemonic(_("A_way"));
	dialog->away_return =
		gtk_check_button_new_with_mnemonic(_("Re_turn from away"));
	dialog->idle =
		gtk_check_button_new_with_mnemonic(_("_Idle"));
	dialog->idle_return =
		gtk_check_button_new_with_mnemonic(_("Retur_n from idle"));
	dialog->typing =
		gtk_check_button_new_with_mnemonic(_("Buddy starts _typing"));
	dialog->stop_typing =
		gtk_check_button_new_with_mnemonic(_("Buddy stops t_yping"));

	gtk_table_attach(GTK_TABLE(table), dialog->signon,      0, 1, 0, 1,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->signoff,     1, 2, 0, 1,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->away,        0, 1, 1, 2,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->away_return, 1, 2, 1, 2,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->idle,        0, 1, 2, 3,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->idle_return, 1, 2, 2, 3,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->typing,      0, 1, 3, 4,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->stop_typing, 1, 2, 3, 5,
					 GTK_FILL, 0, 0, 0);

	gtk_widget_show(dialog->signon);
	gtk_widget_show(dialog->signoff);
	gtk_widget_show(dialog->away);
	gtk_widget_show(dialog->away_return);
	gtk_widget_show(dialog->idle);
	gtk_widget_show(dialog->idle_return);
	gtk_widget_show(dialog->typing);
	gtk_widget_show(dialog->stop_typing);

	/* Create the "Pounce Action" frame. */
	frame = gaim_gtk_make_frame(vbox2, _("Pounce Action"));

	table = gtk_table_new(3, 5, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_widget_show(table);

	dialog->open_win = gtk_check_button_new_with_mnemonic(_("Op_en an IM window"));
	dialog->popup = gtk_check_button_new_with_mnemonic(_("_Popup notification"));
	dialog->send_msg = gtk_check_button_new_with_mnemonic(_("Send a _message"));
	dialog->exec_cmd = gtk_check_button_new_with_mnemonic(_("E_xecute a command"));
	dialog->play_sound = gtk_check_button_new_with_mnemonic(_("P_lay a sound"));

	dialog->send_msg_entry   = gtk_entry_new();
	dialog->exec_cmd_entry   = gtk_entry_new();
	dialog->exec_cmd_browse = gtk_button_new_with_mnemonic(_("B_rowse..."));
	dialog->play_sound_entry = gtk_entry_new();
	dialog->play_sound_browse = gtk_button_new_with_mnemonic(_("Bro_wse..."));
	dialog->play_sound_test = gtk_button_new_with_mnemonic(_("Pre_view"));

	gtk_widget_set_sensitive(dialog->send_msg_entry,   FALSE);
	gtk_widget_set_sensitive(dialog->exec_cmd_entry,   FALSE);
	gtk_widget_set_sensitive(dialog->exec_cmd_browse, FALSE);
	gtk_widget_set_sensitive(dialog->play_sound_entry, FALSE);
	gtk_widget_set_sensitive(dialog->play_sound_browse,   FALSE);
	gtk_widget_set_sensitive(dialog->play_sound_test, FALSE);

	gtk_table_attach(GTK_TABLE(table), dialog->open_win,         0, 1, 0, 1,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->popup,            0, 1, 1, 2,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->send_msg,         0, 1, 2, 3,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->send_msg_entry,   1, 4, 2, 3,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->exec_cmd,         0, 1, 3, 4,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->exec_cmd_entry,   1, 2, 3, 4,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->exec_cmd_browse,   2, 3, 3, 4,
					 GTK_FILL | GTK_EXPAND, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->play_sound,       0, 1, 4, 5,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->play_sound_entry, 1, 2, 4, 5,
					 GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->play_sound_browse, 2, 3, 4, 5,
					 GTK_FILL | GTK_EXPAND, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), dialog->play_sound_test, 3, 4, 4, 5,
					 GTK_FILL | GTK_EXPAND, 0, 0, 0);

	gtk_widget_show(dialog->open_win);
	gtk_widget_show(dialog->popup);
	gtk_widget_show(dialog->send_msg);
	gtk_widget_show(dialog->send_msg_entry);
	gtk_widget_show(dialog->exec_cmd);
	gtk_widget_show(dialog->exec_cmd_entry);
	gtk_widget_show(dialog->exec_cmd_browse);
	gtk_widget_show(dialog->play_sound);
	gtk_widget_show(dialog->play_sound_entry);
	gtk_widget_show(dialog->play_sound_browse);
	gtk_widget_show(dialog->play_sound_test);

	g_signal_connect(G_OBJECT(dialog->send_msg), "clicked",
					 G_CALLBACK(gaim_gtk_toggle_sensitive),
					 dialog->send_msg_entry);

	exec_widgets = g_ptr_array_new();
	g_ptr_array_add(exec_widgets,dialog->exec_cmd_entry);
	g_ptr_array_add(exec_widgets,dialog->exec_cmd_browse);

	g_signal_connect(G_OBJECT(dialog->exec_cmd), "clicked",
					 G_CALLBACK(gtk_toggle_sensitive_array),
					 exec_widgets);
	g_signal_connect(G_OBJECT(dialog->exec_cmd_browse), "clicked",
					 G_CALLBACK(filesel),
					 dialog->exec_cmd_entry);

	sound_widgets = g_ptr_array_new();
	g_ptr_array_add(sound_widgets,dialog->play_sound_entry);
	g_ptr_array_add(sound_widgets,dialog->play_sound_browse);
	g_ptr_array_add(sound_widgets,dialog->play_sound_test);

	g_signal_connect(G_OBJECT(dialog->play_sound), "clicked",
					 G_CALLBACK(gtk_toggle_sensitive_array),
					 sound_widgets);
	g_signal_connect(G_OBJECT(dialog->play_sound_browse), "clicked",
					 G_CALLBACK(filesel),
					 dialog->play_sound_entry);
	g_signal_connect(G_OBJECT(dialog->play_sound_test), "clicked",
					 G_CALLBACK(pounce_test_sound),
					 dialog->play_sound_entry);

	g_signal_connect(G_OBJECT(dialog->send_msg_entry), "activate",
					 G_CALLBACK(save_pounce_cb), dialog);
	g_signal_connect(G_OBJECT(dialog->exec_cmd_entry), "activate",
					 G_CALLBACK(save_pounce_cb), dialog);
	g_signal_connect(G_OBJECT(dialog->play_sound_entry), "activate",
					 G_CALLBACK(save_pounce_cb), dialog);

	/* Now the last part, where we have the Save checkbox */
	dialog->save_pounce = gtk_check_button_new_with_mnemonic(
		_("Sav_e this pounce after activation"));

	gtk_box_pack_start(GTK_BOX(vbox2), dialog->save_pounce, FALSE, FALSE, 0);

	/* Separator... */
	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox1), sep, FALSE, FALSE, 0);
	gtk_widget_show(sep);

	/* Now the button box! */
	bbox = gtk_hbutton_box_new();
	gtk_box_set_spacing(GTK_BOX(bbox), 6);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_box_pack_end(GTK_BOX(vbox1), bbox, FALSE, FALSE, 0);
	gtk_widget_show(bbox);

	/* Delete button */
	button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(delete_cb), dialog);

	/* Cancel button */
	button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(cancel_cb), dialog);

	/* Save button */
	dialog->save_button = button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(save_pounce_cb), dialog);

	if (*gtk_entry_get_text(GTK_ENTRY(dialog->buddy_entry)) == '\0')
		gtk_widget_set_sensitive(button, FALSE);

	/* Set the values of stuff. */
	if (cur_pounce != NULL) {
		GaimPounceEvent events = gaim_pounce_get_events(cur_pounce);
		const char *value;

		/* Events */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->signon),
									(events & GAIM_POUNCE_SIGNON));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->signoff),
									(events & GAIM_POUNCE_SIGNOFF));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->away),
									(events & GAIM_POUNCE_AWAY));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->away_return),
									(events & GAIM_POUNCE_AWAY_RETURN));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->idle),
									(events & GAIM_POUNCE_IDLE));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->idle_return),
									(events & GAIM_POUNCE_IDLE_RETURN));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->typing),
									(events & GAIM_POUNCE_TYPING));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->stop_typing),
									(events & GAIM_POUNCE_TYPING_STOPPED));

		/* Actions */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->open_win),
			gaim_pounce_action_is_enabled(cur_pounce, "open-window"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->popup),
			gaim_pounce_action_is_enabled(cur_pounce, "popup-notify"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->send_msg),
			gaim_pounce_action_is_enabled(cur_pounce, "send-message"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->exec_cmd),
			gaim_pounce_action_is_enabled(cur_pounce, "execute-command"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->play_sound),
			gaim_pounce_action_is_enabled(cur_pounce, "play-sound"));

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->save_pounce),
			gaim_pounce_get_save(cur_pounce));

		if ((value = gaim_pounce_action_get_attribute(cur_pounce,
													  "send-message",
													  "message")) != NULL) {

			gtk_entry_set_text(GTK_ENTRY(dialog->send_msg_entry), value);
		}

		if ((value = gaim_pounce_action_get_attribute(cur_pounce,
													  "execute-command",
													  "command")) != NULL) {

			gtk_entry_set_text(GTK_ENTRY(dialog->exec_cmd_entry), value);
		}

		if ((value = gaim_pounce_action_get_attribute(cur_pounce,
													  "play-sound",
													  "filename")) != NULL) {
			gtk_entry_set_text(GTK_ENTRY(dialog->play_sound_entry), value);
		}
	}
	else {
		/* Set some defaults */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->send_msg), TRUE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->signon), TRUE);
	}

	gtk_widget_show_all(vbox2);
	gtk_widget_show(window);
}

static void
new_pounce_cb(GtkWidget *w, GaimBuddy *b)
{
	if (b == NULL)
		gaim_gtkpounce_dialog_show(NULL, NULL, NULL);
	else
		gaim_gtkpounce_dialog_show(b->account, b->name, NULL);
}

static void
delete_pounce_cb(GtkWidget *w, GaimPounce *pounce)
{
	gaim_pounce_destroy(pounce);
}

static void
edit_pounce_cb(GtkWidget *w, GaimPounce *pounce)
{
	gaim_gtkpounce_dialog_show(NULL, NULL, pounce);
}

static void
fill_menu(GtkWidget *menu, GCallback cb)
{
	GtkWidget *image;
	GtkWidget *item;
	GdkPixbuf *pixbuf, *scale;
	GaimPounce *pounce;
	const char *buddy;
	GList *bp;

	for (bp = gaim_pounces_get_all(); bp != NULL; bp = bp->next) {
		pounce = (GaimPounce *)bp->data;
		buddy = gaim_pounce_get_pouncee(pounce);

		/* Check if account is online, if not skip it */
		if(!gaim_account_is_connected(pounce->pouncer))
			continue;
		
		/* Build the menu item */
		item = gtk_image_menu_item_new_with_label(buddy);

		/* Create a pixmap for the protocol icon. */
		pixbuf = create_prpl_icon(gaim_pounce_get_pouncer(pounce));
		if(pixbuf) {
			scale = gdk_pixbuf_scale_simple(pixbuf, 16, 16,
					GDK_INTERP_BILINEAR);

			/* Now convert it to GtkImage */
			image = gtk_image_new_from_pixbuf(scale);
			g_object_unref(G_OBJECT(scale));
			g_object_unref(G_OBJECT(pixbuf));
			gtk_widget_show(image);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
		}

		/* Put the item in the menu */
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		gtk_widget_show(item);

		/* Set our callbacks. */
		g_signal_connect(G_OBJECT(item), "activate", cb, pounce);
	}
}

void
gaim_gtkpounce_menu_build(GtkWidget *menu)
{
	GtkWidget *remmenu;
	GtkWidget *item;
	GList *l;

	if(!menu)
		return;

	for (l = gtk_container_get_children(GTK_CONTAINER(menu));
		 l != NULL;
		 l = l->next) {

		gtk_widget_destroy(GTK_WIDGET(l->data));
	}
	g_list_free(l);

	/* "New Buddy Pounce" */
	item = gtk_menu_item_new_with_label(_("New Buddy Pounce"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
	g_signal_connect(G_OBJECT(item), "activate",
					 G_CALLBACK(new_pounce_cb), NULL);

	/* "Remove Buddy Pounce" */
	item = gtk_menu_item_new_with_label(_("Remove Buddy Pounce"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* "Remove Buddy Pounce" menu */
	remmenu = gtk_menu_new();

	fill_menu(remmenu, G_CALLBACK(delete_pounce_cb));

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), remmenu);
	gtk_widget_show(remmenu);
	gtk_widget_show(item);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);

	fill_menu(menu, G_CALLBACK(edit_pounce_cb));
}

static void
pounce_cb(GaimPounce *pounce, GaimPounceEvent events, void *data)
{
	GaimConversation *conv;
	GaimAccount *account;
	GaimBuddy *buddy;
	const char *pouncee;
	const char *alias;

	pouncee = gaim_pounce_get_pouncee(pounce);
	account = gaim_pounce_get_pouncer(pounce);

	buddy = gaim_find_buddy(account, pouncee);

	alias = gaim_get_buddy_alias(buddy);

	if (gaim_pounce_action_is_enabled(pounce, "open-window")) {
		conv = gaim_find_conversation_with_account(pouncee, account);

		if (conv == NULL)
			conv = gaim_conversation_new(GAIM_CONV_IM, account, pouncee);
	}

	if (gaim_pounce_action_is_enabled(pounce, "popup-notify")) {
		char tmp[1024];

		g_snprintf(tmp, sizeof(tmp),
				   (events & GAIM_POUNCE_TYPING) ? _("%s has started typing to you") :
				   (events & GAIM_POUNCE_SIGNON) ? _("%s has signed on") :
				   (events & GAIM_POUNCE_IDLE_RETURN) ? _("%s has returned from being idle") :
				   (events & GAIM_POUNCE_AWAY_RETURN) ? _("%s has returned from being away") :
				   (events & GAIM_POUNCE_TYPING_STOPPED) ? _("%s has stopped typing to you") :
				   (events & GAIM_POUNCE_SIGNOFF) ? _("%s has signed off") :
				   (events & GAIM_POUNCE_IDLE) ? _("%s has become idle") :
				   (events & GAIM_POUNCE_AWAY) ? _("%s has gone away.") :
				   _("Unknown pounce event. Please report this!"),
				   alias);

		gaim_notify_info(NULL, NULL, tmp, NULL);
	}

	if (gaim_pounce_action_is_enabled(pounce, "send-message")) {
		const char *message;

		message = gaim_pounce_action_get_attribute(pounce, "send-message",
												   "message");

		if (message != NULL) {
			conv = gaim_find_conversation_with_account(pouncee, account);

			if (conv == NULL)
				conv = gaim_conversation_new(GAIM_CONV_IM, account, pouncee);

			gaim_conversation_write(conv, NULL, message,
									GAIM_MESSAGE_SEND, time(NULL));

			serv_send_im(account->gc, (char *)pouncee, (char *)message, 0);
		}
	}

	if (gaim_pounce_action_is_enabled(pounce, "execute-command")) {
		const char *command;

		command = gaim_pounce_action_get_attribute(pounce, "execute-command",
												   "command");

		if (command != NULL) {
#ifndef _WIN32
			int pid = fork();

			if (pid == 0) {
				char *args[4];

				args[0] = "sh";
				args[1] = "-c";
				args[2] = (char *)command;
				args[3] = NULL;

				execvp(args[0], args);

				_exit(0);
			}
#else
			STARTUPINFO StartInfo;
			PROCESS_INFORMATION ProcInfo;

			memset(&ProcInfo, 0, sizeof(ProcInfo));
			memset(&StartInfo, 0 , sizeof(StartInfo));
			StartInfo.cb = sizeof(StartInfo);
			CreateProcess(NULL, (char *)command, NULL, NULL, 0, 0, NULL, NULL, &StartInfo, &ProcInfo);
			gaim_debug_info("pounce",
							"Pounce execute command called for: %s\n",
							command);
#endif
		}
	}

	if (gaim_pounce_action_is_enabled(pounce, "play-sound")) {
		const char *sound;

		sound = gaim_pounce_action_get_attribute(pounce, "play-sound",
												 "filename");

		if (sound != NULL)
			gaim_sound_play_file(sound);
		else
			gaim_sound_play_event(GAIM_SOUND_POUNCE_DEFAULT);
	}
}

static void
free_pounce(GaimPounce *pounce)
{
	GaimBuddyList *blist;
	GaimGtkBuddyList *gtkblist;

	/* Rebuild the pounce menu */
	blist = gaim_get_blist();

	if (GAIM_IS_GTK_BLIST(blist))
	{
		gtkblist = GAIM_GTK_BLIST(blist);

		gaim_gtkpounce_menu_build(gtkblist->bpmenu);
	}
}

static void
new_pounce(GaimPounce *pounce)
{
	gaim_pounce_action_register(pounce, "open-window");
	gaim_pounce_action_register(pounce, "popup-notify");
	gaim_pounce_action_register(pounce, "send-message");
	gaim_pounce_action_register(pounce, "execute-command");
	gaim_pounce_action_register(pounce, "play-sound");
}

void
gaim_gtk_pounces_init(void)
{
	gaim_pounces_register_handler(GAIM_GTK_UI, pounce_cb, new_pounce,
								  free_pounce);
}
