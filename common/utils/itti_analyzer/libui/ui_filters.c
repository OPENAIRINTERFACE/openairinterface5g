/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <stdio.h>
#include <stdlib.h>

#define G_LOG_DOMAIN ("UI_FILTER")

#include <glib.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "ui_callbacks.h"
#include "ui_main_screen.h"
#include "ui_filters.h"
#include "ui_tree_view.h"
#include "ui_notif_dlg.h"
#include "rc.h"

const uint32_t FILTER_ALLOC_NUMBER = 100;
const uint32_t FILTER_ID_UNDEFINED = ~0;

const char * const COLOR_WHITE = "#ffffff";
const char * const COLOR_DARK_GREY = "#585858";

#define ENABLED_NAME    "enabled"
#define FOREGROUND_NAME "foreground_color"
#define BACKGROUND_NAME "background_color"

ui_filters_t ui_filters;

static int ui_init_filter(ui_filter_t *filter, int reset, int clear_ids, char *name)
{
    if (filter->items == NULL)
    {
        filter->name = name;

        /* Allocate some filter entries */
        filter->items = malloc (FILTER_ALLOC_NUMBER * sizeof(ui_filter_item_t));
        filter->allocated = FILTER_ALLOC_NUMBER;
    }
    if (reset)
    {
        /* Reset number of used filter entries */
        filter->used = 0;
    }
    else
    {
        if (clear_ids)
        {
            /* Clear entries IDs */
            int item;

            for (item = 0; item < filter->used; item++)
            {
                filter->items[item].id = FILTER_ID_UNDEFINED;
            }
        }
    }

    return (RC_OK);
}

int ui_init_filters(int reset, int clear_ids)
{
    ui_init_filter (&ui_filters.messages, reset, clear_ids, "messages");
    ui_init_filter (&ui_filters.origin_tasks, reset, clear_ids, "origin_tasks");
    ui_init_filter (&ui_filters.destination_tasks, reset, clear_ids, "destination_tasks");
    ui_init_filter (&ui_filters.instances, reset, clear_ids, "instances");

    ui_destroy_filter_menus ();

    return (RC_OK);
}

gboolean ui_filters_enable(gboolean enabled)
{
    gboolean changed = ui_filters.filters_enabled != enabled;

    if (changed)
    {
        ui_filters.filters_enabled = enabled;
    }

    return changed;
}

static int ui_filters_search_name(ui_filter_t *filter, const char *name)
{
    int item;

    for (item = 0; item < filter->used; item++)
    {
        if (strncmp (name, filter->items[item].name, SIGNAL_NAME_LENGTH) == 0)
        {
            return (item);
        }
    }

    return (item);
}

int ui_filters_search_id(ui_filter_t *filter, uint32_t value)
{
    int item;

    for (item = 0; item < filter->used; item++)
    {
        if (filter->items[item].id == value)
        {
            return (item);
        }
    }

    return (-1);
}

static void ui_filter_set_enabled(uint8_t *enabled, ui_entry_enabled_e entry_enabled, gboolean new)
{
    if (entry_enabled == ENTRY_ENABLED_UNDEFINED)
    {
        if (new)
        {
            *enabled = TRUE;
        }
    }
    else
    {
        if (entry_enabled == ENTRY_ENABLED_FALSE)
        {
            *enabled = FALSE;
        }
        else
        {
            *enabled = TRUE;
        }
    }
}

static int ui_filter_add(ui_filter_t *filter, uint32_t value, const char *name, ui_entry_enabled_e entry_enabled,
                         const char *foreground, const char *background)
{
    int item = ui_filters_search_name (filter, name);

    if (item >= filter->allocated)
    {
        /* Increase number of filter entries */
        filter->items = realloc (filter->items, (filter->allocated + FILTER_ALLOC_NUMBER) * sizeof(ui_filter_item_t));
        filter->allocated += FILTER_ALLOC_NUMBER;
    }

    if (value != FILTER_ID_UNDEFINED)
    {
        filter->items[item].id = value;
    }
    if (item >= filter->used)
    {
        /* New entry */
        strncpy (filter->items[item].name, name, SIGNAL_NAME_LENGTH);
        ui_filter_set_enabled (&filter->items[item].enabled, entry_enabled, TRUE);
        strncpy (filter->items[item].foreground, foreground != NULL ? foreground : COLOR_DARK_GREY, COLOR_SIZE);
        strncpy (filter->items[item].background, background != NULL ? background : COLOR_WHITE, COLOR_SIZE);

        filter->used++;
    }
    else
    {
        ui_filter_set_enabled (&filter->items[item].enabled, entry_enabled, FALSE);
        if (foreground != NULL)
        {
            strncpy (filter->items[item].foreground, foreground, COLOR_SIZE);
        }
        if (background != NULL)
        {
            strncpy (filter->items[item].background, background, COLOR_SIZE);
        }
    }

    g_debug("filter \"%s\" add %d \"%s\" %d", filter->name, value, name, entry_enabled);

    return (item);
}

