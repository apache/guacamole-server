#!/bin/sh

# Warn about deprecation of GUACD_LOG_LEVEL
if [ -n "$GUACD_LOG_LEVEL" ]; then
    echo "WARNING: The GUACD_LOG_LEVEL environment variable has been deprecated in favor of the LOG_LEVEL environment variable. Please migrate your configuration when possible." >&2
fi

# Start PC/SC daemon
pcscd --disable-polkit &

# Start virtual card emulator (background)
/opt/virtualsmartcard-0.10/src/vpicc/vicc --reader 0 &

# Listen on 0.0.0.0:4822, logging messages at the info level. Allow log level
# to be overridden with LOG_LEVEL, and other behavior to be overridden with
# additional command-line options passed to Docker.
exec /opt/guacamole/sbin/guacd -f -b 0.0.0.0 -L "${LOG_LEVEL:-${GUACD_LOG_LEVEL:-info}}" "$@"
