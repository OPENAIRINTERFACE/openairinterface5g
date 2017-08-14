AM_CFLAGS =	\
	@ADD_CFLAGS@	\
	-I$(top_srcdir)/common	\
	-I$(top_srcdir)/libbuffers	\
	-I$(top_srcdir)/libparser	\
	-I$(top_srcdir)/libresolver

noinst_LTLIBRARIES = libui.la

libui_la_LDFLAGS = -all-static
libui_la_SOURCES = \
	ui_main_screen.c		ui_main_screen.h	\
	ui_menu_bar.c			ui_menu_bar.h	\
	ui_callbacks.c			ui_callbacks.h	\
	ui_tree_view.c			ui_tree_view.h	\
	ui_signal_dissect_view.c	ui_signal_dissect_view.h	\
	ui_notifications.c		ui_notifications.h	\
	ui_notif_dlg.c			ui_notif_dlg.h	\
	ui_interface.c			ui_interface.h	\
	ui_notebook.c			ui_notebook.h	\
	ui_filters.c			ui_filters.h