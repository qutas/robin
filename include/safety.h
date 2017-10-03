#pragma once

#include <stdbool.h>
#include "mavlink/mavlink_types.h"
#include "params.h"

#define MAV_STATE_NUM_STATES (8 + 1)
#define MAV_STATE_NAME_LEN 10
#define MAV_MODE_NUM_MODES (7 + 1)
#define MAV_MODE_NAME_LEN 10

typedef enum {
	MAIN_MODE_MANUAL = 1,
	MAIN_MODE_ALTCTL,
	MAIN_MODE_POSCTL,
	MAIN_MODE_AUTO,
	MAIN_MODE_ACRO,
	MAIN_MODE_OFFBOARD,
	MAIN_MODE_STABILIZED,
	MAIN_MODE_RATTITUDE
} compat_px4_main_mode_t;

//General UAV health status
typedef enum {
	SYSTEM_HEALTH_UNKNOWN,	//Have not made contact with sensor
	SYSTEM_HEALTH_OK,		//Operation is fine
	SYSTEM_HEALTH_TIMEOUT,	//Stopped recieving messages
	SYSTEM_HEALTH_ERROR,	//Error in readings
	SYSTEM_HEALTH_INVALID
} safety_health_t;

typedef struct {
	uint8_t health;	//Set with safety_health_t
	uint32_t last_read;
	uint32_t count;
	param_id_t param_stream_count;
	param_id_t param_timeout;
	char name[25];
} timeout_status_t;

typedef struct {
	timeout_status_t imu;
	timeout_status_t mag;
	timeout_status_t baro;
	timeout_status_t sonar;
	timeout_status_t ext_pose;
	timeout_status_t offboard_heartbeat;
	timeout_status_t offboard_control;
} safety_sensor_status_t;

//List of failures
//True means system is OK
typedef struct {
	uint8_t health;	//Set with safety_health_t

	//Note: Mirrors MAVLINK configuration
		//MAV_STATE_UNINIT:			System start
		//MAV_STATE_BOOT:			System booting/initializing
		//MAV_STATE_CALIBRATING:	System calibration active
		//MAV_STATE_STANDBY:		System ready, waiting for arming and/or command input
		//MAV_STATE_ACTIVE:			System fully armed (and may be flying) (Don't allow switch to without MAV_MODE_FLAG_SAFETY_ARMED set)
		//MAV_STATE_CRITICAL:		Something is giving errors, should be attempting graceful failsafe maneouvres
		//MAV_STATE_EMERGENCY:		System failure, should be attempting hard failsafe maneouvres, keep comms up, but should not allow recover without reboot
		//MAV_STATE_POWEROFF:		Shutting down (write EEPROM/SD card logs, alert GCS?)
	uint8_t state;	//Set with MAV_STATE

	uint8_t mode;	//Purely for reporting to gcs, no actual use
	bool parameters;
	bool safety_button_status;	//Safety button to engage and disengage motor output
	bool arm_status;	//Safety button to engage and disengage motor output
	safety_sensor_status_t sensors;
} system_status_t;

typedef enum {
	STATUS_BUZZER_QUIET,
	STATUS_BUZZER_SUCCESS,
	STATUS_BUZZER_FAILURE,
	STATUS_BUZZER_FAILSAFE
} status_buzzer_modes_t;

typedef struct {
	GPIO_TypeDef *gpio_p;
	uint16_t pin;

	uint32_t period_us;
	uint32_t length_us;
	uint32_t last_pulse;
} status_led_t;

typedef struct {
	GPIO_TypeDef *gpio_p;
	uint16_t pin;

	status_buzzer_modes_t mode;
	bool state;
	uint8_t beep_steps;
	uint32_t period_high_us;
	uint32_t period_low_us;
	uint32_t length_us;
	uint32_t last_pulse;
	uint32_t last_beep;
} status_buzzer_t;

extern char mav_state_names[MAV_STATE_NUM_STATES][MAV_STATE_NAME_LEN];

extern system_status_t _system_status;
extern uint8_t _system_operation_control;

void safety_init( void );

bool safety_is_armed( void );
bool safety_request_state( uint8_t req_state );
bool safety_request_arm( void );
bool safety_request_disarm( void );

void status_buzzer_success( void );
void status_buzzer_failure( void );

uint32_t compat_encode_px4_main_mode( uint8_t main_mode );
uint8_t compat_decode_px4_main_mode( uint32_t mode );

void safety_update_sensor( timeout_status_t *sensor);

void safety_run( uint32_t time_now );

