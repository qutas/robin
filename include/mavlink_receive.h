#pragma once

//static int mode = MAV_MODE_UNINIT; /* Defined in mavlink_types.h, which is included by mavlink.h */
//static int packet_drops = 0;

//This will contain functions to receive and parse mavlink messages
//and should also hanlde any unsupported commands

void communication_receive(void);

//TODO: reminder for mav_cmd: systemResetToBootloader();

//==-- List of supported mavlink messages
	//x Ignored
	//o Optional
	//- Needed
	//+ Done!

	//TODO: Not all of these are relevant for just receiving

	//x actuator_control_target.h
	//x adsb_vehicle.h
	//x altitude.h
	//o attitude.h
	//o attitude_quaternion_cov.h
	//- attitude_quaternion.h
	//- attitude_target.h
	//o att_pos_mocap.h
	//x auth_key.h
	//- autopilot_version.h
	//- battery_status.h
	//x camera_trigger.h
	//x change_operator_control_ack.h
	//x change_operator_control.h
	//x collision.h
	//- command_ack.h
	//- command_int.h
	//- command_long.h
	//o control_system_state.h
	//x data_stream.h
	//x data_transmission_handshake.h
	//- debug.h
	//- debug_vect.h
	//- distance_sensor.h
	//x encapsulated_data.h
	//x estimator_status.h
	//x extended_sys_state.h
	//x file_transfer_protocol.h
	//x follow_target.h
	//x global_position_int_cov.h
	//x global_position_int.h
	//x global_vision_position_estimate.h
	//x gps2_raw.h
	//x gps2_rtk.h
	//x gps_global_origin.h
	//x gps_inject_data.h
	//x gps_input.h
	//x gps_raw_int.h
	//x gps_rtcm_data.h
	//x gps_rtk.h
	//x gps_status.h
	//- heartbeat.h
	//x high_latency.h
	//o highres_imu.h
	//o hil_actuator_controls.h
	//o hil_controls.h
	//x hil_gps.h
	//o hil_optical_flow.h
	//x hil_rc_inputs_raw.h
	//o hil_sensor.h
	//o hil_state.h
	//o hil_state_quaternion.h
	//x home_position.h
	//x landing_target.h
	//x local_position_ned_cov.h
	//x local_position_ned.h
	//x local_position_ned_system_global_offset.h
	//x log_data.h
	//x log_entry.h
	//x log_erase.h
	//x log_request_data.h
	//x log_request_end.h
	//x log_request_list.h
	//x manual_control.h
	//- manual_setpoint.h
	//x memory_vect.h
	//- message_interval.h
	//x mission_ack.h
	//x mission_clear_all.h
	//x mission_count.h
	//x mission_current.h
	//x mission_item.h
	//x mission_item_int.h
	//x mission_item_reached.h
	//x mission_request.h
	//x mission_request_int.h
	//x mission_request_list.h
	//x mission_request_partial_list.h
	//x mission_set_current.h
	//x mission_write_partial_list.h
	//- named_value_float.h
	//- named_value_int.h
	//x nav_controller_output.h
	//o optical_flow.h
	//o optical_flow_rad.h
	//x param_map_rc.h
	//- param_request_list.h
	//- param_request_read.h
	//- param_set.h
	//- param_value.h
	//- ping.h
	//x position_target_global_int.h
	//x position_target_local_ned.h
	//o power_status.h
	//o radio_status.h
	//- raw_imu.h
	//- raw_pressure.h
	//x rc_channels.h
	//x rc_channels_override.h
	//x rc_channels_raw.h
	//x rc_channels_scaled.h
	//x request_data_stream.h
	//x resource_request.h
	//x safety_allowed_area.h
	//x safety_set_allowed_area.h
	//x scaled_imu2.h
	//x scaled_imu3.h
	//- scaled_imu.h
	//x scaled_pressure2.h
	//x scaled_pressure3.h
	//- scaled_pressure.h
	//x serial_control.h
	//- servo_output_raw.h
	//x set_actuator_control_target.h
	//- set_attitude_target.h
	//x set_gps_global_origin.h
	//x set_home_position.h
	//x set_mode.h
	//x set_position_target_global_int.h
	//x set_position_target_local_ned.h
	//o sim_state.h
	//- statustext.h
	//- sys_status.h
	//- system_time.h
	//x terrain_check.h
	//x terrain_data.h
	//x terrain_report.h
	//x terrain_request.h
	//- timesync.h
	//x v2_extension.h
	//x vfr_hud.h
	//x vibration.h
	//x vicon_position_estimate.h
	//x vision_position_estimate.h
	//x vision_speed_estimate.h
	//x wind_cov.h
