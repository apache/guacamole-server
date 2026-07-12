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

#include "config.h"

#include "argv.h"
#include "settings.h"
#include "terminal/terminal.h"

#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <guacamole/user.h>

#include <freeipmi/freeipmi.h>
#include <ipmiconsole.h>

#include <sys/types.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Client plugin arguments */
const char* GUAC_IPMI_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "timeout",
    "username",
    "password",
    "k-g",
    "privilege-level",
    "cipher-suite",
    "encryption-policy",
    "workaround-flags",
    "sol-payload-instance",
    "power-on-connect",
    "boot-device",
    GUAC_IPMI_ARGV_FONT_NAME,
    GUAC_IPMI_ARGV_FONT_SIZE,
    GUAC_IPMI_ARGV_COLOR_SCHEME,
    "typescript-path",
    "typescript-name",
    "create-typescript-path",
    "typescript-write-existing",
    "recording-path",
    "recording-name",
    "recording-exclude-output",
    "recording-exclude-mouse",
    "recording-include-keys",
    "recording-include-clipboard",
    "create-recording-path",
    "recording-write-existing",
    "read-only",
    "backspace",
    "terminal-type",
    "scrollback",
    "disable-copy",
    "disable-paste",
    "boot-device-persistent",
    "keepalive-interval",
    NULL
};

enum IPMI_ARGS_IDX {

    /**
     * The hostname or IP address of the BMC to connect to. Required.
     */
    IDX_HOSTNAME,

    /**
     * The RMCP port. Informational only; libipmiconsole uses 623/UDP.
     */
    IDX_PORT,

    /**
     * The IPMI session timeout, in seconds. Optional.
     */
    IDX_TIMEOUT,

    /**
     * The username to authenticate with. Optional.
     */
    IDX_USERNAME,

    /**
     * The password to authenticate with. Optional.
     */
    IDX_PASSWORD,

    /**
     * The BMC key (K_g) for two-key authentication. Optional.
     */
    IDX_K_G,

    /**
     * The privilege level to authenticate with ("user", "operator", or
     * "admin"). Optional; defaults to "admin".
     */
    IDX_PRIVILEGE_LEVEL,

    /**
     * The RMCP+ cipher suite ID. Optional; defaults to 3.
     */
    IDX_CIPHER_SUITE,

    /**
     * The encryption policy governing whether a non-confidential cipher suite
     * is permitted ("required", "preferred", or "none"). Optional; defaults to
     * "required".
     */
    IDX_ENCRYPTION_POLICY,

    /**
     * A comma-separated list of FreeIPMI workaround flags and/or vendor preset
     * names to apply to the SOL and chassis sessions. Optional; defaults to
     * none.
     */
    IDX_WORKAROUND_FLAGS,

    /**
     * The SOL payload instance (1-15). Optional; defaults to 1.
     */
    IDX_SOL_PAYLOAD_INSTANCE,

    /**
     * The chassis power action to perform once connected, prior to attaching
     * the console ("none", "on", "reset", or "cycle"). Optional; defaults to
     * "none".
     */
    IDX_POWER_ON_CONNECT,

    /**
     * The boot device override to apply for the next boot ("none", "pxe",
     * "disk", "cdrom", or "bios"). Optional; defaults to "none".
     */
    IDX_BOOT_DEVICE,

    /**
     * The name of the font to use within the terminal.
     */
    IDX_FONT_NAME,

    /**
     * The size of the font to use within the terminal, in points.
     */
    IDX_FONT_SIZE,

    /**
     * The color scheme to use, as accepted by the terminal emulator.
     */
    IDX_COLOR_SCHEME,

    /**
     * The full absolute path to the directory in which typescripts should be
     * written.
     */
    IDX_TYPESCRIPT_PATH,

    /**
     * The name that should be given to typescripts which are written in the
     * given path.
     */
    IDX_TYPESCRIPT_NAME,

    /**
     * Whether the specified typescript path should automatically be created if
     * it does not yet exist.
     */
    IDX_CREATE_TYPESCRIPT_PATH,

    /**
     * Whether existing files should be appended to when creating a new
     * typescript.
     */
    IDX_TYPESCRIPT_WRITE_EXISTING,

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
     * Whether broadcast output should NOT be included in the session
     * recording.
     */
    IDX_RECORDING_EXCLUDE_OUTPUT,

