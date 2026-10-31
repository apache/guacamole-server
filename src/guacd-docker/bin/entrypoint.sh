#!/bin/sh

# Warn about deprecation of GUACD_LOG_LEVEL
if [ -n "$GUACD_LOG_LEVEL" ]; then
    echo "WARNING: The GUACD_LOG_LEVEL environment variable has been deprecated in favor of the LOG_LEVEL environment variable. Please migrate your configuration when possible." >&2
fi

# Listen on 0.0.0.0:4822, logging messages at the info level. Allow log level
# to be overridden with LOG_LEVEL, the RDP drive open-file limit to be
# overridden with RDP_DRIVE_MAX_FILES, and other behavior to be overridden
# with additional command-line options passed to Docker. Options passed to
# Docker are applied last, so they override the environment variables.
set -- -f -b 0.0.0.0 -L "${LOG_LEVEL:-${GUACD_LOG_LEVEL:-info}}" "$@"

if [ -n "$RDP_DRIVE_MAX_FILES" ]; then
    set -- -M "$RDP_DRIVE_MAX_FILES" "$@"
fi

exec /opt/guacamole/sbin/guacd "$@"
