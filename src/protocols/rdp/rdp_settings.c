/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "client.h"
#include "guac_string.h"
#include "rdp.h"
#include "rdp_settings.h"
#include "resolution.h"

#include <freerdp/constants.h>
#include <freerdp/settings.h>
#include <guacamole/user.h>

#ifdef ENABLE_WINPR
#include <winpr/wtypes.h>
#else
#include "compat/winpr-wtypes.h"
#endif

#include <stddef.h>
#include <string.h>

/* Client plugin arguments */
const char* GUAC_RDP_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "domain",
    "username",
    "password",
    "width",
    "height",
    "dpi",
    "initial-program",
    "color-depth",
    "disable-audio",
    "enable-printing",
    "enable-drive",
    "drive-path",
    "create-drive-path",
    "console",
    "console-audio",
    "server-layout",
    "security",
    "ignore-cert",
    "disable-auth",
    "remote-app",
    "remote-app-dir",
    "remote-app-args",
    "static-channels",
    "client-name",
    "enable-wallpaper",
    "enable-theming",
    "enable-font-smoothing",
    "enable-full-window-drag",
    "enable-desktop-composition",
    "enable-menu-animations",
    "preconnection-id",
    "preconnection-blob",

#ifdef ENABLE_COMMON_SSH
    "enable-sftp",
    "sftp-hostname",
    "sftp-port",
    "sftp-username",
    "sftp-password",
    "sftp-private-key",
    "sftp-passphrase",
    "sftp-directory",
#endif

    "recording-path",
    "recording-name",
    "create-recording-path",

    NULL
};

enum RDP_ARGS_IDX {

    /**
     * The hostname to connect to.
     */
    IDX_HOSTNAME,

    /**
     * The port to connect to. If omitted, the default RDP port of 3389 will be
     * used.
     */
    IDX_PORT,

    /**
     * The domain of the user logging in.
     */
    IDX_DOMAIN,

    /**
     * The username of the user logging in.
     */
    IDX_USERNAME,

    /**
     * The password of the user logging in.
     */
    IDX_PASSWORD,

    /**
     * The width of the display to request, in pixels. If omitted, a reasonable
     * value will be calculated based on the user's own display size and
     * resolution.
     */
    IDX_WIDTH,

    /**
     * The height of the display to request, in pixels. If omitted, a
     * reasonable value will be calculated based on the user's own display
     * size and resolution.
     */
    IDX_HEIGHT,

    /**
     * The resolution of the display to request, in DPI. If omitted, a
     * reasonable value will be calculated based on the user's own display
     * size and resolution.
     */
    IDX_DPI,

    /**
     * The initial program to run, if any.
     */
    IDX_INITIAL_PROGRAM,

    /**
     * The color depth of the display to request, in bits.
     */
    IDX_COLOR_DEPTH,

    /**
     * "true" if audio should be disabled, "false" or blank to leave audio
     * enabled.
     */
    IDX_DISABLE_AUDIO,

    /**
     * "true" if printing should be enabled, "false" or blank otherwise.
     */
    IDX_ENABLE_PRINTING,

    /**
     * "true" if the virtual drive should be enabled, "false" or blank
     * otherwise.
     */
    IDX_ENABLE_DRIVE,

    /**
     * The local system path which will be used to persist the
     * virtual drive. This must be specified if the virtual drive is enabled.
     */
    IDX_DRIVE_PATH,

    /**
     * "true" to automatically create the local system path used by the virtual
     * drive if it does not yet exist, "false" or blank otherwise.
     */
    IDX_CREATE_DRIVE_PATH,

    /**
     * "true" if this session is a console session, "false" or blank otherwise.
     */
    IDX_CONSOLE,

    /**
     * "true" if audio should be allowed in console sessions, "false" or blank
     * otherwise.
     */
    IDX_CONSOLE_AUDIO,

    /**
     * The name of the keymap chosen as the layout of the server. Legal names
     * are defined within the *.keymap files in the "keymaps" directory of the
     * source for Guacamole's RDP support.
     */
    IDX_SERVER_LAYOUT,