    /**
     * Whether mouse state should NOT be included in the session recording.
     */
    IDX_RECORDING_EXCLUDE_MOUSE,

    /**
     * Whether keys pressed and released should be included in the session
     * recording.
     */
    IDX_RECORDING_INCLUDE_KEYS,

    /**
     * Whether clipboard data should be included in the session recording.
     * Clipboard data is NOT included by default within the recording,
     * as doing so has privacy and security implications. Including clipboard data
     * may be necessary in certain auditing contexts, but should only be done
     * with caution. Clipboard data can easily contain sensitive information, such
     * as passwords, credit card numbers, etc.
     */
    IDX_RECORDING_INCLUDE_CLIPBOARD,

    /**
     * Whether the specified screen recording path should automatically be
     * created if it does not yet exist.
     */
    IDX_CREATE_RECORDING_PATH,

    /**
     * Whether existing files should be appended to when creating a new
     * recording.
     */
    IDX_RECORDING_WRITE_EXISTING,

    /**
     * "true" if this connection should be read-only, "false" or blank
     * otherwise.
     */
    IDX_READ_ONLY,

    /**
     * ASCII code, as an integer, to use for the backspace key.
     */
    IDX_BACKSPACE,

    /**
     * The terminal emulator type that is exposed to the remote system.
     */
    IDX_TERMINAL_TYPE,

    /**
     * The maximum size of the scrollback buffer in rows.
     */
    IDX_SCROLLBACK,

    /**
     * Whether outbound clipboard access should be blocked.
     */
    IDX_DISABLE_COPY,

    /**
     * Whether inbound clipboard access should be blocked.
     */
    IDX_DISABLE_PASTE,

    /**
     * Whether a configured boot device override should persist across
     * subsequent boots rather than applying only to the next boot. Optional;
     * defaults to false (next boot only).
     */
    IDX_BOOT_DEVICE_PERSISTENT,

    /**
     * The interval, in seconds, at which SOL keepalive packets are sent. Zero
     * uses the libipmiconsole default. Optional.
     */
    IDX_KEEPALIVE_INTERVAL,

    IPMI_ARGS_COUNT
};

/**
 * Parses the given privilege level string into the corresponding
 * guac_ipmi_privilege_level value, logging a warning and falling back to the
 * default (admin) for unrecognized values.
 *
 * @param user
 *     The user who provided the value, on whose behalf warnings are logged.
 *
 * @param value
 *     The privilege level string ("user", "operator", or "admin").
 *
 * @return
 *     The parsed privilege level, or GUAC_IPMI_PRIVILEGE_ADMIN if the value is
 *     blank or unrecognized.
 */
static guac_ipmi_privilege_level guac_ipmi_parse_privilege_level(
        guac_user* user, const char* value) {

    if (value == NULL || strcmp(value, "") == 0
            || strcasecmp(value, "admin") == 0
            || strcasecmp(value, "administrator") == 0)
        return GUAC_IPMI_PRIVILEGE_ADMIN;

    if (strcasecmp(value, "operator") == 0)
        return GUAC_IPMI_PRIVILEGE_OPERATOR;

    if (strcasecmp(value, "user") == 0)
        return GUAC_IPMI_PRIVILEGE_USER;

    guac_user_log(user, GUAC_LOG_WARNING, "Unrecognized privilege level "
            "\"%s\"; defaulting to \"admin\".", value);
    return GUAC_IPMI_PRIVILEGE_ADMIN;

}

/**
 * Parses the given power action string into the corresponding
 * guac_ipmi_power_action value. Only the actions appropriate for an automatic
 * "on connect" action are accepted ("none", "on", "reset", "cycle").
 *
 * @param user
 *     The user who provided the value, on whose behalf warnings are logged.
 *
 * @param value
 *     The power action string.
 *
 * @return
 *     The parsed power action, or GUAC_IPMI_POWER_NONE if the value is blank
 *     or unrecognized.
 */
