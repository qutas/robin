/*
   pwm.h : PWM support for STM32F103

   Adapted from https://github.com/multiwii/baseflight/blob/master/src/pwm.h

   This file is part of BreezySTM32.

   BreezySTM32 is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   BreezySTM32 is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with BreezySTM32.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "io_type.h"

#define MAX_SERVOS  8
#define MAX_INPUTS  8
#define PULSE_MIN   (750)       // minimum PWM pulse width which is considered valid
#define PULSE_MAX   (2250)      // maximum PWM pulse width which is considered valid
#define PULSE_DIGITAL_OFF 1000
#define PULSE_DIGITAL_ON 2000

void pwmInit(io_def_t *io_map, bool usePwmFilter, uint32_t motorPwmRate, uint32_t servoPwmRate, uint16_t idlePulseUsec);
void pwmWriteMotor(uint8_t index, uint16_t value);
uint16_t pwmRead(uint8_t channel);