    /**
     * The type of security to use for the connection. Valid values are "rdp",
     * "tls", "nla", or "any". By default, "rdp" security is used.
     */
    IDX_SECURITY,

    /**
     * "true" if validity of the RDP server's certificate should be ignored,
     * "false" or blank if invalid certificates should result in a failure to
     * connect.
     */
    IDX_IGNORE_CERT,

    /**
     * "true" if authentication should be disabled, "false" or blank otherwise.
     * This is different from the authentication that takes place when a user
     * provides their username and password. Authentication is required by
     * definition for NLA.
     */
    IDX_DISABLE_AUTH,

    /**
     * The application to launch, if RemoteApp is in use.
     */
    IDX_REMOTE_APP,

    /**
     * The working directory of the remote application, if RemoteApp is in use.
     */
    IDX_REMOTE_APP_DIR,

    /**
     * The arguments to pass to the remote application, if RemoteApp is in use.
     */
    IDX_REMOTE_APP_ARGS,

    /**
     * Comma-separated list of the names of all static virtual channels that
     * should be connected to and exposed as Guacamole pipe streams, or blank
     * if no static virtual channels should be used. 
     */
    IDX_STATIC_CHANNELS,

    /**
     * The name of the client to submit to the RDP server upon connection.
     */
    IDX_CLIENT_NAME,

    /**
     * "true" if the desktop wallpaper should be visible, "false" or blank if
     * the desktop wallpaper should be hidden.
     */
    IDX_ENABLE_WALLPAPER,

    /**
     * "true" if desktop and window theming should be allowed, "false" or blank
     * if theming should be temporarily disabled on the desktop of the RDP
     * server for the sake of performance.
     */
    IDX_ENABLE_THEMING,

    /**
     * "true" if glyphs should be smoothed with antialiasing (ClearType),
     * "false" or blank if glyphs should be rendered with sharp edges and using
     * single colors, effectively 1-bit images.
     */
    IDX_ENABLE_FONT_SMOOTHING,

    /**
     * "true" if windows' contents should be shown as they are moved, "false"
     * or blank if only a window border should be shown during window move
     * operations.
     */
    IDX_ENABLE_FULL_WINDOW_DRAG,

    /**
     * "true" if desktop composition (Aero) should be enabled during the
     * session, "false" or blank otherwise.  As desktop composition provides
     * alpha blending and other special effects, this increases the amount of
     * bandwidth used.
     */
    IDX_ENABLE_DESKTOP_COMPOSITION,

    /**
     * "true" if menu animations should be shown, "false" or blank menus should
     * not be animated.
     */
    IDX_ENABLE_MENU_ANIMATIONS,

    /**
     * The preconnection ID to send within the preconnection PDU when
     * initiating an RDP connection, if any.
     */
    IDX_PRECONNECTION_ID,

    /**
     * The preconnection BLOB (PCB) to send to the RDP server prior to full RDP
     * connection negotiation. This value is used by Hyper-V to select the
     * destination VM.
     */
    IDX_PRECONNECTION_BLOB,

#ifdef ENABLE_COMMON_SSH
    /**
     * "true" if SFTP should be enabled for the RDP connection, "false" or
     * blank otherwise.
     */
    IDX_ENABLE_SFTP,

    /**
     * The hostname of the SSH server to connect to for SFTP. If blank, the
     * hostname of the RDP server will be used.
     */
    IDX_SFTP_HOSTNAME,

    /**
     * The port of the SSH server to connect to for SFTP. If blank, the default
     * SSH port of "22" will be used.
     */
    IDX_SFTP_PORT,

    /**
     * The username to provide when authenticating with the SSH server for
     * SFTP. If blank, the username provided for the RDP user will be used.
     */
    IDX_SFTP_USERNAME,

    /**
     * The password to provide when authenticating with the SSH server for
     * SFTP (if not using a private key).
     */
    IDX_SFTP_PASSWORD,

    /**
     * The base64-encoded private key to use when authenticating with the SSH
     * server for SFTP (if not using a password).
     */
    IDX_SFTP_PRIVATE_KEY,