static guac_ipmi_power_action guac_ipmi_parse_power_action(
        guac_user* user, const char* value) {

    if (value == NULL || strcmp(value, "") == 0
            || strcasecmp(value, "none") == 0)
        return GUAC_IPMI_POWER_NONE;

    if (strcasecmp(value, "on") == 0)
        return GUAC_IPMI_POWER_ON;

    if (strcasecmp(value, "reset") == 0)
        return GUAC_IPMI_POWER_RESET;

    if (strcasecmp(value, "cycle") == 0)
        return GUAC_IPMI_POWER_CYCLE;

    guac_user_log(user, GUAC_LOG_WARNING, "Unrecognized power-on-connect "
            "action \"%s\"; no power action will be taken.", value);
    return GUAC_IPMI_POWER_NONE;

}

/**
 * Parses the given boot device string into the corresponding
 * guac_ipmi_boot_device value.
 *
 * @param user
 *     The user who provided the value, on whose behalf warnings are logged.
 *
 * @param value
 *     The boot device string ("none", "pxe", "disk", "cdrom", or "bios").
 *
 * @return
 *     The parsed boot device, or GUAC_IPMI_BOOT_NONE if the value is blank or
 *     unrecognized.
 */
static guac_ipmi_boot_device guac_ipmi_parse_boot_device(
        guac_user* user, const char* value) {

    if (value == NULL || strcmp(value, "") == 0
            || strcasecmp(value, "none") == 0)
        return GUAC_IPMI_BOOT_NONE;

    if (strcasecmp(value, "pxe") == 0)
        return GUAC_IPMI_BOOT_PXE;

    if (strcasecmp(value, "disk") == 0 || strcasecmp(value, "hd") == 0)
        return GUAC_IPMI_BOOT_DISK;

    if (strcasecmp(value, "cdrom") == 0 || strcasecmp(value, "cd") == 0)
        return GUAC_IPMI_BOOT_CDROM;

    if (strcasecmp(value, "bios") == 0 || strcasecmp(value, "setup") == 0)
        return GUAC_IPMI_BOOT_BIOS;

    guac_user_log(user, GUAC_LOG_WARNING, "Unrecognized boot device "
            "\"%s\"; boot order will be left unchanged.", value);
    return GUAC_IPMI_BOOT_NONE;

}

/**
 * Parses the given string into a guac_ipmi_encryption_policy. Unrecognized
 * values fall back to the (secure) default of "required".
 */
static guac_ipmi_encryption_policy guac_ipmi_parse_encryption_policy(
        guac_user* user, const char* value) {

    if (value == NULL || strcmp(value, "") == 0
            || strcasecmp(value, "required") == 0)
        return GUAC_IPMI_ENCRYPTION_REQUIRED;

    if (strcasecmp(value, "preferred") == 0)
        return GUAC_IPMI_ENCRYPTION_PREFERRED;

    if (strcasecmp(value, "none") == 0)
        return GUAC_IPMI_ENCRYPTION_NONE;

    guac_user_log(user, GUAC_LOG_WARNING, "Unrecognized encryption policy "
            "\"%s\"; requiring encryption.", value);
    return GUAC_IPMI_ENCRYPTION_REQUIRED;

}

int guac_ipmi_cipher_provides_confidentiality(int cipher_suite) {

    /* IPMI 2.0 RMCP+ cipher suites whose confidentiality algorithm is NOT
     * "none" (i.e. the payload is encrypted). Suites 0, 1, 2, 6, 7, 11, 15,
     * and 16 provide no confidentiality and are therefore excluded. */
    switch (cipher_suite) {
        case 3:  case 4:  case 5:
        case 8:  case 9:  case 10:
        case 12: case 13: case 14:
        case 17: case 18: case 19:
            return 1;
        default:
            return 0;
    }

}

/**
 * A single FreeIPMI workaround flag, mapping a user-facing token to the
 * corresponding libipmiconsole (SOL) and libfreeipmi (chassis) flag bits. A
 * chassis bit of 0 indicates the flag has no equivalent in the chassis
 * (out-of-band 2.0) namespace and applies to the SOL session only.
 */
typedef struct guac_ipmi_workaround_token {
    const char* name;
    unsigned int sol_flag;
    unsigned int chassis_flag;
} guac_ipmi_workaround_token;

