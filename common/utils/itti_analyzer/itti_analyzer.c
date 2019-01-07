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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libxml/parser.h>

#include "xml_parse.h"
#include "resolvers.h"
#include "locate_root.h"
#include "file.h"
#include "ui_main_screen.h"

#include "rc.h"

#define G_LOG_LEVELS (G_LOG_LEVEL_ERROR     | \
                      G_LOG_LEVEL_CRITICAL  | \
                      G_LOG_LEVEL_WARNING   | \
                      G_LOG_LEVEL_MESSAGE   | \
                      G_LOG_LEVEL_INFO      | \
                      G_LOG_LEVEL_DEBUG)

int debug_buffers = 1;
int debug_parser = 0;

static void console_log_handler(const char *log_domain, GLogLevelFlags log_level,
                                const char *message, gpointer user_data)
{
    GLogLevelFlags domain_log_level = (GLogLevelFlags) user_data;
    time_t curr;
    struct tm *today;
    const char *level;

    if (ui_main_data.log_flags & domain_log_level & log_level)
    {
        switch (log_level & G_LOG_LEVEL_MASK)
        {
        case G_LOG_LEVEL_ERROR:
            level = "Err ";
            break;
        case G_LOG_LEVEL_CRITICAL:
            level = "Crit";
            break;
        case G_LOG_LEVEL_WARNING:
            level = "Warn";
            break;
        case G_LOG_LEVEL_MESSAGE:
            level = "Msg ";
            break;
        case G_LOG_LEVEL_INFO:
            level = "Info";
            break;
        case G_LOG_LEVEL_DEBUG:
            level = "Dbg ";
            break;
        default:
            fprintf(stderr, "unknown log_level %u\n", log_level);
            level = NULL;
            g_assert_not_reached();
            break;
        }

        /* create a "timestamp" */
        time(&curr);
        today = localtime(&curr);

        fprintf(stderr, "%02u:%02u:%02u %-9s %s %s\n", today->tm_hour, today->tm_min, today->tm_sec,
                log_domain != NULL ? log_domain : "", level, message);
    }
}

int main(int argc, char *argv[])
{
    int ret = 0;

    GLogLevelFlags log_flags;

    log_flags = (GLogLevelFlags)
        (G_LOG_LEVEL_ERROR      |
        G_LOG_LEVEL_CRITICAL    |
        G_LOG_LEVEL_WARNING     |
        G_LOG_LEVEL_MESSAGE     |
        G_LOG_LEVEL_INFO        |
        G_LOG_LEVEL_DEBUG);

    /* This initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION;
    xmlInitParser();

    /* Initialize the widget set */
    gtk_init(&argc, &argv);

    /* Parse command line options */
    ui_gtk_parse_arg (argc, argv);

    /* Set log handlers:
     *                 Domain,      Levels,    Handler,             Domain enabled levels */
    g_log_set_handler( NULL,        log_flags, console_log_handler, (gpointer) (G_LOG_LEVELS));
    g_log_set_handler("BUFFERS",    log_flags, console_log_handler, (gpointer) (G_LOG_LEVELS & (~(G_LOG_LEVEL_DEBUG))));
    g_log_set_handler("PARSER",     log_flags, console_log_handler, (gpointer) (G_LOG_LEVELS & (~(G_LOG_LEVEL_DEBUG))));
    g_log_set_handler("RESOLVER",   log_flags, console_log_handler, (gpointer) (G_LOG_LEVELS & (~(G_LOG_LEVEL_DEBUG))));
    g_log_set_handler("UI",         log_flags, console_log_handler, (gpointer) (G_LOG_LEVELS & (~(G_LOG_LEVEL_DEBUG))));
    g_log_set_handler("UI_CB",      log_flags, console_log_handler, (gpointer) (G_LOG_LEVELS));
    g_log_set_handler("UI_FILTER",  log_flags, console_log_handler, (gpointer) (G_LOG_LEVELS & (~(G_LOG_LEVEL_DEBUG))));
    g_log_set_handler("UI_INTER",   log_flags, console_log_handler, (gpointer) (G_LOG_LEVELS));
    g_log_set_handler("UI_TREE",    log_flags, console_log_handler, (gpointer) (G_LOG_LEVELS & (~(G_LOG_LEVEL_DEBUG))));

    CHECK_FCT(ui_gtk_initialize(argc, argv));

    /* Enter the main event loop, and wait for user interaction */
    gtk_main ();

    /* Free the global variables that may
     * have been allocated by the parser.
     */
    xmlCleanupParser ();

    return ret;
}
