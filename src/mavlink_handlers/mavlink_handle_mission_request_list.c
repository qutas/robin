#include "mavlink_system.h"
#include "mavlink_receive.h"

void mavlink_handle_mission_request_list( uint8_t port, mavlink_message_t *msg, mavlink_status_t *status ) {
	//XXX: We don't support missions, so just send back 0
	mavlink_message_t msg_out;
	mavlink_msg_mission_count_pack(mavlink_system.sysid,
								   mavlink_system.compid,
								   &msg_out,
								   mavlink_system.sysid,
								   mavlink_system.compid,
								   0, 0);
	lpq_queue_msg(port, &msg_out);
}
