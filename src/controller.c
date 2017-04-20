#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "breezystm32.h"

#include "params.h"
#include "estimator.h"
#include "controller.h"
#include "pid_controller.h"

state_t _state_estimator;
command_input_t _command_input;
control_output_t _control_output;

pid_controller_t _pid_roll_rate;
pid_controller_t _pid_pitch_rate;
pid_controller_t _pid_yaw_rate;

//pid_controller_t pid_roll;
//pid_controller_t pid_pitch;
//pid_controller_t pid_yaw;
//pid_controller_t pid_altitude;

void controller_reset(void) {
	pid_reset(&_pid_roll_rate, _state_estimator.p);
	pid_reset(&_pid_pitch_rate, _state_estimator.q);
	pid_reset(&_pid_yaw_rate, _state_estimator.r);
}

void controller_init(void) {
/*
  pid_init(&pid_roll,
           PARAM_PID_ROLL_ANGLE_P,
           PARAM_PID_ROLL_ANGLE_I,
           PARAM_PID_ROLL_ANGLE_D,
           _current_state.phi,
           _current_state.p,
           _combined_control.x.value,
           _command.x,
           get_param_int(PARAM_MAX_COMMAND)/2.0f,
           -1.0f*get_param_int(PARAM_MAX_COMMAND)/2.0f);

  pid_init(&pid_pitch,
           PARAM_PID_PITCH_ANGLE_P,
           PARAM_PID_PITCH_ANGLE_I,
           PARAM_PID_PITCH_ANGLE_D,
           &_current_state.theta,
           &_current_state.q,
           &_combined_control.y.value,
           &_command.y,
           get_param_int(PARAM_MAX_COMMAND)/2.0f,
           -1.0f*get_param_int(PARAM_MAX_COMMAND)/2.0f);
*/
pid_init(&_pid_roll_rate,
		 get_param_float(PARAM_PID_ROLL_RATE_P),
		 get_param_float(PARAM_PID_ROLL_RATE_I),
		 get_param_float(PARAM_PID_ROLL_RATE_D),
		 get_param_float(PARAM_PID_TAU),
		 _state_estimator.p,
		 0,
		 0,
		 0,
		 -get_param_float(PARAM_MAX_ROLL_RATE),
		 get_param_float(PARAM_MAX_ROLL_RATE));

pid_init(&_pid_pitch_rate,
		 get_param_float(PARAM_PID_PITCH_RATE_P),
		 get_param_float(PARAM_PID_PITCH_RATE_I),
		 get_param_float(PARAM_PID_PITCH_RATE_D),
		 get_param_float(PARAM_PID_TAU),
		 _state_estimator.q,
		 0,
		 0,
		 0,
		 -get_param_float(PARAM_MAX_PITCH_RATE),
		 get_param_float(PARAM_MAX_PITCH_RATE));

pid_init(&_pid_yaw_rate,
		 get_param_float(PARAM_PID_YAW_RATE_P),
		 get_param_float(PARAM_PID_YAW_RATE_I),
		 get_param_float(PARAM_PID_YAW_RATE_D),
		 get_param_float(PARAM_PID_TAU),
		 _state_estimator.r,
		 0,
		 0,
		 0,
		 -get_param_float(PARAM_MAX_YAW_RATE),
		 get_param_float(PARAM_MAX_YAW_RATE));
/*
  pid_init(&pid_altitude,
           PARAM_PID_ALT_P,
           PARAM_PID_ALT_I,
           PARAM_PID_ALT_D,
           &_current_state.altitude,
           NULL,
           &_combined_control.F.value,
           &_command.F,
           get_param_int(PARAM_MAX_COMMAND),
           0.0f);
*/

	_command_input.r = 0;
	_command_input.p = 0;
	_command_input.y = 0;
	_command_input.q.w = 1;
	_command_input.q.x = 0;
	_command_input.q.y = 0;
	_command_input.q.z = 0;
	_command_input.T = 0;
	_command_input.input_mask |= CMD_IN_IGNORE_ATTITUDE;	//Set it to just hold rpy rates (as this skips unnessessary computing during boot, and is possibly safer)

	_control_output.r = 0;
	_control_output.p = 0;
	_control_output.y = 0;
	_control_output.T = 0;
}

void controller_set_input_failsafe(void) {
	_command_input.r = 0;
	_command_input.p = 0;
	_command_input.y = 0;
	_command_input.q.w = 1;
	_command_input.q.x = 0;
	_command_input.q.y = 0;
	_command_input.q.z = 0;
	_command_input.T = get_param_float(PARAM_FAILSAFE_THROTTLE);
	_command_input.input_mask |= CMD_IN_IGNORE_ATTITUDE;	//Set it to just hold rpy rates (as this skips unnessessary computing during boot, and is possibly safer)
}