static const guac_ipmi_workaround_token GUAC_IPMI_WORKAROUND_TOKENS[] = {
    { "authcap",              IPMICONSOLE_WORKAROUND_AUTHENTICATION_CAPABILITIES,     IPMI_WORKAROUND_FLAGS_OUTOFBAND_2_0_AUTHENTICATION_CAPABILITIES },
    { "intel20",              IPMICONSOLE_WORKAROUND_INTEL_2_0_SESSION,               IPMI_WORKAROUND_FLAGS_OUTOFBAND_2_0_INTEL_2_0_SESSION },
    { "supermicro20",         IPMICONSOLE_WORKAROUND_SUPERMICRO_2_0_SESSION,          IPMI_WORKAROUND_FLAGS_OUTOFBAND_2_0_SUPERMICRO_2_0_SESSION },
    { "sun20",                IPMICONSOLE_WORKAROUND_SUN_2_0_SESSION,                 IPMI_WORKAROUND_FLAGS_OUTOFBAND_2_0_SUN_2_0_SESSION },
    { "opensesspriv",         IPMICONSOLE_WORKAROUND_OPEN_SESSION_PRIVILEGE,          IPMI_WORKAROUND_FLAGS_OUTOFBAND_2_0_OPEN_SESSION_PRIVILEGE },
    { "integritycheckvalue",  IPMICONSOLE_WORKAROUND_NON_EMPTY_INTEGRITY_CHECK_VALUE, IPMI_WORKAROUND_FLAGS_OUTOFBAND_2_0_NON_EMPTY_INTEGRITY_CHECK_VALUE },
    { "nochecksumcheck",      IPMICONSOLE_WORKAROUND_NO_CHECKSUM_CHECK,               IPMI_WORKAROUND_FLAGS_OUTOFBAND_2_0_NO_CHECKSUM_CHECK },
    { "serialalertsdeferred", IPMICONSOLE_WORKAROUND_SERIAL_ALERTS_DEFERRED,          0 },
    { "solpacketseq",         IPMICONSOLE_WORKAROUND_INCREMENT_SOL_PACKET_SEQUENCE,   0 },
    { "solpayloadsize",       IPMICONSOLE_WORKAROUND_IGNORE_SOL_PAYLOAD_SIZE,         0 },
    { "solport",              IPMICONSOLE_WORKAROUND_IGNORE_SOL_PORT,                 0 },
    { "solstatus",            IPMICONSOLE_WORKAROUND_SKIP_SOL_ACTIVATION_STATUS,      0 },
    { "channelpayload",       IPMICONSOLE_WORKAROUND_SKIP_CHANNEL_PAYLOAD_SUPPORT,    0 }
};

/**
 * A vendor preset expanding to a set of individual workaround tokens known to
 * be commonly required for that vendor's BMCs.
 */
typedef struct guac_ipmi_workaround_preset {
    const char* name;
    const char* tokens;
} guac_ipmi_workaround_preset;

static const guac_ipmi_workaround_preset GUAC_IPMI_WORKAROUND_PRESETS[] = {
    { "none",       "" },
    { "supermicro", "supermicro20,opensesspriv,integritycheckvalue,solpayloadsize,solport,solstatus" },
    { "intel",      "intel20,opensesspriv,integritycheckvalue" },
    { "sun",        "sun20,authcap,opensesspriv,solpayloadsize" },
    { "dell",       "opensesspriv,solpayloadsize" },

    /* Modern HPE iLO and Lenovo XCC generations generally require no
     * workarounds; these presets are provided for discoverability and may be
     * combined with explicit tokens for older firmware. */
    { "hpe",        "" },
    { "lenovo",     "" }
};

/**
 * Applies a single individual workaround flag token (not a preset) to the
 * given SOL and chassis masks. Returns non-zero if the token was recognized.
 */
static int guac_ipmi_apply_workaround_token(const char* token,
        unsigned int* sol, unsigned int* chassis) {

    int count = sizeof(GUAC_IPMI_WORKAROUND_TOKENS)
            / sizeof(GUAC_IPMI_WORKAROUND_TOKENS[0]);

    for (int i = 0; i < count; i++) {
        if (strcasecmp(token, GUAC_IPMI_WORKAROUND_TOKENS[i].name) == 0) {
            *sol     |= GUAC_IPMI_WORKAROUND_TOKENS[i].sol_flag;
            *chassis |= GUAC_IPMI_WORKAROUND_TOKENS[i].chassis_flag;
            return 1;
        }
    }

    return 0;

}