    /**
     * The passphrase to use to decrypt the provided base64-encoded private
     * key.
     */
    IDX_SFTP_PASSPHRASE,

    /**
     * The default location for file uploads within the SSH server. This will
     * apply only to uploads which do not use the filesystem guac_object (where
     * the destination directory is otherwise ambiguous).
     */
    IDX_SFTP_DIRECTORY,

#endif

    /**
     * The full absolute path to the directory in which screen recordings
     * should be written.
     */
    IDX_RECORDING_PATH,

    /**
     * The name that should be given to screen recordings which are written in
     * the given path.
     */
    IDX_RECORDING_NAME,

    /**
     * Whether the specified screen recording path should automatically be
     * created if it does not yet exist.
     */
    IDX_CREATE_RECORDING_PATH,

    RDP_ARGS_COUNT
};

guac_rdp_settings* guac_rdp_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != RDP_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                RDP_ARGS_COUNT, argc);
        return NULL;
    }

    guac_rdp_settings* settings = calloc(1, sizeof(guac_rdp_settings));

    /* Use console */
    settings->console =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_CONSOLE, 0);

    /* Enable/disable console audio */
    settings->console_audio =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_CONSOLE_AUDIO, 0);

    /* Ignore SSL/TLS certificate */
    settings->ignore_certificate =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_IGNORE_CERT, 0);

    /* Disable authentication */
    settings->disable_authentication =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DISABLE_AUTH, 0);

    /* NLA security */
    if (strcmp(argv[IDX_SECURITY], "nla") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Security mode: NLA");
        settings->security_mode = GUAC_SECURITY_NLA;
    }

    /* TLS security */
    else if (strcmp(argv[IDX_SECURITY], "tls") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Security mode: TLS");
        settings->security_mode = GUAC_SECURITY_TLS;
    }

    /* RDP security */
    else if (strcmp(argv[IDX_SECURITY], "rdp") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Security mode: RDP");
        settings->security_mode = GUAC_SECURITY_RDP;
    }

    /* ANY security (allow server to choose) */
    else if (strcmp(argv[IDX_SECURITY], "any") == 0) {
        guac_user_log(user, GUAC_LOG_INFO, "Security mode: ANY");
        settings->security_mode = GUAC_SECURITY_ANY;
    }

    /* If nothing given, default to RDP */
    else {
        guac_user_log(user, GUAC_LOG_INFO, "No security mode specified. Defaulting to RDP.");
        settings->security_mode = GUAC_SECURITY_RDP;
    }

    /* Set hostname */
    settings->hostname =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_HOSTNAME, "");

    /* If port specified, use it */
    settings->port =
        guac_user_parse_args_int(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_PORT, RDP_DEFAULT_PORT);

    guac_user_log(user, GUAC_LOG_DEBUG,
            "User resolution is %ix%i at %i DPI",
            user->info.optimal_width,
            user->info.optimal_height,
            user->info.optimal_resolution);

    /* Use suggested resolution unless overridden */
    settings->resolution =
        guac_user_parse_args_int(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DPI, guac_rdp_suggest_resolution(user));

    /* Use optimal width unless overridden */
    settings->width = user->info.optimal_width
                    * settings->resolution
                    / user->info.optimal_resolution;

    if (argv[IDX_WIDTH][0] != '\0')
        settings->width = atoi(argv[IDX_WIDTH]);

    /* Use default width if given width is invalid. */
    if (settings->width <= 0) {
        settings->width = RDP_DEFAULT_WIDTH;
        guac_user_log(user, GUAC_LOG_ERROR,
                "Invalid width: \"%s\". Using default of %i.",
                argv[IDX_WIDTH], settings->width);
    }

    /* Round width down to nearest multiple of 4 */
    settings->width = settings->width & ~0x3;

    /* Use optimal height unless overridden */
    settings->height = user->info.optimal_height
                     * settings->resolution
                     / user->info.optimal_resolution;

    if (argv[IDX_HEIGHT][0] != '\0')
        settings->height = atoi(argv[IDX_HEIGHT]);

    /* Use default height if given height is invalid. */
    if (settings->height <= 0) {
        settings->height = RDP_DEFAULT_HEIGHT;
        guac_user_log(user, GUAC_LOG_ERROR,
                "Invalid height: \"%s\". Using default of %i.",
                argv[IDX_WIDTH], settings->height);
    }

    guac_user_log(user, GUAC_LOG_DEBUG,
            "Using resolution of %ix%i at %i DPI",
            settings->width,
            settings->height,
            settings->resolution);

    /* Domain */
    settings->domain =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DOMAIN, NULL);

    /* Username */
    settings->username =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_USERNAME, NULL);

    /* Password */
    settings->password =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_PASSWORD, NULL);

    /* Client name */
    settings->client_name =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_CLIENT_NAME, NULL);

    /* Initial program */
    settings->initial_program =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_INITIAL_PROGRAM, NULL);

    /* RemoteApp program */
    settings->remote_app =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_REMOTE_APP, NULL);

    /* RemoteApp working directory */
    settings->remote_app_dir =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_REMOTE_APP_DIR, NULL);

    /* RemoteApp arguments */
    settings->remote_app_args =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_REMOTE_APP_ARGS, NULL);

    /* Static virtual channels */
    settings->svc_names = NULL;
    if (argv[IDX_STATIC_CHANNELS][0] != '\0')
        settings->svc_names = guac_split(argv[IDX_STATIC_CHANNELS], ',');

    /*
     * Performance flags
     */

    settings->wallpaper_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_WALLPAPER, 0);

    settings->theming_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_THEMING, 0);

    settings->font_smoothing_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_FONT_SMOOTHING, 0);

    settings->full_window_drag_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_FULL_WINDOW_DRAG, 0);

    settings->desktop_composition_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_DESKTOP_COMPOSITION, 0);

    settings->menu_animations_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_MENU_ANIMATIONS, 0);

    /* Session color depth */
    settings->color_depth = 
        guac_user_parse_args_int(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_COLOR_DEPTH, RDP_DEFAULT_DEPTH);

    /* Preconnection ID */
    settings->preconnection_id = -1;
    if (argv[IDX_PRECONNECTION_ID][0] != '\0') {

        /* Parse preconnection ID, warn if invalid */
        int preconnection_id = atoi(argv[IDX_PRECONNECTION_ID]);
        if (preconnection_id < 0)
            guac_user_log(user, GUAC_LOG_WARNING,
                    "Ignoring invalid preconnection ID: %i",
                    preconnection_id);

        /* Otherwise, assign specified ID */
        else {
            settings->preconnection_id = preconnection_id;
            guac_user_log(user, GUAC_LOG_DEBUG,
                    "Preconnection ID: %i", settings->preconnection_id);
        }

    }

    /* Preconnection BLOB */
    settings->preconnection_blob = NULL;
    if (argv[IDX_PRECONNECTION_BLOB][0] != '\0') {
        settings->preconnection_blob = strdup(argv[IDX_PRECONNECTION_BLOB]);
        guac_user_log(user, GUAC_LOG_DEBUG,
                "Preconnection BLOB: \"%s\"", settings->preconnection_blob);
    }

