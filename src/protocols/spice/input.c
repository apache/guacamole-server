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

#include "input.h"
#include "keyboard.h"
#include "spice.h"

#include <guacamole/client.h>
#include <guacamole/client-constants.h>
#include <guacamole/display.h>
#include <guacamole/recording.h>
#include <guacamole/user.h>
#include <spice-client.h>
#include <spice/vd_agent.h>

/**
 * Translates a Guacamole mouse button mask into the equivalent SPICE button
 * state mask (a bitwise OR of SPICE_MOUSE_BUTTON_MASK_* values). Only the
 * primary three buttons are represented within a SPICE button state mask;
 * scroll events are delivered separately as momentary button presses.
 *
 * @param mask
 *     The Guacamole mouse button mask to translate.
 *
 * @return
 *     The equivalent SPICE button state mask.
 */
static int guac_spice_button_mask(int mask) {

    int spice_mask = 0;

    if (mask & GUAC_CLIENT_MOUSE_LEFT)
        spice_mask |= SPICE_MOUSE_BUTTON_MASK_LEFT;

    if (mask & GUAC_CLIENT_MOUSE_MIDDLE)
        spice_mask |= SPICE_MOUSE_BUTTON_MASK_MIDDLE;

    if (mask & GUAC_CLIENT_MOUSE_RIGHT)
        spice_mask |= SPICE_MOUSE_BUTTON_MASK_RIGHT;

    return spice_mask;

}

/**
 * Sends a momentary button press and release for the given SPICE button,
 * relative to the given current button state mask. This is used for scroll
 * wheel events, which Guacamole delivers as transient button presses.
 *
 * @param inputs
 *     The SPICE inputs channel through which the events should be sent.
 *
 * @param button
 *     The SPICE button to press and release (one of SPICE_MOUSE_BUTTON_*).
 *
 * @param button_state
 *     The current SPICE button state mask.
 */
/* Deferred handlers, run on the SPICE event-loop thread. spice-gtk is not
 * thread-safe, so mouse events generated on Guacamole user threads must be
 * marshalled onto the loop thread via guac_spice_defer_call(). */

static void guac_spice_do_button_press(guac_spice_deferred_call* call) {
    spice_inputs_channel_button_press((SpiceInputsChannel*) call->channel,
            (int) call->args[0], (int) call->args[1]);
}

static void guac_spice_do_button_release(guac_spice_deferred_call* call) {
    spice_inputs_channel_button_release((SpiceInputsChannel*) call->channel,
            (int) call->args[0], (int) call->args[1]);
}

static void guac_spice_do_motion(guac_spice_deferred_call* call) {
    spice_inputs_channel_motion((SpiceInputsChannel*) call->channel,
            (int) call->args[0], (int) call->args[1], (int) call->args[2]);
}

static void guac_spice_do_position(guac_spice_deferred_call* call) {
    spice_inputs_channel_position((SpiceInputsChannel*) call->channel,
            (int) call->args[0], (int) call->args[1], (int) call->args[2],
            (int) call->args[3]);
}

/**
 * Marshals a SPICE mouse button press or release onto the event-loop thread.
 */
static void guac_spice_defer_button(SpiceInputsChannel* inputs, int button,
        int mask, int pressed) {
    if (inputs == NULL)
        return;
    guac_spice_deferred_call* call = g_new0(guac_spice_deferred_call, 1);
    call->handler = pressed ? guac_spice_do_button_press : guac_spice_do_button_release;
    call->channel = inputs;
    call->args[0] = (unsigned int) button;
    call->args[1] = (unsigned int) mask;
    guac_spice_defer_call(call);
}

/**
 * Marshals relative SPICE pointer motion onto the event-loop thread.
 */
static void guac_spice_defer_motion(SpiceInputsChannel* inputs, int dx, int dy,
        int mask) {
    if (inputs == NULL)
        return;
    guac_spice_deferred_call* call = g_new0(guac_spice_deferred_call, 1);
    call->handler = guac_spice_do_motion;
    call->channel = inputs;
    call->args[0] = (unsigned int) dx;
    call->args[1] = (unsigned int) dy;
    call->args[2] = (unsigned int) mask;
    guac_spice_defer_call(call);
}

