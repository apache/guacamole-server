
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#ifndef _GUAC_PLUGIN_H
#define _GUAC_PLUGIN_H

/**
 * Provides functions and structures required for handling a client plugin.
 *
 * @file plugin.h
 */

/**
 * String prefix which begins the library filename of all client plugins.
 */
#define GUAC_PROTOCOL_LIBRARY_PREFIX "libguac-client-"

/**
 * String suffix which ends the library filename of all client plugins.
 */
#define GUAC_PROTOCOL_LIBRARY_SUFFIX ".so"

/**
 * The maximum number of characters (COUNTING NULL TERMINATOR) to allow
 * for protocol names within the library filename of client plugins.
 */
#define GUAC_PROTOCOL_NAME_LIMIT 256

/**
 * The maximum number of characters (INCLUDING NULL TERMINATOR) that a
 * character array containing the concatenation of the library prefix,
 * protocol name, and suffix can contain, assuming the protocol name is
 * limited to GUAC_PROTOCOL_NAME_LIMIT characters.
 */
#define GUAC_PROTOCOL_LIBRARY_LIMIT (                                  \
                                                                       \
      sizeof(GUAC_PROTOCOL_LIBRARY_PREFIX) - 1 /* "libguac-client-" */ \
    +        GUAC_PROTOCOL_NAME_LIMIT      - 1 /* [up to 256 chars] */ \
    + sizeof(GUAC_PROTOCOL_LIBRARY_SUFFIX) - 1 /* ".so"             */ \
    + 1                                        /* NULL terminator   */ \
                                                                       \
)

typedef struct guac_client_plugin guac_client_plugin;

/**
 * A handle to a client plugin, containing enough information about the
 * plugin to complete the initial protocol handshake and instantiate a new
 * client supporting the protocol provided by the client plugin. 
 */
struct guac_client_plugin {

    /**
     * Reference to dlopen'd client plugin.
     */
    void* __client_plugin_handle;

    /**
     * Reference to the init handler of this client plugin. This
     * function will be called when the client plugin is started.
     */
    guac_client_init_handler* init_handler;

    /**
     * NULL-terminated array of all arguments accepted by this client
     * plugin, in order. The values of these arguments will be passed
     * to the init_handler if the client plugin is started.
     */
    const char** args;

};

/**
 * Open the plugin which provides support for the given protocol, if it
 * exists.
 *
 * @param protocol The name of the protocol to retrieve the client plugin
 *                 for.
 * @return The client plugin supporting the given protocol, or NULL if
 *         an error occurs or no such plugin exists.
 */
guac_client_plugin* guac_client_plugin_open(const char* protocol);

/**
 * Close the given plugin, releasing all associated resources. This function
 * must be called after use of a client plugin is finished.
 *
 * @param plugin The client plugin to close.
 * @return Zero on success, non-zero if an error occurred while releasing
 *         the resources associated with the plugin.
 */
int guac_client_plugin_close(guac_client_plugin* plugin);

/**
 * Initializes the given guac_client using the initialization routine provided
 * by the given guac_client_plugin.
 *
 * @param plugin The client plugin to use to initialize the new client.
 * @param client The guac_client to initialize.
 * @param argc The number of arguments being passed to the client.
 * @param argv All arguments to be passed to the client.
 * @return Zero if initialization was successful, non-zero otherwise.
 */
int guac_client_plugin_init_client(guac_client_plugin* plugin, 
        guac_client* client, int argc, char** argv);

#endif
