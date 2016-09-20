
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

#ifndef __GUAC_SCREEN_H
#define __GUAC_SCREEN_H

#include "config.h"
#include "guac_display.h"
#include "list.h"

#include <xorg-server.h>
#include <xf86.h>
#include <xf86str.h>

#include <guacamole/client.h>
#include <guacamole/pool.h>

/**
 * Private data for each screen, containing handlers for wrapped functions
 * and structures required for Guacamole protocol communication.
 */
typedef struct guac_drv_screen {

    /**
     * The Guacamole display.
     */
    guac_drv_display* display;

    /**
     * The framebuffer backing the screen.
     */
    unsigned char* framebuffer;

    /**
     * Wrapped CloseScreen implementation.
     */
    CloseScreenProcPtr wrapped_close_screen;

    /**
     * Wrapped CreatePixmap implementation.
     */
    CreatePixmapProcPtr wrapped_create_pixmap;

    /**
     * Wrapped DestroyPixmap implementation.
     */
    DestroyPixmapProcPtr wrapped_destroy_pixmap;

    /**
     * Wrapped CreateWindow implementation.
     */
    CreateWindowProcPtr wrapped_create_window;

    /**
     * Wrapped CreateGC implementation.
     */
    CreateGCProcPtr wrapped_create_gc;

    /**
     * Wrapped RealizeWindow implementation.
     */
    RealizeWindowProcPtr wrapped_realize_window;

    /**
     * Wrapped UnrealizeWindow implementation.
     */
    UnrealizeWindowProcPtr wrapped_unrealize_window;

    /**
     * Wrapped MoveWindow implementation.
     */
    MoveWindowProcPtr wrapped_move_window;

    /**
     * Wrapped ResizeWindow implementation.
     */
    ResizeWindowProcPtr wrapped_resize_window;

    /**
     * Wrapped ReparentWindow implementation.
     */
    ReparentWindowProcPtr wrapped_reparent_window;

    /**
     * Wrapped RestackWindow implementation.
     */
    RestackWindowProcPtr wrapped_restack_window;

    /**
     * Wrapped DestroyWindow implementation.
     */
    DestroyWindowProcPtr wrapped_destroy_window;

    /**
     * Wrapped ChangeWindowAttributes implementation.
     */
    ChangeWindowAttributesProcPtr wrapped_change_window_attributes;

} guac_drv_screen;

/**
 * Initializes the given screen.
 */
Bool guac_drv_pre_init(ScrnInfoPtr screen, int flags);

/**
 * Returns whether the given mode is valid on the given screen.
 */
ModeStatus guac_drv_valid_mode(ScrnInfoPtr screen_info, DisplayModePtr mode,
        Bool verbose, int flags);

/**
 * Called when the VT is entered.
 */
Bool guac_drv_enter_vt(ScrnInfoPtr screen_info);

/**
 * Called when leaving the VT.
 */
void guac_drv_leave_vt(ScrnInfoPtr screen_info);

/**
 * Called to initialize the members of the given screen.
 */
Bool guac_drv_screen_init(ScreenPtr screen, int argc, char** argv);

/**
 * Frees all memory associated with the given screen.
 */
void guac_drv_free_screen(ScrnInfoPtr screen_info);

/**
 * Switches the given screen to the given mode.
 */
Bool guac_drv_switch_mode(ScrnInfoPtr screen_info, DisplayModePtr mode);

/**
 * Sets which section of the framebuffer is visible within the viewport.
 */
void guac_drv_adjust_frame(ScrnInfoPtr screen_info, int x, int y);

/**
 * Synchronize the given client with the screen state.
 */
void guac_drv_screen_sync_client(guac_drv_screen* screen, guac_client* client);

/**
 * Completes the current frame, flushing all buffers and sending syncs.
 */
void guac_drv_screen_end_frame(guac_drv_screen* screen);

/**
 * Key for retrieving/setting Guacamole-specific information.
 */
extern const DevPrivateKey GUAC_SCREEN_PRIVATE;

#endif