#ifndef HAVE_RDPSETTINGS_SENDPRECONNECTIONPDU
    /* Warn if support for the preconnection BLOB / ID is absent */
    if (settings->preconnection_blob != NULL
            || settings->preconnection_id != -1) {
        guac_user_log(user, GUAC_LOG_WARNING,
                "Installed version of FreeRDP lacks support for the "
                "preconnection PDU. The specified preconnection BLOB and/or "
                "ID will be ignored.");
    }
#endif

    /* Audio enable/disable */
    settings->audio_enabled =
        !guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DISABLE_AUDIO, 0);

    /* Printing enable/disable */
    settings->printing_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_PRINTING, 0);

    /* Drive enable/disable */
    settings->drive_enabled =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_DRIVE, 0);

    settings->drive_path =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_DRIVE_PATH, "");

    settings->create_drive_path =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_CREATE_DRIVE_PATH, 0);

    /* Pick keymap based on argument */
    settings->server_layout = NULL;
    if (argv[IDX_SERVER_LAYOUT][0] != '\0')
        settings->server_layout =
            guac_rdp_keymap_find(argv[IDX_SERVER_LAYOUT]);

    /* If no keymap requested, use default */
    if (settings->server_layout == NULL)
        settings->server_layout = guac_rdp_keymap_find(GUAC_DEFAULT_KEYMAP);