/**
 * Parses a comma-separated list of workaround flag tokens and/or vendor preset
 * names, accumulating the resulting flags into the given SOL (libipmiconsole)
 * and chassis (libfreeipmi) masks. Both masks are cleared to 0 first.
 */
static void guac_ipmi_parse_workaround_flags(guac_user* user,
        const char* value, unsigned int* sol, unsigned int* chassis) {

    *sol = 0;
    *chassis = 0;

    if (value == NULL || strcmp(value, "") == 0)
        return;

    int preset_count = sizeof(GUAC_IPMI_WORKAROUND_PRESETS)
            / sizeof(GUAC_IPMI_WORKAROUND_PRESETS[0]);

    /* Tokenize a mutable copy of the value on commas */
    char* copy = guac_strdup(value);
    char* saveptr = NULL;
    char* token = strtok_r(copy, ",", &saveptr);

    while (token != NULL) {

        /* Trim leading whitespace */
        while (*token == ' ' || *token == '\t')
            token++;

        /* Trim trailing whitespace */
        char* end = token + strlen(token);
        while (end > token && (end[-1] == ' ' || end[-1] == '\t'))
            *(--end) = '\0';

        if (*token != '\0') {

            /* Check for a vendor preset first */
            int matched = 0;
            for (int i = 0; i < preset_count; i++) {
                if (strcasecmp(token, GUAC_IPMI_WORKAROUND_PRESETS[i].name) == 0) {

                    /* Expand the preset's individual tokens */
                    char* preset_copy =
                        guac_strdup(GUAC_IPMI_WORKAROUND_PRESETS[i].tokens);
                    char* preset_save = NULL;
                    char* sub = strtok_r(preset_copy, ",", &preset_save);
                    while (sub != NULL) {
                        guac_ipmi_apply_workaround_token(sub, sol, chassis);
                        sub = strtok_r(NULL, ",", &preset_save);
                    }
                    guac_mem_free(preset_copy);

                    matched = 1;
                    break;
                }
            }

            /* Otherwise treat as an individual flag token */
            if (!matched && !guac_ipmi_apply_workaround_token(token, sol, chassis))
                guac_user_log(user, GUAC_LOG_WARNING, "Unrecognized IPMI "
                        "workaround flag \"%s\"; ignoring.", token);

        }

        token = strtok_r(NULL, ",", &saveptr);
    }

    guac_mem_free(copy);

}

/**
 * The maximum length of an IPMI 2.0 K_g (BMC) key, in bytes.
 */
#define GUAC_IPMI_K_G_MAX_LENGTH 20

/**
 * Parses the raw K_g argument into a newly-allocated byte buffer, returning
 * the buffer and storing its length in *length. If the value begins with "0x",
 * it is decoded as a hexadecimal byte string (allowing binary keys that may
 * contain NUL bytes); otherwise it is treated as a raw passphrase. Returns NULL
 * (with *length set to 0) if the value is NULL or invalid.
 */
