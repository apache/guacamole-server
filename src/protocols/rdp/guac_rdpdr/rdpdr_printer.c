
#include <errno.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <freerdp/utils/svc_plugin.h>

#include <guacamole/protocol.h>

#include "rdpdr_messages.h"
#include "rdpdr_printer.h"
#include "rdpdr_service.h"
#include "client.h"

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
    "-f",
    "-",
    NULL
};

static void* guac_rdpdr_print_filter_output_thread(void* data) {

    guac_rdpdrPlugin* rdpdr = (guac_rdpdrPlugin*) data;

    int length;
    char buffer[8192];

    /* Write all output as blobs */
    while ((length = read(rdpdr->printer_output, buffer, sizeof(buffer))) > 0)
        guac_protocol_send_blob(rdpdr->client->socket,
                GUAC_RDPDR_PRINTER_BLOB, buffer, length);

    /* Log any error */
    if (length < 0)
        guac_client_log_error(rdpdr->client, "Error reading from filter: %s", strerror(errno));

    return NULL;

}

static int guac_rdpdr_create_print_process(guac_rdpdrPlugin* rdpdr) {

    int child_pid;
    int stdin_pipe[2];
    int stdout_pipe[2];

    /* Create STDIN pipe */
    if (pipe(stdin_pipe)) {
        guac_client_log_error(rdpdr->client, "Unable to create STDIN pipe for PDF filter process: %s", strerror(errno));
        return 1;
    }

    /* Create STDOUT pipe */
    if (pipe(stdout_pipe)) {
        guac_client_log_error(rdpdr->client, "Unable to create STDIN pipe for PDF filter process: %s", strerror(errno));
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        return 1;
    }

    /* Store our side of stdin/stdout */
    rdpdr->printer_input  = stdin_pipe[1];
    rdpdr->printer_output = stdout_pipe[0];

    /* Start output thread */
    if (pthread_create(&(rdpdr->printer_output_thread), NULL, guac_rdpdr_print_filter_output_thread, rdpdr)) {
        guac_client_log_error(rdpdr->client, "Unable to fork PDF filter process");
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return 1;
    }

    /* Fork child process */
    child_pid = fork();

    /* Log fork errors */
    if (child_pid == -1) {
        guac_client_log_error(rdpdr->client, "Unable to fork PDF filter process: %s", strerror(errno));
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
        guac_client_log_info(rdpdr->client, "Running %s", guac_rdpdr_pdf_filter_command[0]);
        if (execvp(guac_rdpdr_pdf_filter_command[0], guac_rdpdr_pdf_filter_command) < 0)
            guac_client_log_error(rdpdr->client, "Unable to execute PDF filter command: %s", strerror(errno));
        else
            guac_client_log_error(rdpdr->client, "Unable to execute PDF filter command, but no error given");

        /* Terminate child process */
        exit(1);

    }

    /* Log fork success */
    guac_client_log_info(rdpdr->client, "Created PDF filter process PID=%i", child_pid);

    /* Close unneeded ends of pipe */
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    return 0;

}

void guac_rdpdr_process_print_job_create(guac_rdpdrPlugin* rdpdr, wStream* input_stream, int completion_id) {

    wStream* output_stream = Stream_New(NULL, 24);

    /* No bytes received yet */
    rdpdr->bytes_received = 0;

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, GUAC_PRINTER_DEVICE_ID);
    Stream_Write_UINT32(output_stream, completion_id);
    Stream_Write_UINT32(output_stream, 0); /* Success */
    Stream_Write_UINT32(output_stream, 0); /* fileId */

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);

}

void guac_rdpdr_process_print_job_write(guac_rdpdrPlugin* rdpdr, wStream* input_stream, int completion_id) {

    int status=0, length;
    unsigned char* buffer;

    wStream* output_stream = Stream_New(NULL, 24);

    Stream_Read_UINT32(input_stream, length);
    Stream_Seek(input_stream, 8);  /* Offset */
    Stream_Seek(input_stream, 20); /* Padding */
    buffer = Stream_Pointer(input_stream);

    /* Create print job, if not yet created */
    if (rdpdr->bytes_received == 0) {

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

        /* Begin file */
        guac_client_log_info(rdpdr->client, "Print job created");
        guac_protocol_send_file(rdpdr->client->socket,
                GUAC_RDPDR_PRINTER_BLOB, "application/pdf", filename);

        /* Start print process */
        if (guac_rdpdr_create_print_process(rdpdr) != 0) {
            status = 0x80000010;
            length = 0;
        }

    }

    rdpdr->bytes_received += length;

    /* If not yet failed, write received data */
    if (status == 0) {

        /* Write data to printer, translate output for RDP */
        length = write(rdpdr->printer_input, buffer, length);
        if (length == -1) {
            guac_client_log_error(rdpdr->client, "Error writing to printer: %s", strerror(errno));
            status = 0x80000010;
            length = 0;
        }

    }

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, GUAC_PRINTER_DEVICE_ID);
    Stream_Write_UINT32(output_stream, completion_id);
    Stream_Write_UINT32(output_stream, status);
    Stream_Write_UINT32(output_stream, length);
    Stream_Write_UINT8(output_stream, 0); /* padding (stated as optional in spec, but requests fail without) */

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);

}

void guac_rdpdr_process_print_job_close(guac_rdpdrPlugin* rdpdr, wStream* input_stream, int completion_id) {

    wStream* output_stream = Stream_New(NULL, 24);

    /* Close input and wait for output thread to finish */
    close(rdpdr->printer_input);
    pthread_join(rdpdr->printer_output_thread, NULL);

    /* Close file descriptors */
    close(rdpdr->printer_output);

    /* Close file */
    guac_client_log_info(rdpdr->client, "Print job closed");
    guac_protocol_send_end(rdpdr->client->socket, GUAC_RDPDR_PRINTER_BLOB);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, GUAC_PRINTER_DEVICE_ID);
    Stream_Write_UINT32(output_stream, completion_id);
    Stream_Write_UINT32(output_stream, 0); /* NTSTATUS - success */
    Stream_Write_UINT32(output_stream, 0); /* padding*/

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);

}

