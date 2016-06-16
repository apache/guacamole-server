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

#include "rdpdr_messages.h"
#include "rdpdr_printer.h"
#include "rdpdr_service.h"
#include "rdp_status.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Command to run GhostScript safely as a filter writing PDF */
char* const guac_rdpdr_pdf_filter_command[] = {
    "gs",
    "-q",
    "-dNOPAUSE",
    "-dBATCH",
    "-dSAFER",
    "-dPARANOIDSAFER",
    "-sDEVICE=pdfwrite",
    "-sOutputFile=-",
    "-c",
    ".setpdfwrite",
    "-sstdout=/dev/null",
    "-f",
    "-",
    NULL
};

/**
 * Handler for "ack" messages received in response to printed data. Additional
 * data will be sent as a result or, if no data remains, the stream will be
 * terminated. It is required that the data pointer of the provided stream be
 * set to the file descriptor from which the printed data should be read.
 *
 * @param user
 *     The user to whom the printed data is being sent.
 *
 * @param stream
 *     The stream along which the printed data is to be sent. The data pointer
 *     of this stream MUST be set to the file descriptor from which the data
 *     being sent is to be read.
 *
 * @param message
 *     An arbitrary, human-readable message describing the success/failure of
 *     the operation being acknowledged (either stream creation or receipt of
 *     a blob).
 *
 * @param status
 *     The status code describing the success/failure of the operation being
 *     acknowledged (either stream creation or receipt of a blob).
 *
 * @return
 *     Always zero.
 */
static int guac_rdpdr_print_filter_ack_handler(guac_user* user,
        guac_stream* stream, char* message, guac_protocol_status status) {

    char buffer[6048];

    /* Pull file descriptor from stream data */
    int fd = (intptr_t) stream->data;

    /* Reading only if ack reports success */
    if (status == GUAC_PROTOCOL_STATUS_SUCCESS) {

        /* Write a single blob of output */
        int length = read(fd, buffer, sizeof(buffer));
        if (length > 0) {
            guac_protocol_send_blob(user->socket, stream, buffer, length);
            guac_socket_flush(user->socket);
            return 0;
        }

        /* Warn of read errors, fall through to termination */
        else if (length < 0)
            guac_user_log(user, GUAC_LOG_ERROR,
                    "Error reading from filter: %s", strerror(errno));

    }

    /* Note if stream aborted by user, fall through to termination */
    else
        guac_user_log(user, GUAC_LOG_INFO, "Print stream aborted.");

    /* Explicitly close down stream */
    guac_protocol_send_end(user->socket, stream);
    guac_socket_flush(user->socket);

    /* Clean up our end of the stream */
    guac_user_free_stream(user, stream);
    close(fd);

    return 0;

}

static int guac_rdpdr_create_print_process(guac_rdpdr_device* device) {

    guac_rdpdr_printer_data* printer_data = (guac_rdpdr_printer_data*) device->data;

    int child_pid;
    int stdin_pipe[2];
    int stdout_pipe[2];

    /* Create STDIN pipe */
    if (pipe(stdin_pipe)) {
        guac_client_log(device->rdpdr->client, GUAC_LOG_ERROR,
                "Unable to create STDIN pipe for PDF filter process: %s", strerror(errno));
        return 1;
    }

    /* Create STDOUT pipe */
    if (pipe(stdout_pipe)) {
        guac_client_log(device->rdpdr->client, GUAC_LOG_ERROR,
                "Unable to create STDIN pipe for PDF filter process: %s", strerror(errno));
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        return 1;
    }

    /* Store our side of stdin/stdout */
    printer_data->printer_input  = stdin_pipe[1];
    printer_data->printer_output = stdout_pipe[0];

    /* Fork child process */
    child_pid = fork();

    /* Log fork errors */
    if (child_pid == -1) {
        guac_client_log(device->rdpdr->client, GUAC_LOG_ERROR,
                "Unable to fork PDF filter process: %s", strerror(errno));
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return 1;
    }

    /* Child process */
    if (child_pid == 0) {

        /* Close unneeded ends of pipe */
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        /* Reassign file descriptors as STDIN/STDOUT */
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);

        /* Run PDF filter */
        guac_client_log(device->rdpdr->client, GUAC_LOG_INFO, "Running %s", guac_rdpdr_pdf_filter_command[0]);
        if (execvp(guac_rdpdr_pdf_filter_command[0], guac_rdpdr_pdf_filter_command) < 0)
            guac_client_log(device->rdpdr->client, GUAC_LOG_ERROR, "Unable to execute PDF filter command: %s", strerror(errno));
        else
            guac_client_log(device->rdpdr->client, GUAC_LOG_ERROR, "Unable to execute PDF filter command, but no error given");

        /* Terminate child process */
        exit(1);

    }

    /* Log fork success */
    guac_client_log(device->rdpdr->client, GUAC_LOG_INFO, "Created PDF filter process PID=%i", child_pid);

    /* Close unneeded ends of pipe */
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    return 0;

}