static char* guac_ipmi_parse_k_g(guac_user* user, const char* raw,
        int* length) {

    *length = 0;

    if (raw == NULL || *raw == '\0')
        return NULL;

    /* Hexadecimal form: "0x..." decodes to arbitrary bytes */
    if (raw[0] == '0' && (raw[1] == 'x' || raw[1] == 'X')) {

        const char* hex = raw + 2;
        size_t hex_len = strlen(hex);

        if (hex_len == 0 || (hex_len % 2) != 0) {
            guac_user_log(user, GUAC_LOG_WARNING, "Hexadecimal K_g key must "
                    "have an even, non-zero number of digits; ignoring.");
            return NULL;
        }

        int bytes = (int) (hex_len / 2);
        if (bytes > GUAC_IPMI_K_G_MAX_LENGTH)
            guac_user_log(user, GUAC_LOG_WARNING, "K_g key exceeds the IPMI "
                    "maximum of %i bytes; the BMC may reject it.",
                    GUAC_IPMI_K_G_MAX_LENGTH);

        char* buffer = guac_mem_alloc(bytes);
        for (int i = 0; i < bytes; i++) {
            char nibbles[3] = { hex[i * 2], hex[i * 2 + 1], '\0' };
            if (!isxdigit((unsigned char) nibbles[0])
                    || !isxdigit((unsigned char) nibbles[1])) {
                guac_user_log(user, GUAC_LOG_WARNING, "K_g key contains an "
                        "invalid hexadecimal digit; ignoring.");
                guac_mem_free(buffer);
                return NULL;
            }
            buffer[i] = (char) strtol(nibbles, NULL, 16);
        }

        *length = bytes;
        return buffer;

    }

    /* Passphrase form: the raw bytes of the string */
    size_t raw_len = strlen(raw);
    if (raw_len > GUAC_IPMI_K_G_MAX_LENGTH)
        guac_user_log(user, GUAC_LOG_WARNING, "K_g key exceeds the IPMI "
                "maximum of %i bytes; the BMC may reject it.",
                GUAC_IPMI_K_G_MAX_LENGTH);

    char* buffer = guac_mem_alloc(raw_len);
    memcpy(buffer, raw, raw_len);
    *length = (int) raw_len;
    return buffer;

}