void ui_filters_add(ui_filter_e filter, uint32_t value, const char *name, ui_entry_enabled_e entry_enabled,
                    const char *foreground, const char *background)
{
    switch (filter)
    {
        case FILTER_MESSAGES:
            ui_filter_add (&ui_filters.messages, value, name, entry_enabled, foreground, background);
            break;

        case FILTER_ORIGIN_TASKS:
            ui_filter_add (&ui_filters.origin_tasks, value, name, entry_enabled, foreground, background);
            break;

        case FILTER_DESTINATION_TASKS:
            ui_filter_add (&ui_filters.destination_tasks, value, name, entry_enabled, foreground, background);
            break;

        case FILTER_INSTANCES:
            ui_filter_add (&ui_filters.instances, value, name, entry_enabled, foreground, background);
            break;

        default:
            g_warning("unknown filter type %d", filter);
            break;
    }
}

static gboolean ui_item_enabled(ui_filter_t *filter, const uint32_t value)
{
    int item;

    if (value != (uint32_t) ~0)
    {
        item = ui_filters_search_id (filter, value);

        if (item >= 0)
        {
            return (filter->items[item].enabled ? TRUE : FALSE);
        }
    }
    return (FALSE);
}

gboolean ui_filters_message_enabled(const uint32_t message, const uint32_t origin_task, const uint32_t destination_task,
                                    const uint32_t instance)
{
    gboolean result;

    result = (ui_item_enabled (&ui_filters.messages, message) && ui_item_enabled (&ui_filters.origin_tasks, origin_task)
            && ui_item_enabled (&ui_filters.destination_tasks, destination_task)
            && ui_item_enabled (&ui_filters.instances, instance));

    return result;
}

static ui_filter_e ui_filter_from_name(const char *name)
{
    if (strcmp (name, ui_filters.messages.name) == 0)
    {
        return FILTER_MESSAGES;
    }
    if (strcmp (name, ui_filters.origin_tasks.name) == 0)
    {
        return FILTER_ORIGIN_TASKS;
    }
    if (strcmp (name, ui_filters.destination_tasks.name) == 0)
    {
        return FILTER_DESTINATION_TASKS;
    }
    if (strcmp (name, ui_filters.instances.name) == 0)
    {
        return FILTER_INSTANCES;
    }
    return FILTER_UNKNOWN;
}

static int xml_parse_filters(xmlDocPtr doc, const char *file_name)
{
    xmlNode *root_element = NULL;
    xmlNode *filter_node = NULL;
    xmlNode *cur_node = NULL;
    ui_filter_e filter;
    guint filters_entries = 0;
    int ret = RC_FAIL;

    /* Get the root element node */
    root_element = xmlDocGetRootElement (doc);

    if (root_element != NULL)
    {
        /* Search for the start of filters definition */
        for (cur_node = root_element; (cur_node != NULL) && (strcmp ((char *) cur_node->name, "filters") != 0);
                cur_node = cur_node->next)
            ;

        if (cur_node != NULL)
        {
            /* Search for filter header */
            for (filter_node = cur_node->children; filter_node != NULL;)
            {
                /* Search for next element node */
                for (; (filter_node != NULL) && (filter_node->type != XML_ELEMENT_NODE); filter_node =
                        filter_node->next)
                    ;

                if (filter_node != NULL)
                {
                    filter = ui_filter_from_name ((const char*) filter_node->name);
                    g_debug("Found filter %s %d", filter_node->name, filter);

                    if (filter == FILTER_UNKNOWN)
                    {
                        g_warning("Unknown filter \"%s\"", filter_node->name);
                    }
                    else
                    {
                        /* Search for entries */
                        for (cur_node = filter_node->children; cur_node != NULL;)
                        {
                            /* Search for next element node */
                            for (; (cur_node != NULL) && (cur_node->type != XML_ELEMENT_NODE); cur_node =
                                    cur_node->next)
                                ;

                            if (cur_node != NULL)
                            {
                                xmlAttr *prop_node;
                                ui_entry_enabled_e enabled = ENTRY_ENABLED_UNDEFINED;
                                char *foreground = NULL;
                                char *background = NULL;

                                for (prop_node = cur_node->properties; prop_node != NULL; prop_node = prop_node->next)
                                {
                                    if (strcmp ((char *) prop_node->name, ENABLED_NAME) == 0)
                                    {
                                        enabled =
                                                prop_node->children->content[0] == '0' ?
                                                        ENTRY_ENABLED_FALSE : ENTRY_ENABLED_TRUE;
                                    }
                                    if (strcmp ((char *) prop_node->name, FOREGROUND_NAME) == 0)
                                    {
                                        foreground = (char *) prop_node->children->content;
                                    }
                                    if (strcmp ((char *) prop_node->name, BACKGROUND_NAME) == 0)
                                    {
                                        background = (char *) prop_node->children->content;
                                    }
                                }

                                g_debug("  Found entry %s %s", cur_node->name, cur_node->properties->children->content);
                                ui_filters_add (filter, FILTER_ID_UNDEFINED, (const char*) cur_node->name, enabled,
                                                foreground, background);

                                filters_entries++;
                                cur_node = cur_node->next;
                            }
                        }
                    }
                    filter_node = filter_node->next;
                }
            }

            /* Filters have changed destroy filter menus and update tree view */
            ui_destroy_filter_menus ();

            /* Reactivate filtering */
            gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.filters_enabled), TRUE);
            ui_tree_view_refilter ();
        }
    }
    /* Free the document */
    xmlFreeDoc (doc);

    if (filters_entries > 0)
    {
        ret = RC_OK;
    }

    g_message("Parsed XML filters file \"%s\", found %d entries (%d messages to display)", file_name, filters_entries, ui_tree_view_get_filtered_number());

    return ret;
}

