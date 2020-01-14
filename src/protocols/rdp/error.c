/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "error.h"
#include "rdp.h"

#include <freerdp/freerdp.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

void guac_rdp_client_abort(guac_client* client) {

    /*
     * NOTE: The RDP status codes translated here are documented within
     * [MS-RDPBCGR], section 2.2.5.1.1: "Set Error Info PDU Data", in the
     * description of the "errorInfo" field.
     *
     * https://msdn.microsoft.com/en-us/library/cc240544.aspx
     */

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    freerdp* rdp_inst = rdp_client->rdp_inst;

    guac_protocol_status status;
    const char* message;

    /* Read disconnect reason code from connection */
    int error_info = freerdp_error_info(rdp_inst);

    /* Translate reason code into Guacamole protocol status */
    switch (error_info) {

        /* Normal disconnect */
        case 0x0: /* ERRINFO_SUCCESS */
            status = GUAC_PROTOCOL_STATUS_SUCCESS;
            message = "Disconnected.";
            break;

        /* Forced disconnect (possibly by admin) */
        case 0x1: /* ERRINFO_RPC_INITIATED_DISCONNECT */
            status = GUAC_PROTOCOL_STATUS_SESSION_CLOSED;
            message = "Forcibly disconnected.";
            break;

        /* The user was logged off (possibly by admin) */
        case 0x2: /* ERRINFO_RPC_INITIATED_LOGOFF */
            status = GUAC_PROTOCOL_STATUS_SESSION_CLOSED;
            message = "Logged off.";
            break;

        /* The user was idle long enough that the RDP server disconnected */
        case 0x3: /* ERRINFO_IDLE_TIMEOUT */
            status = GUAC_PROTOCOL_STATUS_SESSION_TIMEOUT;
            message = "Idle session time limit exceeded.";
            break;

        /* The user's session has been active for too long */
        case 0x4: /* ERRINFO_LOGON_TIMEOUT */
            status = GUAC_PROTOCOL_STATUS_SESSION_CLOSED;
            message = "Active session time limit exceeded.";
            break;

        /* Another user logged on, disconnecting this user */
        case 0x5: /* ERRINFO_DISCONNECTED_BY_OTHER_CONNECTION */
            status = GUAC_PROTOCOL_STATUS_SESSION_CONFLICT;
            message = "Disconnected by other connection.";
            break;

        /* The RDP server is refusing to service the connection */
        case 0x6: /* ERRINFO_OUT_OF_MEMORY */
        case 0x7: /* ERRINFO_SERVER_DENIED_CONNECTION */
            status = GUAC_PROTOCOL_STATUS_UPSTREAM_UNAVAILABLE;
            message = "Server refused connection.";
            break;

        /* The user does not have permission to connect */
        case 0x9: /* ERRINFO_SERVER_INSUFFICIENT_PRIVILEGES */
            status = GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN;
            message = "Insufficient privileges.";
            break;

        /* The user's credentials have expired */
        case 0xA: /* ERRINFO_SERVER_FRESH_CREDENTIALS_REQUIRED */
            status = GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN;
            message = "Credentials expired.";
            break;

        /* The user manually disconnected using an administrative tool within
         * the session */
        case 0xB: /* ERRINFO_RPC_INITIATED_DISCONNECT_BYUSER */
            status = GUAC_PROTOCOL_STATUS_SUCCESS;
            message = "Manually disconnected.";
            break;

        /* The user manually logged off */
        case 0xC: /* ERRINFO_LOGOFF_BY_USER */
            status = GUAC_PROTOCOL_STATUS_SUCCESS;
            message = "Manually logged off.";
            break;

        /* Unimplemented/unknown disconnect reason code */
        default:
            status = GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR;
            message = "Upstream error.";

    }

    /* Send error code if an error occurred */
    if (status != GUAC_PROTOCOL_STATUS_SUCCESS) {
        guac_protocol_send_error(client->socket, message, status);
        guac_socket_flush(client->socket);
    }

    /* Log human-readable description of disconnect at info level */
    guac_client_log(client, GUAC_LOG_INFO, "RDP server closed connection: %s",
            message);

    /* Log internal disconnect reason code at debug level */
    if (error_info)
        guac_client_log(client, GUAC_LOG_DEBUG, "Disconnect reason "
                "code: 0x%X.", error_info);

    /* Abort connection */
    guac_client_stop(client);

}

