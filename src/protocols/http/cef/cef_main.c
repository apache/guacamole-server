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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/capi/cef_base_capi.h"
#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_browser_capi.h"
#include "include/capi/cef_command_line_capi.h"

#include "cef_client.h"
#include "cef_life_span_handler.h"
#include "cef_render_handler.h"

/**
 * Function called before the command-line processing starts. It appends the necessary
 * command line switches for CEF headless operation.
 *
 * @param self Pointer to the application instance.
 * @param process_type Pointer to the current process type string.
 * @param command_line Pointer to the command line instance.
 */
void on_before_command_line_processing(struct _cef_app_t* self,
                                       const cef_string_t* process_type,
                                       struct _cef_command_line_t* command_line) {
    /* Append the --disable-gpu switch to the command line arguments */
    cef_string_t disable_gpu = {};
    cef_string_from_ascii("--disable-gpu", 13, &disable_gpu);
    command_line->append_switch(command_line, &disable_gpu);
    
    /* Append the --no-sandbox switch to the command line argument */
    cef_string_t no_sandbox = {};
    cef_string_from_ascii("--no-sandbox", 12, &no_sandbox);
    command_line->append_switch(command_line, &no_sandbox);

    /* Append the --no-zygote switch to the command line arguments */
    cef_string_t no_zygote = {};
    cef_string_from_ascii("--no-zygote", 11, &no_zygote);
    command_line->append_switch(command_line, &no_zygote);
}

/**
 * Main entry point of the headless browser application.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments, including the executable name.
 * 
 * @return Zero (0) if successful, non-zero error code otherwise.
 */
int main(int argc, char* argv[]) {
    /**
     * Log process arguments to differentiate between main process and
     * subprocesses.
     */
    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "%s ", argv[i]);
    }

    fprintf(stderr, "\n");

    /* Initialize CEF */
    cef_main_args_t main_args = {0};
    main_args.argc = argc;
    main_args.argv = argv;
    
    cef_settings_t settings = {0};
    settings.size = sizeof(cef_settings_t);
    settings.log_severity = LOGSEVERITY_ERROR; // or LOGSEVERITY_WARNING or LOGSEVERITY_VERBOSE
    cef_string_utf8_to_utf16("cef_log.txt", strlen("cef_log.txt"), &settings.log_file);

    /* Set CEF_RESOURCE_PATH and CEF_LOCALES_PATH for proper CEF operation */
    cef_string_utf8_to_utf16(CEF_RESOURCE_PATH, strlen(CEF_RESOURCE_PATH), &settings.resources_dir_path);
    cef_string_utf8_to_utf16(CEF_LOCALES_PATH, strlen(CEF_LOCALES_PATH), &settings.locales_dir_path);

    /* Enable headless mode */
    settings.windowless_rendering_enabled = 1;

    /* Allocate a new cef_app_t structure */
    cef_app_t app;
    memset(&app, 0, sizeof(cef_app_t));
    app.base.size = sizeof(cef_app_t);
    app.on_before_command_line_processing = on_before_command_line_processing;

    if (!cef_initialize(&main_args, &settings, &app, NULL)) {
        fprintf(stderr, "Failed to initialize CEF.\n");
        return 1;
    }

    /* Create a custom render handler */
    custom_render_handler_t *render_handler = create_render_handler();

    /* Create a custom client */
    cef_client_t *client = create_client(render_handler);

    /* Browser settings */
    cef_browser_settings_t browser_settings = {0};
    browser_settings.size = sizeof(cef_browser_settings_t);

    /* Create a browser instance */
    cef_window_info_t window_info = {0};
    window_info.windowless_rendering_enabled = 1;

    cef_string_t url = {0};
    cef_string_utf8_to_utf16("https://www.example.com", strlen("https://www.example.com"), &url);
    cef_browser_host_create_browser(&window_info, client, &url, &browser_settings, NULL, NULL);

    /* Main loop to handle CEF-related actions and rendering until termination */
    cef_run_message_loop();

    /* Cleanup resources when the client is no longer running */
    cef_shutdown();
    free(render_handler);
    free(client);

    fprintf(stderr, "Exiting CEF process!\n");

    return 0;
}