int ui_filters_read(const char *file_name)
{
    xmlDocPtr doc; /* the resulting document tree */
    int ret;

    if (file_name == NULL)
    {
        g_warning("No name for filters file");
        return RC_FAIL;
    }

    doc = xmlReadFile (file_name, NULL, 0);
    if (doc == NULL)
    {
        ui_notification_dialog (GTK_MESSAGE_ERROR, FALSE, "open filters", "Failed to parse file \"%s\"", file_name);
        return RC_FAIL;
    }

    ret = xml_parse_filters (doc, file_name);
    if (ret != RC_OK)
    {
        ui_notification_dialog (GTK_MESSAGE_WARNING, FALSE, "open filters", "Found no filter definitions in \"%s\"",
                                file_name);
        return RC_FAIL;
    }

    return ret;
}

static void write_filter(FILE *filter_file, ui_filter_t *filter, gboolean save_colors)
{
    int item;

    fprintf (filter_file, "  <%s>\n", filter->name);
    for (item = 0; item < filter->used; item++)
    {
        if (save_colors)
        {
            fprintf (filter_file,
                     "    <%s " ENABLED_NAME "=\"%d\" " FOREGROUND_NAME "=\"%s\" " BACKGROUND_NAME "=\"%s\"/>\n",
                     filter->items[item].name, filter->items[item].enabled ? 1 : 0, filter->items[item].foreground,
                     filter->items[item].background);
        }
        else
        {
            fprintf (filter_file, "    <%s " ENABLED_NAME "=\"%d\"/>\n", filter->items[item].name,
                     filter->items[item].enabled ? 1 : 0);
        }
    }
    fprintf (filter_file, "  </%s>\n", filter->name);
}

int ui_filters_file_write(const char *file_name)
{
    FILE *filter_file;

    if (file_name == NULL)
    {
        g_warning("No name for filters file");
        return RC_FAIL;
    }

    filter_file = fopen (file_name, "w");
    if (filter_file == NULL)
    {
        ui_notification_dialog (GTK_MESSAGE_ERROR, FALSE, "Failed to open file \"%s\": %s", file_name, g_strerror (errno));
        return RC_FAIL;
    }

    fprintf (filter_file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
             "<filters>\n");

    write_filter (filter_file, &ui_filters.messages, TRUE);
    write_filter (filter_file, &ui_filters.origin_tasks, FALSE);
    write_filter (filter_file, &ui_filters.destination_tasks, FALSE);

    fprintf (filter_file, "</filters>\n");

    fclose (filter_file);
    return RC_OK;
}

