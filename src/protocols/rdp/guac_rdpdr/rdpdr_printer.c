
#include <freerdp/utils/stream.h>
#include <freerdp/utils/svc_plugin.h>

#include <guacamole/protocol.h>

#include "rdpdr_messages.h"
#include "rdpdr_printer.h"
#include "rdpdr_service.h"
#include "client.h"

void guac_rdpdr_process_print_job_create(guac_rdpdrPlugin* rdpdr, STREAM* input_stream, int completion_id) {

    rdp_guac_client_data* client_data = (rdp_guac_client_data*) rdpdr->client->data;
    STREAM* output_stream = stream_new(24);

    /* Open file */
    guac_client_log_info(rdpdr->client, "Print job created");
    pthread_mutex_lock(&(client_data->update_lock));
    guac_protocol_send_file(rdpdr->client->socket,
            GUAC_RDPDR_PRINTER_BLOB, "print.ps", "application/postscript");
    pthread_mutex_unlock(&(client_data->update_lock));

    /* Write header */
    stream_write_uint16(output_stream, RDPDR_CTYP_CORE);
    stream_write_uint16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    stream_write_uint32(output_stream, GUAC_PRINTER_DEVICE_ID);
    stream_write_uint32(output_stream, completion_id);
    stream_write_uint32(output_stream, 0); /* NTSTATUS - success */
    stream_write_uint32(output_stream, 0); /* fileId */

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);

}

void guac_rdpdr_process_print_job_write(guac_rdpdrPlugin* rdpdr, STREAM* input_stream, int completion_id) {

    int length;
    unsigned char* buffer;

    rdp_guac_client_data* client_data = (rdp_guac_client_data*) rdpdr->client->data;
    STREAM* output_stream = stream_new(24);

    stream_read_uint32(input_stream, length);
    stream_seek(input_stream, 8);  /* Offset */
    stream_seek(input_stream, 20); /* Padding */
    buffer = stream_get_tail(input_stream);

    /* Send data */
    pthread_mutex_lock(&(client_data->update_lock));
    guac_protocol_send_blob(rdpdr->client->socket,
            GUAC_RDPDR_PRINTER_BLOB, buffer, length);
    pthread_mutex_unlock(&(client_data->update_lock));

    /* Write header */
    stream_write_uint16(output_stream, RDPDR_CTYP_CORE);
    stream_write_uint16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    stream_write_uint32(output_stream, GUAC_PRINTER_DEVICE_ID);
    stream_write_uint32(output_stream, completion_id);
    stream_write_uint32(output_stream, 0); /* NTSTATUS - success */
    stream_write_uint32(output_stream, length);
    stream_write_uint8(output_stream, 0); /* padding (stated as optional in spec, but requests fail without) */

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);

}

void guac_rdpdr_process_print_job_close(guac_rdpdrPlugin* rdpdr, STREAM* input_stream, int completion_id) {

    rdp_guac_client_data* client_data = (rdp_guac_client_data*) rdpdr->client->data;
    STREAM* output_stream = stream_new(24);

    /* Close file */
    guac_client_log_info(rdpdr->client, "Print job closed");
    pthread_mutex_lock(&(client_data->update_lock));
    guac_protocol_send_end(rdpdr->client->socket, GUAC_RDPDR_PRINTER_BLOB);
    pthread_mutex_unlock(&(client_data->update_lock));

    /* Write header */
    stream_write_uint16(output_stream, RDPDR_CTYP_CORE);
    stream_write_uint16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    stream_write_uint32(output_stream, GUAC_PRINTER_DEVICE_ID);
    stream_write_uint32(output_stream, completion_id);
    stream_write_uint32(output_stream, 0); /* NTSTATUS - success */
    stream_write_uint32(output_stream, 0); /* padding*/

    svc_plugin_send((rdpSvcPlugin*) rdpdr, output_stream);

}