#ifdef ENABLE_COMMON_SSH
    /* SFTP enable/disable */
    settings->enable_sftp =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_ENABLE_SFTP, 0);

    /* Hostname for SFTP connection */
    settings->sftp_hostname =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_HOSTNAME, settings->hostname);

    /* Port for SFTP connection */
    settings->sftp_port =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_PORT, "22");

    /* Username for SSH/SFTP authentication */
    settings->sftp_username =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_USERNAME,
                settings->username != NULL ? settings->username : "");

    /* Password for SFTP (if not using private key) */
    settings->sftp_password =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_PASSWORD, "");

    /* Private key for SFTP (if not using password) */
    settings->sftp_private_key =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_PRIVATE_KEY, NULL);

    /* Passphrase for decrypting the SFTP private key (if applicable */
    settings->sftp_passphrase =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_PASSPHRASE, "");

    /* Default upload directory */
    settings->sftp_directory =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_SFTP_DIRECTORY, NULL);
#endif

    /* Read recording path */
    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    /* Read recording name */
    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_RDP_DEFAULT_RECORDING_NAME);

    /* Parse path creation flag */
    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_RDP_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, 0);

    /* Success */
    return settings;

}

void guac_rdp_settings_free(guac_rdp_settings* settings) {

    /* Free settings strings */
    free(settings->client_name);
    free(settings->domain);
    free(settings->drive_path);
    free(settings->hostname);
    free(settings->initial_program);
    free(settings->password);
    free(settings->preconnection_blob);
    free(settings->recording_name);
    free(settings->recording_path);
    free(settings->remote_app);
    free(settings->remote_app_args);
    free(settings->remote_app_dir);
    free(settings->username);

    /* Free channel name array */
    free(settings->svc_names);

#ifdef ENABLE_COMMON_SSH
    /* Free SFTP settings */
    free(settings->sftp_directory);
    free(settings->sftp_hostname);
    free(settings->sftp_passphrase);
    free(settings->sftp_password);
    free(settings->sftp_port);
    free(settings->sftp_private_key);
    free(settings->sftp_username);
#endif

    /* Free settings structure */
    free(settings);

}

int guac_rdp_get_width(freerdp* rdp) {
#ifdef LEGACY_RDPSETTINGS
    return rdp->settings->width;
#else
    return rdp->settings->DesktopWidth;
#endif
}

int guac_rdp_get_height(freerdp* rdp) {
#ifdef LEGACY_RDPSETTINGS
    return rdp->settings->height;
#else
    return rdp->settings->DesktopHeight;
#endif
}

int guac_rdp_get_depth(freerdp* rdp) {
#ifdef LEGACY_RDPSETTINGS
    return rdp->settings->color_depth;
#else
    return rdp->settings->ColorDepth;
#endif
}

/**
 * Given the settings structure of the Guacamole RDP client, calculates the
 * standard performance flag value to send to the RDP server. The value of
 * these flags is dictated by the RDP standard.
 *
 * @param guac_settings
 *     The settings structure to read performance settings from.
 *
 * @returns
 *     The standard RDP performance flag value representing the union of all
 *     performance settings within the given settings structure.
 */
static int guac_rdp_get_performance_flags(guac_rdp_settings* guac_settings) {

    /* No performance flags initially */
    int flags = PERF_FLAG_NONE;

    /* Desktop wallpaper */
    if (!guac_settings->wallpaper_enabled)
        flags |= PERF_DISABLE_WALLPAPER;

    /* Theming of desktop/windows */
    if (!guac_settings->theming_enabled)
        flags |= PERF_DISABLE_THEMING;

    /* Font smoothing (ClearType) */
    if (guac_settings->font_smoothing_enabled)
        flags |= PERF_ENABLE_FONT_SMOOTHING;

    /* Full-window drag */
    if (!guac_settings->full_window_drag_enabled)
        flags |= PERF_DISABLE_FULLWINDOWDRAG;

    /* Desktop composition (Aero) */
    if (guac_settings->desktop_composition_enabled)
        flags |= PERF_ENABLE_DESKTOP_COMPOSITION;

    /* Menu animations */
    if (!guac_settings->menu_animations_enabled)
        flags |= PERF_DISABLE_MENUANIMATIONS;

    return flags;

}

