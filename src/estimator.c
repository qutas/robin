#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "estimator.h"
#include "params.h"
#include "safety.h"
#include "sensors.h"

#include "drivers/drv_system.h"
#include "fix16.h"
#include "fixextra.h"
#include "fixquat.h"
#include "fixvector3d.h"

state_t _state_estimator;
sensor_readings_t _sensors;


//static v3d w1;	 //Integrator for quad int
//static v3d w2;	 //Integrator for quad int
static v3d w_bias_;	  //Integrator for adaptive bias
static qf16 q_hat_; //Attitude estimate
//static v3d g_; //Gravity vector

static uint32_t time_last_;
static uint32_t init_time_;

static v3d accel_lpf_;
static v3d gyro_lpf_;

void estimator_init( void ) {
	_state_estimator.ax = 0;
	_state_estimator.ay = 0;
	_state_estimator.az = 0;
	_state_estimator.p = 0;
	_state_estimator.q = 0;
	_state_estimator.r = 0;
	_state_estimator.attitude.a = _fc_1;
	_state_estimator.attitude.b = 0;
	_state_estimator.attitude.c = 0;
	_state_estimator.attitude.d = 0;

	q_hat_.a = _fc_1;
	q_hat_.b = 0;
	q_hat_.c = 0;
	q_hat_.d = 0;

	//g_.x = 0;
	//g_.y = 0;
	//g_.z = _fc_1;

	reset_adaptive_gyro_bias();

	accel_lpf_.x = 0;
	accel_lpf_.y = 0;
	accel_lpf_.z = _fc_gravity;

	gyro_lpf_.x = 0;
	gyro_lpf_.y = 0;
	gyro_lpf_.z = 0;

	init_time_ = get_param_uint( PARAM_EST_INIT_TIME ) * 1000; //nano->microseconds

	time_last_ = 0;
}

void reset_adaptive_gyro_bias() {
	w_bias_.x = 0;
	w_bias_.y = 0;
	w_bias_.z = 0;
}

static void lpf_update( v3d* accel, v3d* gyro ) {
	//value_lpf = ((1 - alpha) * value) + (alpha * value_lpf);
	fix16_t alpha_acc = get_param_fix16( PARAM_ACC_ALPHA );
	accel_lpf_.x = fix16_add( fix16_mul( fix16_sub( _fc_1, alpha_acc ), accel_lpf_.x ), fix16_mul( alpha_acc, accel->x ) );
	accel_lpf_.y = fix16_add( fix16_mul( fix16_sub( _fc_1, alpha_acc ), accel_lpf_.y ), fix16_mul( alpha_acc, accel->y ) );
	accel_lpf_.z = fix16_add( fix16_mul( fix16_sub( _fc_1, alpha_acc ), accel_lpf_.z ), fix16_mul( alpha_acc, accel->z ) );

	fix16_t alpha_gyro = get_param_fix16( PARAM_GYRO_ALPHA );
	gyro_lpf_.x = fix16_add( fix16_mul( fix16_sub( _fc_1, alpha_gyro ), gyro_lpf_.x ), fix16_mul( alpha_gyro, gyro->x ) );
	gyro_lpf_.y = fix16_add( fix16_mul( fix16_sub( _fc_1, alpha_gyro ), gyro_lpf_.y ), fix16_mul( alpha_gyro, gyro->y ) );
	gyro_lpf_.z = fix16_add( fix16_mul( fix16_sub( _fc_1, alpha_gyro ), gyro_lpf_.z ), fix16_mul( alpha_gyro, gyro->z ) );
}

/*
static void corr_heading_estimate( v3d* corr, const qf16* q, const v3d* m, const fix16_t alpha, const fix16_t mag_decl ) {
	v3d mw;	//World measurement
	qf16_rotate( &mw, q, m );
	fix16_t mocap_hdg_err = wrap_pi(fix16_sub(fix16_atan2(mw.y,mw.x), mag_decl));
	v3d yaw_c;
	yaw_c.x = 0;
	yaw_c.y = 0;
	yaw_c.z = -mocap_hdg_err;
	// Project correction to body frame
	qf16 qi;
	qf16_inverse(&qi,q);
	v3d err;
	qf16_rotate( &err, &qi, &yaw_c );
	v3d_mul_s(corr, &err, alpha);
}
*/