void guac_rdpdr_process_print_job_create(guac_rdpdr_device* device,
        wStream* input_stream, int completion_id) {

    guac_rdpdr_printer_data* printer_data =
        (guac_rdpdr_printer_data*) device->data;

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 4);

    /* No bytes received yet */
    printer_data->bytes_received = 0;
    Stream_Write_UINT32(output_stream, 0); /* fileId */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

/**
 * Given data representing a print device with a pending pring job, allocates a
 * new stream for the given user, associating it with the provided data and
 * returning the resulting guac_stream. The stream will be pre-configured to
 * send blobs of print data in response to "ack" messages received from the
 * user. If the given user is NULL, no stream will be allocated, and the
 * print job will be immediately aborted.
 *
 * @param user
 *     The user to whom the print job is being sent, or NULL if no stream
 *     should be allocated.
 *
 * @param data
 *     A pointer to the guac_rdpdr_device instance associated with the new
 *     print job.
 *
 * @return
 *     The guac_stream allocated for the new print job, or NULL if no stream
 *     could be allocated.
 */
static void* guac_rdpdr_alloc_printer_stream(guac_user* owner, void* data) {

    guac_rdpdr_device* device = (guac_rdpdr_device*) data;
    guac_rdpdr_printer_data* printer_data =
        (guac_rdpdr_printer_data*) device->data;

    /* Abort immediately if there is no owner */
    if (owner == NULL) {
        close(printer_data->printer_output);
        close(printer_data->printer_input);
        printer_data->printer_output = -1;
        printer_data->printer_input = -1;
        return NULL;
    }

    /* Allocate stream for owner */
    guac_stream* stream = guac_user_alloc_stream(owner);
    stream->ack_handler = guac_rdpdr_print_filter_ack_handler;
    stream->data = (void*) (intptr_t) printer_data->printer_output;

    return stream;

}