static void ui_create_filter_menu(GtkWidget **menu, ui_filter_t *filter)
{
    if (*menu == NULL)
    {
        GtkWidget *menu_items;
        int item;
        gpointer data;

        *menu = gtk_menu_new ();

        /* Create the "NONE" menu-item */
        {
            /* Create a new menu-item with a name */
            menu_items = gtk_menu_item_new_with_label ("NONE");

            /* Add it to the menu. */
            gtk_menu_shell_append (GTK_MENU_SHELL(*menu), menu_items);

            g_debug("ui_create_filter_menu %lx", (long) menu_items);
            g_signal_connect(G_OBJECT(menu_items), "activate", G_CALLBACK(ui_callback_on_menu_none), *menu);

            /* Show the widget */
            gtk_widget_show (menu_items);
        }

        /* Create the "ALL" menu-item */
        {
            /* Create a new menu-item with a name */
            menu_items = gtk_menu_item_new_with_label ("ALL");

            /* Add it to the menu. */
            gtk_menu_shell_append (GTK_MENU_SHELL(*menu), menu_items);

            g_debug("ui_create_filter_menu %lx", (long) menu_items);
            g_signal_connect(G_OBJECT(menu_items), "activate", G_CALLBACK(ui_callback_on_menu_all), *menu);

            /* Show the widget */
            gtk_widget_show (menu_items);
        }

        /* Create separator */
        {
            menu_items = gtk_menu_item_new ();

            /* Add it to the menu. */
            gtk_menu_shell_append (GTK_MENU_SHELL(*menu), menu_items);

            /* Show the widget */
            gtk_widget_show (menu_items);
        }

        /* Creates menu-items */
        for (item = 0; item < filter->used; item++)
        {
            /* Create a new menu-item with a name */
            menu_items = gtk_check_menu_item_new_with_label (filter->items[item].name);

            /* Add it to the menu. */
            gtk_menu_shell_append (GTK_MENU_SHELL(*menu), menu_items);

            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menu_items), filter->items[item].enabled);

            /* Connect function to be called when the menu item is selected */
            data = &filter->items[item];
            g_debug("ui_create_filter_menu %lx %lx", (long) menu_items, (long) data);
            g_signal_connect(G_OBJECT(menu_items), "activate", G_CALLBACK(ui_callback_on_menu_item_selected), data);
            /* Save the menu_item reference */
            filter->items[item].menu_item = menu_items;

            /* Show the widget */
            gtk_widget_show (menu_items);
        }
    }
}

void ui_create_filter_menus(void)
{
    ui_create_filter_menu (&ui_main_data.menu_filter_messages, &ui_filters.messages);
    ui_create_filter_menu (&ui_main_data.menu_filter_origin_tasks, &ui_filters.origin_tasks);
    ui_create_filter_menu (&ui_main_data.menu_filter_destination_tasks, &ui_filters.destination_tasks);
    ui_create_filter_menu (&ui_main_data.menu_filter_instances, &ui_filters.instances);
}

static void ui_destroy_filter_menu_item(GtkWidget *widget, gpointer data)
{
    if (GTK_IS_MENU_ITEM(widget))
    {
        gtk_widget_destroy (widget);
    }
}

static void ui_destroy_filter_menu_widget(GtkWidget **menu)
{
    if (*menu != NULL)
    {
        gtk_container_foreach (GTK_CONTAINER(*menu), ui_destroy_filter_menu_item, NULL);

        gtk_widget_destroy (*menu);
        *menu = NULL;
    }
}

void ui_destroy_filter_menus(void)
{
    ui_destroy_filter_menu_widget (&ui_main_data.menu_filter_messages);
    ui_destroy_filter_menu_widget (&ui_main_data.menu_filter_origin_tasks);
    ui_destroy_filter_menu_widget (&ui_main_data.menu_filter_destination_tasks);
    ui_destroy_filter_menu_widget (&ui_main_data.menu_filter_instances);
}

void ui_destroy_filter_menu(ui_filter_e filter)
{
    switch (filter)
    {
        case FILTER_MESSAGES:
            ui_destroy_filter_menu_widget (&ui_main_data.menu_filter_messages);
            break;

        case FILTER_ORIGIN_TASKS:
            ui_destroy_filter_menu_widget (&ui_main_data.menu_filter_origin_tasks);
            break;

        case FILTER_DESTINATION_TASKS:
            ui_destroy_filter_menu_widget (&ui_main_data.menu_filter_destination_tasks);
            break;

        case FILTER_INSTANCES:
            ui_destroy_filter_menu_widget (&ui_main_data.menu_filter_instances);
            break;

        default:
            g_warning("unknown filter type %d", filter);
            break;
    }
}

void ui_show_filter_menu(GtkWidget **menu, ui_filter_t *filter)
{
    ui_create_filter_menu (menu, filter);

    gtk_menu_popup (GTK_MENU (*menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
}
