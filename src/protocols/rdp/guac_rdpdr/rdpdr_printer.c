
#include <freerdp/utils/stream.h>
#include <freerdp/utils/svc_plugin.h>

#include "rdpdr_messages.h"
#include "rdpdr_service.h"

void guac_rdpdr_process_print_job_create(guac_rdpdrPlugin* rdpdr, STREAM* input_stream, int completion_id) {

    STREAM* output_stream = stream_new(24);

    guac_client_log_info(rdpdr->client, "Print job created");

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

    STREAM* output_stream = stream_new(24);

    stream_read_uint32(input_stream, length);
    stream_seek(input_stream, 8);  /* Offset */
    stream_seek(input_stream, 20); /* Padding */

    /* TODO: Here, read data */
    guac_client_log_info(rdpdr->client, "Data received: %i bytes", length);

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

    STREAM* output_stream = stream_new(24);

    guac_client_log_info(rdpdr->client, "Print job closed");

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

