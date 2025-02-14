#!/bin/sh

# Listen on 0.0.0.0:4822, logging messages at the info level. Allow log level
# to be overridden with GUACD_LOG_LEVEL, and other behavior to be overridden
# with additional command-line options passed to Docker.
exec /opt/guacamole/sbin/guacd -f -b 0.0.0.0 -L "${GUACD_LOG_LEVEL:-info}" "$@"