guac_ipmi_settings* guac_ipmi_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != IPMI_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                IPMI_ARGS_COUNT, argc);
        return NULL;
    }

    guac_ipmi_settings* settings = guac_mem_zalloc(sizeof(guac_ipmi_settings));

    /* Read hostname */
    settings->hostname =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_HOSTNAME, "");

    /* Read port (informational only) */
    settings->port =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_PORT, GUAC_IPMI_DEFAULT_PORT);

    /* Warn if a non-default port was requested, as it cannot be honored */
    if (strcmp(settings->port, GUAC_IPMI_DEFAULT_PORT) != 0)
        guac_user_log(user, GUAC_LOG_INFO, "A non-default IPMI port (\"%s\") "
                "was specified, but IPMI out-of-band access always uses "
                "623/UDP. The provided value will be ignored.", settings->port);

    /* Read IPMI session timeout */
    settings->timeout =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_TIMEOUT, GUAC_IPMI_DEFAULT_TIMEOUT);

    /* Read SOL keepalive interval (0 = libipmiconsole default) */
    settings->keepalive_interval =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_KEEPALIVE_INTERVAL, 0);

    /* Read credentials */
    settings->username =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_USERNAME, NULL);

    settings->password =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_PASSWORD, NULL);

    /* Read the BMC key (K_g), decoding a "0x..." value as a raw byte string so
     * that binary keys containing NUL bytes are preserved. */
    char* raw_k_g =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_K_G, NULL);
    settings->k_g = guac_ipmi_parse_k_g(user, raw_k_g, &settings->k_g_length);
    guac_mem_free(raw_k_g);

    /* Read privilege level */
    settings->privilege_level = guac_ipmi_parse_privilege_level(user,
            guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                    IDX_PRIVILEGE_LEVEL, "admin"));

    /* Read cipher suite */
    settings->cipher_suite =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_CIPHER_SUITE, GUAC_IPMI_DEFAULT_CIPHER_SUITE);

    /* Warn strongly against the use of cipher suite 0 (no security) */
    if (settings->cipher_suite == 0)
        guac_user_log(user, GUAC_LOG_WARNING, "Cipher suite 0 provides NO "
                "authentication, integrity, or confidentiality. Its use is "
                "strongly discouraged.");

    /* Read encryption policy */
    settings->encryption_policy = guac_ipmi_parse_encryption_policy(user,
            guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                    IDX_ENCRYPTION_POLICY, "required"));

    /* Read vendor workaround flags, resolving into separate SOL and chassis
     * masks */
    char* workaround_value =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_WORKAROUND_FLAGS, NULL);
    guac_ipmi_parse_workaround_flags(user, workaround_value,
            &settings->sol_workaround_flags,
            &settings->chassis_workaround_flags);
    guac_mem_free(workaround_value);

    /* Read SOL payload instance */
    settings->sol_payload_instance =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_SOL_PAYLOAD_INSTANCE, GUAC_IPMI_DEFAULT_SOL_PAYLOAD_INSTANCE);

    /* Read pre-connect power and boot actions */
    settings->power_on_connect = guac_ipmi_parse_power_action(user,
            guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                    IDX_POWER_ON_CONNECT, "none"));

    settings->boot_device = guac_ipmi_parse_boot_device(user,
            guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                    IDX_BOOT_DEVICE, "none"));

    settings->boot_persistent =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_BOOT_DEVICE_PERSISTENT, false);

    /* Read-only mode */
    settings->read_only =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_READ_ONLY, false);

    /* Read maximum scrollback size */
    settings->max_scrollback =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_SCROLLBACK, GUAC_TERMINAL_DEFAULT_MAX_SCROLLBACK);

    /* Read font name */
    settings->font_name =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_FONT_NAME, GUAC_TERMINAL_DEFAULT_FONT_NAME);

    /* Read font size */
    settings->font_size =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_FONT_SIZE, GUAC_TERMINAL_DEFAULT_FONT_SIZE);

    /* Copy requested color scheme */
    settings->color_scheme =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_COLOR_SCHEME, GUAC_TERMINAL_DEFAULT_COLOR_SCHEME);

    /* Pull width/height/resolution directly from user */
    settings->width      = user->info.optimal_width;
    settings->height     = user->info.optimal_height;
    settings->resolution = user->info.optimal_resolution;

    /* Read typescript path */
    settings->typescript_path =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_PATH, NULL);

    /* Read typescript name */
    settings->typescript_name =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_NAME, GUAC_IPMI_DEFAULT_TYPESCRIPT_NAME);

    /* Parse path creation flag */
    settings->create_typescript_path =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_CREATE_TYPESCRIPT_PATH, false);

    /* Parse allow write existing file flag */
    settings->typescript_write_existing =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_WRITE_EXISTING, false);

    /* Read recording path */
    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    /* Read recording name */
    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_IPMI_DEFAULT_RECORDING_NAME);

    /* Parse output exclusion flag */
    settings->recording_exclude_output =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_OUTPUT, false);

    /* Parse mouse exclusion flag */
    settings->recording_exclude_mouse =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_MOUSE, false);

    /* Parse key event inclusion flag */
    settings->recording_include_keys =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_KEYS, false);

    /* Parse clipboard inclusion flag */
    settings->recording_include_clipboard =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_CLIPBOARD, false);

    /* Parse path creation flag */
    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, false);

    /* Parse allow write existing file flag */
    settings->recording_write_existing =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_WRITE_EXISTING, false);

    /* Parse backspace key code */
    settings->backspace =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_BACKSPACE, GUAC_TERMINAL_DEFAULT_BACKSPACE);

    /* Read terminal emulator type */
    settings->terminal_type =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_TERMINAL_TYPE, "linux");

    /* Parse clipboard copy disable flag */
    settings->disable_copy =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_DISABLE_COPY, false);

    /* Parse clipboard paste disable flag */
    settings->disable_paste =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_DISABLE_PASTE, false);

    /* Parsing was successful */
    return settings;

}

void guac_ipmi_settings_free(guac_ipmi_settings* settings) {

    /* Free network connection information */
    guac_mem_free(settings->hostname);
    guac_mem_free(settings->port);

    /* Free credentials, scrubbing the sensitive BMC secrets from memory before
     * releasing them so plaintext does not linger in freed heap. */
    guac_mem_free(settings->username);
    if (settings->password != NULL)
        explicit_bzero(settings->password, strlen(settings->password));
    guac_mem_free(settings->password);
    if (settings->k_g != NULL)
        explicit_bzero(settings->k_g, settings->k_g_length);
    guac_mem_free(settings->k_g);

    /* Free display preferences */
    guac_mem_free(settings->font_name);
    guac_mem_free(settings->color_scheme);

    /* Free typescript settings */
    guac_mem_free(settings->typescript_name);
    guac_mem_free(settings->typescript_path);

    /* Free screen recording settings */
    guac_mem_free(settings->recording_name);
    guac_mem_free(settings->recording_path);

    /* Free terminal emulator type */
    guac_mem_free(settings->terminal_type);

    /* Free overall structure */
    guac_mem_free(settings);

}
