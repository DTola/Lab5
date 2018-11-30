/*
 * RGBLeds.h
 *
 *  Created on: Sep 7, 2018
 *      Author: Daniel
 */

#ifndef RGBLEDS_H_
#define RGBLEDS_H_

/* Enums for RGB LEDs */
typedef enum device{
    BLUE = 0,
    GREEN = 1,
    RED = 2
} unit_desig;

/*
 * unit = R,G,B
 * PWM_DATA = Duty Cycle
 *
 * NOT CREATED RN
 */
//static void LP3943_ColorSet(uint32_t unit, uint32_t PWM_DATA);

/*
 * unit = R, G, B
 * LED_DATA = LEDS to turn on and off for specified color/unit
 */
void LP3943_LedModeSet(uint32_t unit, uint16_t LED_DATA);

/*
 * Sets up UCB registers
 */
void init_RGBLEDS();

#endif /* RGBLEDS_H_ */