void guac_rdp_push_settings(guac_rdp_settings* guac_settings, freerdp* rdp) {

    BOOL bitmap_cache;
    rdpSettings* rdp_settings = rdp->settings;

    /* Authentication */
#ifdef LEGACY_RDPSETTINGS
    rdp_settings->domain = guac_settings->domain;
    rdp_settings->username = guac_settings->username;
    rdp_settings->password = guac_settings->password;
#else
    rdp_settings->Domain = guac_settings->domain;
    rdp_settings->Username = guac_settings->username;
    rdp_settings->Password = guac_settings->password;
#endif

    /* Connection */
#ifdef LEGACY_RDPSETTINGS
    rdp_settings->hostname = guac_settings->hostname;
    rdp_settings->port = guac_settings->port;
#else
    rdp_settings->ServerHostname = guac_settings->hostname;
    rdp_settings->ServerPort = guac_settings->port;
#endif

    /* Session */
#ifdef LEGACY_RDPSETTINGS
    rdp_settings->color_depth = guac_settings->color_depth;
    rdp_settings->width = guac_settings->width;
    rdp_settings->height = guac_settings->height;
    rdp_settings->shell = guac_settings->initial_program;
    rdp_settings->kbd_layout = guac_settings->server_layout->freerdp_keyboard_layout;
#else
    rdp_settings->ColorDepth = guac_settings->color_depth;
    rdp_settings->DesktopWidth = guac_settings->width;
    rdp_settings->DesktopHeight = guac_settings->height;
    rdp_settings->AlternateShell = guac_settings->initial_program;
    rdp_settings->KeyboardLayout = guac_settings->server_layout->freerdp_keyboard_layout;
#endif

    /* Performance flags */
#ifdef LEGACY_RDPSETTINGS
    rdp_settings->performance_flags = guac_rdp_get_performance_flags(guac_settings);
#else
    rdp_settings->PerformanceFlags = guac_rdp_get_performance_flags(guac_settings);
#endif

    /* Client name */
    if (guac_settings->client_name != NULL) {
#ifdef LEGACY_RDPSETTINGS
        strncpy(rdp_settings->client_hostname, guac_settings->client_name,
                RDP_CLIENT_HOSTNAME_SIZE - 1);
#else
        strncpy(rdp_settings->ClientHostname, guac_settings->client_name,
                RDP_CLIENT_HOSTNAME_SIZE - 1);
#endif
    }

    /* Console */
#ifdef LEGACY_RDPSETTINGS
    rdp_settings->console_session = guac_settings->console;
    rdp_settings->console_audio = guac_settings->console_audio;
#else
    rdp_settings->ConsoleSession = guac_settings->console;
    rdp_settings->RemoteConsoleAudio = guac_settings->console_audio;
#endif

    /* Audio */
#ifdef LEGACY_RDPSETTINGS
#ifdef HAVE_RDPSETTINGS_AUDIOPLAYBACK
    rdp_settings->audio_playback = guac_settings->audio_enabled;
#endif
#else
#ifdef HAVE_RDPSETTINGS_AUDIOPLAYBACK
    rdp_settings->AudioPlayback = guac_settings->audio_enabled;
#endif
#endif

    /* Device redirection */
#ifdef LEGACY_RDPSETTINGS
#ifdef HAVE_RDPSETTINGS_DEVICEREDIRECTION
    rdp_settings->device_redirection =  guac_settings->audio_enabled
                                     || guac_settings->drive_enabled
                                     || guac_settings->printing_enabled;
#endif
#else
#ifdef HAVE_RDPSETTINGS_DEVICEREDIRECTION
    rdp_settings->DeviceRedirection =  guac_settings->audio_enabled
                                    || guac_settings->drive_enabled
                                    || guac_settings->printing_enabled;
#endif
#endif

    /* Security */
    switch (guac_settings->security_mode) {

        /* Standard RDP encryption */
        case GUAC_SECURITY_RDP:
#ifdef LEGACY_RDPSETTINGS
            rdp_settings->rdp_security = TRUE;
            rdp_settings->tls_security = FALSE;
            rdp_settings->nla_security = FALSE;
            rdp_settings->encryption_level = ENCRYPTION_LEVEL_CLIENT_COMPATIBLE;
            rdp_settings->encryption_method =
                  ENCRYPTION_METHOD_40BIT
                | ENCRYPTION_METHOD_128BIT
                | ENCRYPTION_METHOD_FIPS;
#else
            rdp_settings->RdpSecurity = TRUE;
            rdp_settings->TlsSecurity = FALSE;
            rdp_settings->NlaSecurity = FALSE;
            rdp_settings->EncryptionLevel = ENCRYPTION_LEVEL_CLIENT_COMPATIBLE;
            rdp_settings->EncryptionMethods =
                  ENCRYPTION_METHOD_40BIT
                | ENCRYPTION_METHOD_128BIT 
                | ENCRYPTION_METHOD_FIPS;
#endif
            break;

        /* TLS encryption */
        case GUAC_SECURITY_TLS:
#ifdef LEGACY_RDPSETTINGS
            rdp_settings->rdp_security = FALSE;
            rdp_settings->tls_security = TRUE;
            rdp_settings->nla_security = FALSE;
#else
            rdp_settings->RdpSecurity = FALSE;
            rdp_settings->TlsSecurity = TRUE;
            rdp_settings->NlaSecurity = FALSE;
#endif
            break;

        /* Network level authentication */
        case GUAC_SECURITY_NLA:
#ifdef LEGACY_RDPSETTINGS
            rdp_settings->rdp_security = FALSE;
            rdp_settings->tls_security = FALSE;
            rdp_settings->nla_security = TRUE;
#else
            rdp_settings->RdpSecurity = FALSE;
            rdp_settings->TlsSecurity = FALSE;
            rdp_settings->NlaSecurity = TRUE;
#endif
            break;

        /* All security types */
        case GUAC_SECURITY_ANY:
#ifdef LEGACY_RDPSETTINGS
            rdp_settings->rdp_security = TRUE;
            rdp_settings->tls_security = TRUE;
            rdp_settings->nla_security = TRUE;
#else
            rdp_settings->RdpSecurity = TRUE;
            rdp_settings->TlsSecurity = TRUE;
            rdp_settings->NlaSecurity = TRUE;
#endif
            break;

    }

    /* Authentication */
#ifdef LEGACY_RDPSETTINGS
    rdp_settings->authentication = !guac_settings->disable_authentication;
    rdp_settings->ignore_certificate = guac_settings->ignore_certificate;
    rdp_settings->encryption = TRUE;
#else
    rdp_settings->Authentication = !guac_settings->disable_authentication;
    rdp_settings->IgnoreCertificate = guac_settings->ignore_certificate;
    rdp_settings->DisableEncryption = FALSE;
#endif

    /* RemoteApp */
    if (guac_settings->remote_app != NULL) {
#ifdef LEGACY_RDPSETTINGS
        rdp_settings->workarea = TRUE;
        rdp_settings->remote_app = TRUE;
        rdp_settings->rail_langbar_supported = TRUE;
#else
        rdp_settings->Workarea = TRUE;
        rdp_settings->RemoteApplicationMode = TRUE;
        rdp_settings->RemoteAppLanguageBarSupported = TRUE;
        rdp_settings->RemoteApplicationProgram = guac_settings->remote_app;
        rdp_settings->ShellWorkingDirectory = guac_settings->remote_app_dir;
        rdp_settings->RemoteApplicationCmdLine = guac_settings->remote_app_args;
#endif
    }

#ifdef HAVE_RDPSETTINGS_SENDPRECONNECTIONPDU
    /* Preconnection ID */
    if (guac_settings->preconnection_id != -1) {
        rdp_settings->NegotiateSecurityLayer = FALSE;
        rdp_settings->SendPreconnectionPdu = TRUE;
        rdp_settings->PreconnectionId = guac_settings->preconnection_id;
    }

    /* Preconnection BLOB */
    if (guac_settings->preconnection_blob != NULL) {
        rdp_settings->NegotiateSecurityLayer = FALSE;
        rdp_settings->SendPreconnectionPdu = TRUE;
        rdp_settings->PreconnectionBlob = guac_settings->preconnection_blob;
    }
#endif

    /* Order support */
#ifdef LEGACY_RDPSETTINGS
    bitmap_cache = rdp_settings->bitmap_cache;
    rdp_settings->os_major_type = OSMAJORTYPE_UNSPECIFIED;
    rdp_settings->os_minor_type = OSMINORTYPE_UNSPECIFIED;
    rdp_settings->desktop_resize = TRUE;
    rdp_settings->order_support[NEG_DSTBLT_INDEX] = TRUE;
    rdp_settings->order_support[NEG_PATBLT_INDEX] = FALSE; /* PATBLT not yet supported */
    rdp_settings->order_support[NEG_SCRBLT_INDEX] = TRUE;
    rdp_settings->order_support[NEG_OPAQUE_RECT_INDEX] = TRUE;
    rdp_settings->order_support[NEG_DRAWNINEGRID_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MULTIDSTBLT_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MULTIPATBLT_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MULTISCRBLT_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MULTIOPAQUERECT_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MULTI_DRAWNINEGRID_INDEX] = FALSE;
    rdp_settings->order_support[NEG_LINETO_INDEX] = FALSE;
    rdp_settings->order_support[NEG_POLYLINE_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MEMBLT_INDEX] = bitmap_cache;
    rdp_settings->order_support[NEG_MEM3BLT_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MEMBLT_V2_INDEX] = bitmap_cache;
    rdp_settings->order_support[NEG_MEM3BLT_V2_INDEX] = FALSE;
    rdp_settings->order_support[NEG_SAVEBITMAP_INDEX] = FALSE;
    rdp_settings->order_support[NEG_GLYPH_INDEX_INDEX] = TRUE;
    rdp_settings->order_support[NEG_FAST_INDEX_INDEX] = TRUE;
    rdp_settings->order_support[NEG_FAST_GLYPH_INDEX] = TRUE;
    rdp_settings->order_support[NEG_POLYGON_SC_INDEX] = FALSE;
    rdp_settings->order_support[NEG_POLYGON_CB_INDEX] = FALSE;
    rdp_settings->order_support[NEG_ELLIPSE_SC_INDEX] = FALSE;
    rdp_settings->order_support[NEG_ELLIPSE_CB_INDEX] = FALSE;
#else
    bitmap_cache = rdp_settings->BitmapCacheEnabled;
    rdp_settings->OsMajorType = OSMAJORTYPE_UNSPECIFIED;
    rdp_settings->OsMinorType = OSMINORTYPE_UNSPECIFIED;
    rdp_settings->DesktopResize = TRUE;
    rdp_settings->OrderSupport[NEG_DSTBLT_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_PATBLT_INDEX] = FALSE; /* PATBLT not yet supported */
    rdp_settings->OrderSupport[NEG_SCRBLT_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_OPAQUE_RECT_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_DRAWNINEGRID_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MULTIDSTBLT_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MULTIPATBLT_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MULTISCRBLT_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MULTIOPAQUERECT_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MULTI_DRAWNINEGRID_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_LINETO_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_POLYLINE_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MEMBLT_INDEX] = bitmap_cache;
    rdp_settings->OrderSupport[NEG_MEM3BLT_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MEMBLT_V2_INDEX] = bitmap_cache;
    rdp_settings->OrderSupport[NEG_MEM3BLT_V2_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_SAVEBITMAP_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_GLYPH_INDEX_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_FAST_INDEX_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_FAST_GLYPH_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_POLYGON_SC_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_POLYGON_CB_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_ELLIPSE_SC_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_ELLIPSE_CB_INDEX] = FALSE;
#endif

}