void guac_rdpdr_process_print_job_write(guac_rdpdr_device* device,
        wStream* input_stream, int completion_id) {

    guac_rdpdr_printer_data* printer_data = (guac_rdpdr_printer_data*) device->data;
    int status=0, length;
    unsigned char* buffer;

    wStream* output_stream;

    Stream_Read_UINT32(input_stream, length);
    Stream_Seek(input_stream, 8);  /* Offset */
    Stream_Seek(input_stream, 20); /* Padding */
    buffer = Stream_Pointer(input_stream);

    /* Create print job, if not yet created */
    if (printer_data->bytes_received == 0) {

        char filename[1024] = "guacamole-print.pdf";
        unsigned char* search = buffer;
        int i;

        /* Search for filename within buffer */
        for (i=0; i<length-9 && i < 2048; i++) {

            /* If title. use as filename */
            if (memcmp(search, "%%Title: ", 9) == 0) {

                /* Skip past "%%Title: " */
                search += 9;

                /* Copy as much of title as reasonable */
                int j;
                for (j=0; j<sizeof(filename) - 5 /* extension + 1 */ && i<length; i++, j++) {

                    /* Get character, stop at EOL */
                    char c = *(search++);
                    if (c == '\r' || c == '\n')
                        break;

                    /* Copy to filename */
                    filename[j] = c;

                }

                /* Append filename with extension */
                strcpy(&(filename[j]), ".pdf");
                break;
            }

            /* Next character */
            search++;

        }

        /* Start print process */
        if (guac_rdpdr_create_print_process(device) != 0) {
            status = STATUS_DEVICE_OFF_LINE;
            length = 0;
        }

        /* If print started successfully, create outbound stream */
        else {

            guac_client_log(device->rdpdr->client, GUAC_LOG_INFO,
                    "Print job created");

            /* Allocate stream */
            guac_stream* stream = (guac_stream*) guac_client_for_owner(
                    device->rdpdr->client, guac_rdpdr_alloc_printer_stream,
                    device);

            /* Begin file if stream allocation was successful */
            if (stream != NULL)
                guac_protocol_send_file(device->rdpdr->client->socket, stream,
                        "application/pdf", filename);

        }

    } /* end if print job beginning */

    printer_data->bytes_received += length;

    /* If not yet failed, write received data */
    if (status == 0) {

        /* Write data to printer, translate output for RDP */
        length = write(printer_data->printer_input, buffer, length);
        if (length == -1) {
            guac_client_log(device->rdpdr->client, GUAC_LOG_ERROR, "Error writing to printer: %s", strerror(errno));
            status = STATUS_DEVICE_OFF_LINE;
            length = 0;
        }

    }

    output_stream = guac_rdpdr_new_io_completion(device, completion_id,
            status, 5);

    Stream_Write_UINT32(output_stream, length);
    Stream_Write_UINT8(output_stream, 0); /* Padding */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_process_print_job_close(guac_rdpdr_device* device,
        wStream* input_stream, int completion_id) {

    guac_rdpdr_printer_data* printer_data =
        (guac_rdpdr_printer_data*) device->data;

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 4);

    Stream_Write_UINT32(output_stream, 0); /* Padding */

    /* Close input - the Guacamole stream will continue while output remains */
    close(printer_data->printer_input);
    printer_data->printer_input = -1;

    guac_client_log(device->rdpdr->client, GUAC_LOG_INFO, "Print job closed");
    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

static void guac_rdpdr_device_printer_announce_handler(guac_rdpdr_device* device,
        wStream* output_stream, int device_id) {

    /* Printer header */
    guac_client_log(device->rdpdr->client, GUAC_LOG_INFO, "Sending printer");
    Stream_Write_UINT32(output_stream, RDPDR_DTYP_PRINT);
    Stream_Write_UINT32(output_stream, device_id);
    Stream_Write(output_stream, "PRN1\0\0\0\0", 8); /* DOS name */

    /* Printer data */
    Stream_Write_UINT32(output_stream, 24 + GUAC_PRINTER_DRIVER_LENGTH + GUAC_PRINTER_NAME_LENGTH);
    Stream_Write_UINT32(output_stream,
              RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER
            | RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER);
    Stream_Write_UINT32(output_stream, 0); /* reserved - must be 0 */
    Stream_Write_UINT32(output_stream, 0); /* PnPName length (PnPName is ultimately ignored) */
    Stream_Write_UINT32(output_stream, GUAC_PRINTER_DRIVER_LENGTH); /* DriverName length */
    Stream_Write_UINT32(output_stream, GUAC_PRINTER_NAME_LENGTH);   /* PrinterName length */
    Stream_Write_UINT32(output_stream, 0);                          /* CachedFields length */

    Stream_Write(output_stream, GUAC_PRINTER_DRIVER, GUAC_PRINTER_DRIVER_LENGTH);
    Stream_Write(output_stream, GUAC_PRINTER_NAME,   GUAC_PRINTER_NAME_LENGTH);

}

static void guac_rdpdr_device_printer_iorequest_handler(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int major_func, int minor_func) {

    switch (major_func) {

        /* Print job create */
        case IRP_MJ_CREATE:
            guac_rdpdr_process_print_job_create(device, input_stream, completion_id);
            break;

        /* Printer job write */
        case IRP_MJ_WRITE:
            guac_rdpdr_process_print_job_write(device, input_stream, completion_id);
            break;

        /* Printer job close */
        case IRP_MJ_CLOSE:
            guac_rdpdr_process_print_job_close(device, input_stream, completion_id);
            break;

        /* Log unknown */
        default:
            guac_client_log(device->rdpdr->client, GUAC_LOG_ERROR,
                    "Unknown printer I/O request function: 0x%x/0x%x",
                    major_func, minor_func);

    }

}

static void guac_rdpdr_device_printer_free_handler(guac_rdpdr_device* device) {

    guac_rdpdr_printer_data* printer_data =
        (guac_rdpdr_printer_data*) device->data;

    /* Close print job input (STDIN for filter process) if open */
    if (printer_data->printer_input != -1)
        close(printer_data->printer_input);

    /* Close print job output (STDOUT for filter process) if open */
    if (printer_data->printer_output != -1)
        close(printer_data->printer_output);

    /* Free underlying data */
    free(device->data);

}

void guac_rdpdr_register_printer(guac_rdpdrPlugin* rdpdr) {

    int id = rdpdr->devices_registered++;

    /* Get new device */
    guac_rdpdr_device* device = &(rdpdr->devices[id]);
    guac_rdpdr_printer_data* printer_data;

    /* Init device */
    device->rdpdr       = rdpdr;
    device->device_id   = id;
    device->device_name = "Guacamole Printer";

    /* Set handlers */
    device->announce_handler  = guac_rdpdr_device_printer_announce_handler;
    device->iorequest_handler = guac_rdpdr_device_printer_iorequest_handler;
    device->free_handler      = guac_rdpdr_device_printer_free_handler;

    /* Init data */
    printer_data = malloc(sizeof(guac_rdpdr_printer_data));
    printer_data->printer_input = -1;
    printer_data->printer_output = -1;
    device->data = printer_data;

}