static vector3_t rate_goals_from_attitude(const quaternion_t *q_sp, const quaternion_t *q_current) {
/*		mf16 I;
		I.rows = 3;
		I.columns = 3;
		I.data[0][0] = CONST_ONE;
		I.data[1][1] = CONST_ONE;
		I.data[2][2] = CONST_ONE;

		//Method derived from px4 attitude controller:
		//DCM from for state and setpoint
		mf16 R_sp;
		qf16_to_matrix(&R_sp, q_sp);
		mf16 R;
		qf16_to_matrix(&R, q_current);

		//Calculate shortest path to goal rotation without yaw (as it's slower than roll/pitch)
		v3d R_z;
		v3d R_sp_z;
		R_z.x = R.data[0][2];
		R_z.y = R.data[1][2];
		R_z.z = R.data[2][2];
		R_sp_z.x = R_sp.data[0][2];
		R_sp_z.y = R_sp.data[1][2];
		R_sp_z.z = R_sp.data[2][2];


		//px4: axis and sin(angle) of desired rotation
		//px4: math::Vector<3> e_R = R.transposed() * (R_z % R_sp_z);
		mf16 Rt;
		mf16_transpose(&Rt, &R);

		v3d R_z_c;
		v3d_cross(&R_z_c, &R_z, &R_sp_z);
		mf16 R_z_c_m;
		R_z_c_m.rows = 3;
		R_z_c_m.columns = 1;
		R_z_c_m.data[0][0] = R_z_c.x;
		R_z_c_m.data[1][0] = R_z_c.y;
		R_z_c_m.data[2][0] = R_z_c.z;

		mf16 e_R_mat;
		v3d e_R;
		mf16_mul(&e_R_mat, &Rt, &R_z_c_m);
		e_R.x = e_R_mat.data[0][0];
		e_R.y = e_R_mat.data[1][0];
		e_R.z = e_R_mat.data[2][0];

		fix16_t e_R_z_sin = v3d_norm(&e_R);
		fix16_t e_R_z_cos = v3d_dot(&R_z, &R_sp_z);	//TODO: make sure "float e_R_z_cos = R_z * R_sp_z;" means dot product

		//px4: calculate weight for yaw control
		fix16_t yaw_w = R_sp.data[2][2] * R_sp.data[2][2];

		//px4: calculate rotation matrix after roll/pitch only rotation
		mf16 R_rp;
		R_rp.rows = 3;
		R_rp.columns = 3;

		if(e_R_z_sin > 0) {
			//px4: get axis-angle representation
			fix16_t e_R_z_angle = fix16_atan2(e_R_z_sin, e_R_z_cos);
			v3d e_R_z_axis;
			v3d_div_s(&e_R_z_axis, &e_R, e_R_z_sin);

			v3d_mul_s(&e_R, &e_R_z_axis, e_R_z_angle);

			//px4: cross product matrix for e_R_axis
			mf16 e_R_cp;
			e_R_cp.rows = 3;
			e_R_cp.columns = 3;
			//mf16_fill(&e_R_cp, 0);
			e_R_cp.data[0][1] = -e_R_z_axis.z;
			e_R_cp.data[0][2] = e_R_z_axis.y;
			e_R_cp.data[1][0] = e_R_z_axis.z;
			e_R_cp.data[1][2] = -e_R_z_axis.x;
			e_R_cp.data[2][0] = -e_R_z_axis.y;
			e_R_cp.data[2][1] = e_R_z_axis.x;

			//px4: rotation matrix for roll/pitch only rotation
			mf16 e_R_cp_2;
			mf16 e_R_cp_z_sin;
			mf16 e_R_add_temp;

			mf16_mul_s(&e_R_cp_z_sin, &e_R_cp, e_R_z_sin);
			mf16_mul(&e_R_cp_2, &e_R_cp, &e_R_cp);
			mf16_mul_s(&e_R_cp_2, &e_R_cp_2, fix16_sub(CONST_ONE, e_R_z_cos));

			mf16_add(&e_R_add_temp, &I, &e_R_cp_z_sin);
			mf16_add(&e_R_add_temp, &e_R_add_temp, &e_R_cp_2);

			mf16_mul(&R_rp, &R, &e_R_add_temp);
		} else {
			//px4: zero roll/pitch rotation
			R_rp = R;
		}

		//px4: R_rp and R_sp has the same Z axis, calculate yaw error
		v3d R_sp_x;
		R_sp_x.x = R_sp.data[0][0];
		R_sp_x.y = R_sp.data[1][0];
		R_sp_x.z = R_sp.data[2][0];

		v3d R_rp_x;
		R_rp_x.x = R_rp.data[0][0];
		R_rp_x.y = R_rp.data[1][0];
		R_rp_x.z = R_rp.data[2][0];

		v3d R_rp_c_sp;
		v3d_cross(&R_rp_c_sp, &R_rp_x, &R_sp_x);

		e_R.z = fix16_mul(fix16_atan2(v3d_dot(&R_rp_c_sp, &R_sp_z), v3d_dot(&R_rp_x, &R_sp_x)), yaw_w);

		if(e_R_z_cos < 0) {
			//px4: for large thrust vector rotations use another rotation method:
			//px4: calculate angle and axis for R -> R_sp rotation directly
			qf16 q_error;
			mf16 Rt_R_sp;
			mf16_mul(&Rt_R_sp, &Rt, &R_sp);
			matrix_to_qf16(&q_error, &Rt_R_sp);
			v3d e_R_d;

			if(q_error.a >= 0) {
				v3d temp_vec;
				temp_vec.x = q_error.b;
				temp_vec.y = q_error.c;
				temp_vec.z = q_error.d;

				v3d_mul_s(&e_R_d, &temp_vec, CONST_TWO);
			} else {
				v3d temp_vec;
				temp_vec.x = -q_error.b;
				temp_vec.y = -q_error.c;
				temp_vec.z = -q_error.d;

				v3d_mul_s(&e_R_d, &temp_vec, CONST_TWO);
			}

			//px4: use fusion of Z axis based rotation and direct rotation
			fix16_t direct_w = fix16_mul(fix16_mul(e_R_z_cos, e_R_z_cos), yaw_w);

			//px4: e_R = e_R * (1.0f - direct_w) + e_R_d * direct_w;
			v3d_mul_s(&e_R, &e_R, fix16_sub(CONST_ONE, direct_w));
			v3d_mul_s(&e_R_d, &e_R_d, direct_w);
			v3d_add(&e_R, &e_R, &e_R_d);
		}

		//px4: calculate angular rates setpoint
		v3d rates_sp;
		rates_sp.x = fix16_mul(get_param_fix16(PARAM_PID_ROLL_ANGLE_P), e_R.x);
		rates_sp.y = fix16_mul(get_param_fix16(PARAM_PID_PITCH_ANGLE_P), e_R.y);
		rates_sp.z = fix16_mul(get_param_fix16(PARAM_PID_YAW_ANGLE_P), e_R.z);

		//px4: feed forward yaw setpoint rate	//TODO:?
		//rates_sp.z += _v_att_sp.yaw_sp_move_rate * yaw_w * _params.yaw_ff;

		//px4: constrain rates to set params
		v3d rates_sp_sat;
		rates_sp_sat.x = fix16_constrain(rates_sp.x, -get_param_fix16(PARAM_MAX_ROLL_RATE), get_param_fix16(PARAM_MAX_ROLL_RATE));
		rates_sp_sat.y = fix16_constrain(rates_sp.y, -get_param_fix16(PARAM_MAX_PITCH_RATE), get_param_fix16(PARAM_MAX_PITCH_RATE));
		rates_sp_sat.z = fix16_constrain(rates_sp.z, -get_param_fix16(PARAM_MAX_YAW_RATE), get_param_fix16(PARAM_MAX_YAW_RATE));

		return rates_sp_sat;
*/

	vector3_t empty;
	return empty;
}