//  212 | 195 us (acc and gyro only, not exp propagation no quadratic integration)
static void estimator_update( uint32_t time_now, v3d* accel, v3d* gyro ) {
	//XXX: This will exit on the first loop, not a nice way of doing it though
	if ( time_last_ == 0 ) {
		time_last_ = time_now;
		return;
	}

	//Converts dt from micros to secs
	fix16_t dt = fix16_from_float( 1e-6 * (float)( time_now - time_last_ ) );
	time_last_ = time_now;

	//Gains for accelerometer correction and adaptive bias
	fix16_t w_a_kp = get_param_fix16( PARAM_EST_ACC_KP );
	fix16_t w_b_ki = get_param_fix16( PARAM_EST_BIAS_KI );
	//Crank up the gains for the first few seconds for quick convergence
	if ( time_now < init_time_ ) {
		w_a_kp = fix16_smul( w_a_kp, _fc_10 );
		w_b_ki = fix16_smul( w_b_ki, _fc_10 );
	}

	//Run LPF to reject a lot of noise
	lpf_update( accel, gyro );

	//Omega correction
	//These terms are used later to correct our attitude
	//using additional sensor measurements
	v3d w_cor;
	w_cor.x = 0;
	w_cor.y = 0;
	w_cor.z = 0;

	//Heading correction
	/*
	if ( ( _system_status.sensors.ext_pose.health == SYSTEM_HEALTH_OK ) && _sensors.ext_pose.status.new_data ) {
		fuse_heading_estimate( &q_hat_, &_sensors.ext_pose.q, get_param_fix16( PARAM_FUSE_EXT_HDG_W ) );
		_sensors.ext_pose.status.new_data = false;
	} else if ( ( _system_status.sensors.mag.health == SYSTEM_HEALTH_OK ) && _sensors.mag.status.new_data ) {
		fuse_heading_estimate( &q_hat_, &_sensors.mag.q, get_param_fix16( PARAM_FUSE_MAG_HDG_W ) );
		_sensors.mag.status.new_data = false;
	}
	*/

	//Accelerometer Correction
	//The reading must be reasonably close to something that resembles a hover
	fix16_t a_sqrd_norm = v3d_sq_norm( &accel_lpf_ );
	if ( ( get_param_fix16( PARAM_EST_ACC_KP ) > 0 ) && ( a_sqrd_norm < fix16_mul( fix16_sq( _fc_1_15 ), fix16_sq( _fc_gravity ) ) ) && ( a_sqrd_norm > fix16_mul( fix16_sq( _fc_0_85 ), fix16_sq( _fc_gravity ) ) ) ) {
		v3d w_acc;

		v3d b_z;
		qf16 q_hat_inv;
		qf16_inverse( &q_hat_inv, &q_hat_ ); //Need to invert to bring the vector into the body frame
		qf16_dcm_z( &b_z, &q_hat_inv );

		v3d a_z;
		v3d_normalize( &a_z, &accel_lpf_ );

		v3d e_R;
		v3d_cross(&e_R, &b_z, &a_z);
		v3d_mul_s(&w_acc, &e_R, w_a_kp);

		//Add the accel correction to the running correction term
		v3d_add(&w_cor, &w_cor, &w_acc);
	}

	//Integrate adaptive bias
	//Shoud be done just before west calculation
	if ( get_param_fix16( PARAM_EST_BIAS_KI ) ) {
		v3d w_cor_i;

		//Calculate the bias error for this time step
		const fix16_t i_max = get_param_fix16(PARAM_EST_BIAS_MAX);
		v3d_mul_s(&w_cor_i, &w_cor, fix16_mul( w_b_ki ,dt ) );

		//Constrain the terms
		w_cor_i.x = fix16_constrain(w_cor_i.x, -i_max, i_max);
		w_cor_i.y = fix16_constrain(w_cor_i.y, -i_max, i_max);
		w_cor_i.z = fix16_constrain(w_cor_i.z, -i_max, i_max);

		//Integrate the bias terms
		v3d_add(&w_bias_, &w_bias_, &w_cor_i);
	} else {
		//Otherwise reset the bias terms, to be safe
		reset_adaptive_gyro_bias();
	}

	//Estimate rotation rates (with the adaptive bias term)
	v3d w_est;
	v3d_add(&w_est, &gyro_lpf_, &w_bias_);

	// Build the composite omega vector for dynamics propagation
	//w_bar = w_est + w_cor;
	v3d w_bar;
	v3d_add( &w_bar, &w_est, &w_cor );

	//Propagate dynamics
	//q_hat += q_hat.derivative1(wbar) * dt;

	qf16 q_w;	//q_hat.derivative1(wbar)
	qf16_from_v3d(&q_w, &w_bar, 0);
	qf16_mul(&q_w,&q_hat_,&q_w);
	qf16_mul_s(&q_w,&q_w,_fc_0_5);

	qf16_mul_s(&q_w, &q_w, dt);

	qf16_add(&q_hat_, &q_hat_, &q_w);

	qf16_normalize_to_unit(&q_hat_, &q_hat_);

	//Perform level horizon measurements
	if ( get_param_uint( PARAM_EST_USE_LEVEL_HORIZON ) ) {
		qf16 q_lh;
		qf16 q_lh_inv;
		qf16 q_hat_lvl;
		q_lh.a = get_param_fix16( PARAM_EST_LEVEL_HORIZON_W );
		q_lh.b = get_param_fix16( PARAM_EST_LEVEL_HORIZON_X );
		q_lh.c = get_param_fix16( PARAM_EST_LEVEL_HORIZON_Y );
		q_lh.d = get_param_fix16( PARAM_EST_LEVEL_HORIZON_Z );
		qf16_normalize_to_unit( &q_lh, &q_lh );
		qf16_inverse( &q_lh_inv, &q_lh );

		qf16_mul( &q_hat_lvl, &q_hat_, &q_lh_inv );
		qf16_normalize_to_unit( &_state_estimator.attitude, &q_hat_lvl );
	} else {
		_state_estimator.attitude = q_hat_;
	}

	_state_estimator.p = w_est.x;
	_state_estimator.q = w_est.y;
	_state_estimator.r = w_est.z;

	_state_estimator.ax = accel_lpf_.x;
	_state_estimator.ay = accel_lpf_.y;
	_state_estimator.az = accel_lpf_.z;
}