/**
 * Marshals an absolute SPICE pointer position onto the event-loop thread.
 */
static void guac_spice_defer_position(SpiceInputsChannel* inputs, int x, int y,
        int display, int mask) {
    if (inputs == NULL)
        return;
    guac_spice_deferred_call* call = g_new0(guac_spice_deferred_call, 1);
    call->handler = guac_spice_do_position;
    call->channel = inputs;
    call->args[0] = (unsigned int) x;
    call->args[1] = (unsigned int) y;
    call->args[2] = (unsigned int) display;
    call->args[3] = (unsigned int) mask;
    guac_spice_defer_call(call);
}

static void guac_spice_scroll(SpiceInputsChannel* inputs, int button,
        int button_state) {
    guac_spice_defer_button(inputs, button, button_state, 1);
    guac_spice_defer_button(inputs, button, button_state, 0);
}

int guac_spice_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    SpiceInputsChannel* inputs = spice_client->inputs_channel;

    /* Track current mouse location/state for all connected users */
    guac_display_render_thread_notify_user_moved_mouse(
            spice_client->render_thread, user, x, y, mask);

    /* Report mouse position within recording */
    if (spice_client->recording != NULL)
        guac_recording_report_mouse(spice_client->recording, x, y, mask);

    /* Send SPICE events only once the inputs channel is ready */
    if (inputs == NULL)
        return 0;

    int spice_mask = guac_spice_button_mask(mask);
    int prev_mask = guac_spice_button_mask(spice_client->last_mouse_mask);

    /* Update pointer position. SPICE client mouse mode accepts absolute
     * coordinates directly; server mode requires relative motion deltas. */
    if (spice_client->mouse_mode == SPICE_MOUSE_MODE_SERVER)
        guac_spice_defer_motion(inputs,
                x - spice_client->last_mouse_x,
                y - spice_client->last_mouse_y, spice_mask);
    else
        guac_spice_defer_position(inputs, x, y, 0, spice_mask);

    /* Send press/release events for any of the primary buttons that changed
     * state since the previous mouse event */
    if (spice_mask != prev_mask) {

        if ((spice_mask & SPICE_MOUSE_BUTTON_MASK_LEFT)
                != (prev_mask & SPICE_MOUSE_BUTTON_MASK_LEFT))
            guac_spice_defer_button(inputs, SPICE_MOUSE_BUTTON_LEFT, spice_mask,
                    spice_mask & SPICE_MOUSE_BUTTON_MASK_LEFT);

        if ((spice_mask & SPICE_MOUSE_BUTTON_MASK_MIDDLE)
                != (prev_mask & SPICE_MOUSE_BUTTON_MASK_MIDDLE))
            guac_spice_defer_button(inputs, SPICE_MOUSE_BUTTON_MIDDLE, spice_mask,
                    spice_mask & SPICE_MOUSE_BUTTON_MASK_MIDDLE);

        if ((spice_mask & SPICE_MOUSE_BUTTON_MASK_RIGHT)
                != (prev_mask & SPICE_MOUSE_BUTTON_MASK_RIGHT))
            guac_spice_defer_button(inputs, SPICE_MOUSE_BUTTON_RIGHT, spice_mask,
                    spice_mask & SPICE_MOUSE_BUTTON_MASK_RIGHT);

    }

    /* Handle scroll wheel as momentary button presses, triggered on the
     * leading edge of the corresponding Guacamole scroll bit */
    if ((mask & GUAC_CLIENT_MOUSE_SCROLL_UP)
            && !(spice_client->last_mouse_mask & GUAC_CLIENT_MOUSE_SCROLL_UP))
        guac_spice_scroll(inputs, SPICE_MOUSE_BUTTON_UP, spice_mask);

    if ((mask & GUAC_CLIENT_MOUSE_SCROLL_DOWN)
            && !(spice_client->last_mouse_mask & GUAC_CLIENT_MOUSE_SCROLL_DOWN))
        guac_spice_scroll(inputs, SPICE_MOUSE_BUTTON_DOWN, spice_mask);

    /* Remember state for the next event */
    spice_client->last_mouse_mask = mask;
    spice_client->last_mouse_x = x;
    spice_client->last_mouse_y = y;

    return 0;

}