void controller_run( uint32_t time_now ) {
	//Variables that store the computed attitude goal rates
	float goal_r = 0;
	float goal_p = 0;
	float goal_y = 0;
	float goal_throttle = 0;

	//==-- Attitude Control
	//If we should listen to attitude input
	if( !(_command_input.input_mask & CMD_IN_IGNORE_ATTITUDE) ) {
		vector3_t rates_sp = rate_goals_from_attitude(&_command_input.q, &_state_estimator.attitude);

		goal_r = rates_sp.x;
		goal_p = rates_sp.y;
		goal_y = rates_sp.z;
	}

	//==-- Rate Control PIDs
	//Roll Rate
	if( !(_command_input.input_mask & CMD_IN_IGNORE_ROLL_RATE) ) {
		//Use the commanded roll rate goal
		goal_r = _command_input.r;

	}

	//Pitch Rate
	if( !(_command_input.input_mask & CMD_IN_IGNORE_PITCH_RATE) ) {
		//Use the commanded pitch rate goal
		goal_p = _command_input.p;
	}

	//Yaw Rate
	if( !(_command_input.input_mask & CMD_IN_IGNORE_YAW_RATE) ) {
		//Use the commanded yaw rate goal
		goal_y = _command_input.y;
	}

	//Rate PID Controllers
	_control_output.r = pid_step(&_pid_roll_rate, time_now, goal_r, _state_estimator.p, 0, false);
	_control_output.p = pid_step(&_pid_pitch_rate, time_now, goal_p, _state_estimator.q, 0, false);
	_control_output.y = pid_step(&_pid_yaw_rate, time_now, goal_y, _state_estimator.r, 0, false);

	//==-- Throttle Control
	//TODO: Could do something here for altitude hold mode if enabled

	//Trottle
	if( !(_command_input.input_mask & CMD_IN_IGNORE_THROTTLE) ) {
		//Use the commanded throttle
		goal_throttle = _command_input.T;
	}

	_control_output.T = goal_throttle;
}

#ifdef __cplusplus
}
#endif
