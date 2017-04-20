#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "breezystm32.h"
#include "math_types.h"

//TODO: Common header file for defines like this
//#define BOARD_REV 2

// global variable declarations
//extern vector_t _accel;
//extern vector_t _gyro;
//extern float _imu_temperature;
//extern uint32_t _imu_time;
//extern bool _imu_ready;

//extern bool _diff_pressure_present;
//extern int16_t _diff_pressure;
//extern int16_t _temperature;

//extern bool _baro_present;
//extern int16_t _baro_pressure;
//extern int16_t _baro_temperature;

//extern bool _sonar_present;
//extern int16_t _sonar_range;
//extern uint32_t _sonar_time;

typedef struct {
	bool present;		//If the sensor is plugged in or not
	bool new_data;	//If there is new data ready
	uint32_t time_read;	//Time measured
} sensor_status_t;

typedef struct {
	bool present;

	uint32_t start;			//Loop start time
	uint32_t end;			//Loop end time
	uint32_t dt;			//Time sepent this loop

	uint32_t counter;		//Times the data from this sensor has been collated
	uint32_t average_time;	//Sum of dt
	uint32_t max;			//Maximum dt so far
	uint32_t min;			//Minimum dt so far

	uint32_t imu_time_read;

	int64_t rt_offset_ns;
	float rt_drift;
	uint64_t rt_ts_last;
	uint64_t rt_tc_last;
	uint32_t rt_sync_last;
} sensor_readings_clock_t;

typedef struct {
	sensor_status_t status;

    int16_t accel_raw[3];
    int16_t gyro_raw[3];
    volatile int16_t temp_raw;

	vector3_t accel;		//Vector of accel data
	vector3_t gyro;		//Vector of gyro data
	float temperature;	//Sensor temperature reading

	float accel_scale;	//Scale to correct raw accel data
	float gyro_scale;	//Scale to correct raw gyro data
} sensor_readings_imu_t;

typedef struct {
	sensor_status_t status;

	int16_t pressure;		//Barometer reading
	int16_t temperature;	//Sensor temperature reading
} sensor_readings_barometer_t;

typedef struct {
	sensor_status_t status;

	int16_t range;	//Measured range
} sensor_readings_sonar_t;

typedef struct {
	sensor_status_t status;

	GPIO_TypeDef *gpio_p;
	uint16_t pin;

	bool state;			//Measured state
	uint32_t period_us;	//Measured range

	uint32_t time_db_read;	//Initial dounce read time
	uint32_t period_db_us;	//Debounce period
	bool state_db;			//State at debounce read
} sensor_readings_safety_button_t;

typedef struct {
	sensor_readings_clock_t clock;
	sensor_readings_imu_t imu;
	sensor_readings_barometer_t baro;
	sensor_readings_sonar_t sonar;
	sensor_readings_safety_button_t safety_button;
} sensor_readings_t;

typedef enum {
	SENSOR_CAL_NONE = 0,
	SENSOR_CAL_GYRO = 1,
	SENSOR_CAL_MAG = 2,
	SENSOR_CAL_BARO = 4,
	SENSOR_CAL_RC = 8,		//Calibrate ESCs?
	SENSOR_CAL_ACCEL = 16,
	SENSOR_CAL_INTER = 32,	//TODO: Implement this
	SENSOR_CAL_ALL = 128
} sensor_calibration_request_t;

typedef struct {
	uint16_t count;
	float sum_x;
	float sum_y;
	float sum_z;
} sensor_calibration_gyro_data_t;

typedef struct {
	uint16_t count;
	int32_t z_up_sum;
	int32_t z_down_sum;
	int16_t z_up_av;
	int16_t z_down_av;
} sensor_calibration_gravity_data_t;

typedef struct {
	uint16_t count;
	int32_t sum_x;
	int32_t sum_y;
	int32_t sum_z;
	int32_t sum_t;

	uint16_t acc1G;
	float temp_scale;
	float temp_shift;
//	fix16_t sum_x;
//	fix16_t sum_y;
//	fix16_t sum_z;
//	fix16_t sum_t;
} sensor_calibration_accel_data_t;

typedef struct {
	sensor_calibration_gyro_data_t gyro;
	sensor_calibration_accel_data_t accel;
} sensor_calibration_data_t;

extern uint8_t _sensor_calibration;
extern sensor_readings_t _sensors;
extern sensor_calibration_data_t _sensor_cal_data;

static const float CONST_GRAVITY = 0x0009CE80; //Is equal to 9.80665 (Positive!) in Q16.16

// function declarations
void sensors_init(void);
bool sensors_read(void);

//void sensors_poll(void);
uint32_t sensors_clock_ls_get(void);	//Get time at loop start
void sensors_clock_ls_set(uint32_t time_us);	//Set time at loop start
void sensors_clock_update(uint32_t time_us);	//Update the timing variable (used at the end of the loop)

int64_t sensors_clock_smooth_time_skew(int64_t tc, int64_t tn);
float sensors_clock_smooth_time_drift(float tc, float tn);
uint64_t sensors_clock_rt_get(void);	//Get the current time syncronized real time (good for logging)

uint32_t sensors_clock_imu_int_get(void);	//Get the time of the latest imu interrupt

bool sensors_update(uint32_t time_us);
bool sensors_calibrate(void);

//bool calibrate_acc(void);
//bool calibrate_gyro(void);