int guac_spice_user_key_handler(guac_user* user, int keysym, int pressed) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    int retval = 0;

    /* Report key state within recording */
    if (spice_client->recording != NULL)
        guac_recording_report_key(spice_client->recording, keysym, pressed);

    pthread_rwlock_rdlock(&(spice_client->lock));

    /* Translate and send the key event only once the inputs channel and
     * keyboard are ready. The keyboard handles mapping the keysym to the
     * appropriate scancode(s) for the negotiated keyboard layout, including
     * any required modifier and lock key synchronization. */
    if (spice_client->inputs_channel != NULL && spice_client->keyboard != NULL)
        retval = guac_spice_keyboard_update_keysym(spice_client->keyboard,
                keysym, pressed, GUAC_SPICE_KEY_SOURCE_CLIENT);

    pthread_rwlock_unlock(&(spice_client->lock));

    return retval;

}

/**
 * Recomputes the x_position and left_offset of every active monitor. Monitors
 * are tiled left-to-right, so each monitor's left_offset is the sum of the
 * widths of the monitors before it and its x_position is its array index. Must
 * be called on the SPICE event-loop thread whenever the monitor set changes.
 *
 * @param spice_client
 *     The SPICE client whose monitor offsets should be recomputed.
 */
static void guac_spice_monitors_recalc_offsets(guac_spice_client* spice_client) {

    int left = 0;
    for (int i = 0; i < spice_client->monitors_count; i++) {
        spice_client->monitors[i].x_position = i;
        spice_client->monitors[i].left_offset = left;
        left += spice_client->monitors[i].width;
    }

}

/**
 * Performs a deferred SPICE multi-monitor resize on the event-loop thread,
 * pushing the current monitor layout to the guest as a single monitors config.
 */
void guac_spice_resize_try(guac_client* client) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Nothing to do unless a resize is queued */
    if (!spice_client->resize_pending)
        return;

    /* Wait until the guest is ready to receive a monitors config: the main
     * channel must be up, the agent connected and advertising monitors-config
     * support, and the display's primary surface created. spice-gtk silently
     * drops a monitors config sent before these conditions hold. */
    if (spice_client->main_channel == NULL
            || !spice_client->resize_agent_ready
            || !spice_client->resize_display_ready)
        return;

    /* Enable and position every active monitor */
    for (int i = 0; i < spice_client->monitors_count; i++) {
        guac_spice_monitor* monitor = &spice_client->monitors[i];
        spice_main_channel_update_display_enabled(spice_client->main_channel,
                i, TRUE, FALSE);
        spice_main_channel_update_display(spice_client->main_channel, i,
                monitor->left_offset, monitor->top_offset,
                monitor->width, monitor->height, FALSE);
    }

    /* Disable any monitors that were previously enabled but have since been
     * removed */
    for (int i = spice_client->monitors_count;
            i < spice_client->resize_monitors_pushed; i++)
        spice_main_channel_update_display_enabled(spice_client->main_channel,
                i, FALSE, FALSE);

    /* Push the whole layout to the guest agent in a single, explicit monitors
     * config (rather than via update_display's debounced timer) so every
     * resize is delivered deterministically. */
    spice_main_channel_send_monitor_config(spice_client->main_channel);

    spice_client->resize_monitors_pushed = spice_client->monitors_count;
    spice_client->resize_pending = 0;
    guac_client_log(client, GUAC_LOG_DEBUG,
            "Sent guest display config: %d monitor(s)",
            spice_client->monitors_count);

}

/**
 * Re-evaluates whether the SPICE agent can drive a display resize (connected and
 * advertising monitors-config support) and flushes any queued resize. The
 * monitors-config capability is announced shortly after the agent connects, so
 * this is invoked both on agent connection and on subsequent agent updates.
 */
