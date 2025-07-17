#include "channels/rdpdr/rdpdr-utils.h"
#include <freerdp/channels/rdpdr.h>

#include <stdint.h>

const char* rdpdr_irp_string(uint32_t major)
{
	switch (major)
	{
		case IRP_MJ_CREATE:
			return "IRP_MJ_CREATE";
		case IRP_MJ_CLOSE:
			return "IRP_MJ_CLOSE";
		case IRP_MJ_READ:
			return "IRP_MJ_READ";
		case IRP_MJ_WRITE:
			return "IRP_MJ_WRITE";
		case IRP_MJ_DEVICE_CONTROL:
			return "IRP_MJ_DEVICE_CONTROL";
		case IRP_MJ_QUERY_VOLUME_INFORMATION:
			return "IRP_MJ_QUERY_VOLUME_INFORMATION";
		case IRP_MJ_SET_VOLUME_INFORMATION:
			return "IRP_MJ_SET_VOLUME_INFORMATION";
		case IRP_MJ_QUERY_INFORMATION:
			return "IRP_MJ_QUERY_INFORMATION";
		case IRP_MJ_SET_INFORMATION:
			return "IRP_MJ_SET_INFORMATION";
		case IRP_MJ_DIRECTORY_CONTROL:
			return "IRP_MJ_DIRECTORY_CONTROL";
		case IRP_MJ_LOCK_CONTROL:
			return "IRP_MJ_LOCK_CONTROL";
		default:
			return "IRP_UNKNOWN";
	}
}
