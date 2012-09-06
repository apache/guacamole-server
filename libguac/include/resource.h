
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


#ifndef _GUAC_RESOURCE_H
#define _GUAC_RESOURCE_H

/**
 * Provides functions and structures required for handling resources.
 *
 * NOTE: The data and end instructions are currently implemented client-side
 *       only, and allocation of resources must ALWAYS be server-side.
 *
 *       Each resource is mono-directional. Two resources must be allocated for
 *       bidirectional communication.
 *
 *       Exposure of client-side resources to the server will be accomplished
 *       over the same protocol (resource -> accept/reject -> data -> end). The
 *       mono-directional nature of resources will allow the index spaces of
 *       client and server resources to be independent.
 *
 * @file resource.h
 */

typedef struct guac_resource guac_resource;

/**
 * Handler which begins resource transfer when the client accepts an exposed resource.
 */
typedef int guac_resource_accept_handler(guac_resource* resource, const char* mimetype);

/**
 * Handler which cancels resource transfer when the client rejects an exposed resource.
 */
typedef int guac_resource_reject_handler(guac_resource* resource);

/**
 * Represents a single resource which can be requested or exposed via
 * the Guacamole protocol.
 */
struct guac_resource {

    /**
     * The index of this resource.
     */
    int index;

    /**
     * Handler which will be called when this resource is accepted by the client.
     */
    guac_resource_accept_handler* accept_handler;

    /**
     * Handler which will be called when this resource is rejected by the client.
     */
    guac_resource_reject_handler* reject_handler;

    /**
     * Arbitrary data associated with this resource.
     */
    void* data;

};

#endif