void estimator_update_sensors( uint32_t now ) {
	estimator_update( now, &_sensors.imu.accel, &_sensors.imu.gyro );

	_sensors.imu.status.new_data = false;
}

void estimator_update_hil( uint32_t now ) {
	estimator_update( now, &_sensors.hil.accel, &_sensors.hil.gyro );

	_sensors.hil.status.new_data = false;
}

void estimator_calc_lvl_horz( qf16* qlh ) {
	mf16 ratt;
	mf16 rlh;
	v3d bx;
	v3d by;
	v3d bz;
	v3d lhx;
	v3d lhy;
	v3d lhz;
	//Get DCM of current attitude estimate and extract axes
	qf16_to_matrix( &ratt, &q_hat_ );
	dcm_to_basis( &bx, &by, &bz, &ratt );

	//Build the new rotation matrix with pure roll/pitch
	lhy.x = 0;
	lhy.y = _fc_1;
	lhy.z = 0;
	lhz = bz;

	v3d_cross( &lhx, &lhy, &lhz );
	v3d_normalize( &lhx, &lhx );

	v3d_cross( &lhy, &lhz, &lhx );
	v3d_normalize( &lhy, &lhy );

	dcm_from_basis( &rlh, &lhx, &lhy, &lhz );

	matrix_to_qf16( qlh, &rlh );
	qf16_normalize_to_unit( qlh, qlh );
}

#ifdef __cplusplus
}
#endif
