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
 * Handler which performs a deferred SPICE primary-display resize on the
 * event-loop thread. The requested width and height are carried in the
 * deferred call's args.
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

    int width = spice_client->resize_pending_width;
    int height = spice_client->resize_pending_height;

    /* Enable and size monitor 0, then explicitly push the monitors config to
     * the guest agent. Sending explicitly (rather than via update_display's
     * debounced timer) ensures every resize is delivered deterministically. */
    spice_main_channel_update_display_enabled(spice_client->main_channel, 0, TRUE, FALSE);
    spice_main_channel_update_display(spice_client->main_channel, 0, 0, 0,
            width, height, FALSE);
    spice_main_channel_send_monitor_config(spice_client->main_channel);

    spice_client->resize_pending = 0;
    guac_client_log(client, GUAC_LOG_DEBUG,
            "Sent guest display resize to %dx%d", width, height);

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
 * Handler which, on the SPICE event-loop thread, records a queued resize
 * request and attempts to send it (subject to guest readiness).
 */
static void guac_spice_do_queue_resize(guac_spice_deferred_call* call) {

    guac_client* client = (guac_client*) call->channel;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    spice_client->resize_pending_width = (int) call->args[0];
    spice_client->resize_pending_height = (int) call->args[1];
    spice_client->resize_pending = 1;

    guac_spice_resize_try(client);

}

int guac_spice_user_size_handler(guac_user* user, int width, int height) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Ignore if the main channel is not yet ready or the requested size is
     * degenerate */
    if (spice_client->main_channel == NULL || width <= 0 || height <= 0)
        return 0;

    /* SPICE guests generally require the display width to be a multiple of 8 */
    width &= ~0x7;

    /* Queue the resize on the SPICE event-loop thread; spice-gtk channel
     * functions must not be called from user threads (see guac_spice_defer_call).
     * The actual send is gated on guest readiness within guac_spice_resize_try. */
    guac_spice_deferred_call* call = g_new0(guac_spice_deferred_call, 1);
    call->handler = guac_spice_do_queue_resize;
    call->channel = client;
    call->args[0] = (unsigned int) width;
    call->args[1] = (unsigned int) height;
    guac_spice_defer_call(call);

    return 0;

}