static void guac_spice_resize_set_agent_ready(SpiceMainChannel* channel,
        guac_client* client) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    gboolean connected = FALSE;
    g_object_get(channel, "agent-connected", &connected, NULL);

    int ready = (connected
            && spice_main_channel_agent_test_capability(channel,
                    VD_AGENT_CAP_MONITORS_CONFIG));

    /* Log only on a change to avoid noise from repeated agent updates */
    if (ready != spice_client->resize_agent_ready)
        guac_client_log(client, GUAC_LOG_DEBUG,
                "SPICE agent monitors-config resize %s",
                ready ? "available" : "unavailable");

    spice_client->resize_agent_ready = ready;

    /* A resize may have been queued before the agent became ready */
    guac_spice_resize_try(client);

}

void guac_spice_resize_agent_update(SpiceMainChannel* channel, GParamSpec* pspec,
        guac_client* client) {
    guac_spice_resize_set_agent_ready(channel, client);
}

void guac_spice_resize_agent_updated(SpiceMainChannel* channel,
        guac_client* client) {
    guac_spice_resize_set_agent_ready(channel, client);
}

/**
 * Handler which, on the SPICE event-loop thread, applies a queued per-monitor
 * resize request to the monitor set and attempts to send the resulting
 * monitors config (subject to guest readiness).
 */
static void guac_spice_do_queue_resize(guac_spice_deferred_call* call) {

    guac_client* client = (guac_client*) call->channel;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    int width      = (int) call->args[0];
    int height     = (int) call->args[1];
    int x_position = (int) call->args[2];
    int top_offset = (int) call->args[3];

    /* Number of monitors permitted (primary plus the configured secondaries),
     * capped at the number of heads guacd supports */
    int max_monitors = spice_client->settings->max_secondary_monitors + 1;
    if (max_monitors > GUAC_SPICE_MAX_MONITORS)
        max_monitors = GUAC_SPICE_MAX_MONITORS;

    /* A positive size adds or updates the monitor at x_position */
    if (width > 0 && height > 0) {

        /* Ignore out-of-range or disallowed indexes, and refuse to create a
         * gap in the (contiguous) monitor layout */
        if (x_position < 0 || x_position >= max_monitors
                || x_position > spice_client->monitors_count)
            return;

        guac_spice_monitor* monitor = &spice_client->monitors[x_position];
        monitor->width      = width;
        monitor->height     = height;
        monitor->top_offset = top_offset;

        /* Grow the active monitor count if this request adds a new monitor */
        if (x_position == spice_client->monitors_count)
            spice_client->monitors_count = x_position + 1;

    }

    /* A non-positive size closes the secondary monitor at x_position, shifting
     * any monitors to its right down to keep the layout contiguous. The
     * primary monitor (index 0) cannot be closed. */
    else {

        if (x_position <= 0 || x_position >= spice_client->monitors_count)
            return;

        for (int i = x_position; i < spice_client->monitors_count - 1; i++)
            spice_client->monitors[i] = spice_client->monitors[i + 1];

        spice_client->monitors_count--;

    }

    guac_spice_monitors_recalc_offsets(spice_client);

    spice_client->resize_pending = 1;
    guac_spice_resize_try(client);

}

int guac_spice_user_size_handler(guac_user* user, int width, int height,
        int x_position, int top_offset) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Ignore if the main channel is not yet ready */
    if (spice_client->main_channel == NULL)
        return 0;

    /* SPICE guests generally require the display width to be a multiple of 8. A
     * non-positive size is a monitor-close request and is passed through
     * unchanged. */
    if (width > 0)
        width &= ~0x7;

    /* Queue the resize on the SPICE event-loop thread; spice-gtk channel
     * functions must not be called from user threads (see guac_spice_defer_call).
     * The actual send is gated on guest readiness within guac_spice_resize_try. */
    guac_spice_deferred_call* call = g_new0(guac_spice_deferred_call, 1);
    call->handler = guac_spice_do_queue_resize;
    call->channel = client;
    call->args[0] = (unsigned int) width;
    call->args[1] = (unsigned int) height;
    call->args[2] = (unsigned int) x_position;
    call->args[3] = (unsigned int) top_offset;
    guac_spice_defer_call(call);

    return 0;

}
