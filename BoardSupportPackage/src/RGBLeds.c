/*
 * RGBLeds.c
 *
 *  Created on: Sep 7, 2018
 *      Author: Daniel
 */
#include "msp.h"

//void Delay2(uint16_t loops){
//    int i = 0;
//    int j = 0;
//    for(i = 0; i < loops; i++){
//        for(j = 0; j < 20; j++);
//    }
//}

void LP3943_LedModeSet(uint32_t unit, uint16_t LED_DATA){
    /*
     * Sets LEDS to mode:
     *  -on
     *  -off
     *  -PWM1
     *  -PWM2
     *
     * Unit:
     *  -2: RED
     *  -0: BLUE
     *  -1: GREEN
     *
     * Registers:
     *  -LS0: LED0-3 Select
     *  -LS1: LED4-7 Select
     *  -LS2: LED8-11 Select
     *  -LS3: LED12-16 Select
     */

    //Generate the data
    uint8_t LED_DATA_TX_0 = 0x00;
    uint8_t LED_DATA_TX_1 = 0x00;
    uint8_t LED_DATA_TX_2 = 0x00;
    uint8_t LED_DATA_TX_3 = 0x00;

    uint16_t check = 0x0001;
    uint16_t temp = 0x01;
    int j = 0;
    for(j = 0; j < 4; j++){
        if((LED_DATA & check) == check){
            LED_DATA_TX_0 = LED_DATA_TX_0 | temp;
        }
        check = check << 1;
        temp = temp << 2;
    }

    temp = 0x01;
    check = 0x0010;

    for(j = 0; j < 4; j++){
        if((LED_DATA & check) == check){
            LED_DATA_TX_1= LED_DATA_TX_1 | temp;
        }
        check = check << 1;
        temp = temp << 2;
    }

    temp = 0x01;
    check = 0x0100;

    for(j = 0; j < 4; j++){
        if((LED_DATA & check) == check){
            LED_DATA_TX_2 = LED_DATA_TX_2 | temp;
        }
        check = check << 1;
        temp = temp << 2;
    }

    temp = 0x01;
    check = 0x1000;

    for(j = 0; j < 4; j++){
        if((LED_DATA & check) == check){
            LED_DATA_TX_3 = LED_DATA_TX_3 | temp;
        }
        check = check << 1;
        temp = temp << 2;
    }

    /*
     * Set initial slave address
     *
     * 7-Bit: MSB is bit 6, Bits 9-7 are ignored
     */
    if(unit == 0){
        UCB2I2CSA = 0x60;   //Blue
    }
    else if(unit == 1){
        UCB2I2CSA = 0x61;   //Green
    }
    else{
        UCB2I2CSA = 0x62;   //Red
    }

    //Generate Start Condition
    //UCB2CTLW0 = UCB2CTLW0 | 0x0002;
    //while((UCB2CTLW0 & 0x0002) == 0x0002);

    //Wait for Buffer
    //while((UCB2IFG & 0x0002) == 0x2 );
    //Delay2(1000);
    //while((UCB2IFG & 0x0001) == 1 );
    //while((UCB2STATW & 0x0010) == 0x10);

    //Send register address
    //UCB2TXBUF = 0x16;   //Auto Increment :)
    //while((UCB2IFG & 0x0002) == 0x2 );
    //Delay2(1000);
    //while((UCB2IFG & 0x0001) == 1 );
    //while((UCB2STATW & 0x0010) == 0x10);

    //Fill TXBUF with data
    int i = 0;
    for(i = 0; i < 4; i++){
        //Send start every loop
        UCB2CTLW0 = UCB2CTLW0 | UCTXSTT;
        //UCB2CTLW0 = UCB2CTLW0 | 0x0002;
        while((UCB2CTLW0 & UCTXSTT)); //Wait for start condition to clear
        //Delay2(1000);
        if(i == 0){
            UCB2TXBUF = 0x06;
            while((UCB2IFG & UCTXIFG0) != UCTXIFG0 );   //Flag is set when buffer is empty
            UCB2TXBUF = LED_DATA_TX_0;
            while((UCB2IFG & UCTXIFG0) != UCTXIFG0 );
        }
        else if(i == 1){
            UCB2TXBUF = 0x07;
            while((UCB2IFG & UCTXIFG0) != UCTXIFG0 );
            UCB2TXBUF = LED_DATA_TX_1;
            while((UCB2IFG & UCTXIFG0) != UCTXIFG0 );
        }
        else if(i == 2){
            UCB2TXBUF = 0x08;
            while((UCB2IFG & UCTXIFG0) != UCTXIFG0 );
            UCB2TXBUF = LED_DATA_TX_2;
            while((UCB2IFG & UCTXIFG0) != UCTXIFG0 );
        }
        else{
            UCB2TXBUF = 0x09;
            while((UCB2IFG & UCTXIFG0) != UCTXIFG0 );
            UCB2TXBUF = LED_DATA_TX_3;
            while((UCB2IFG & UCTXIFG0) != UCTXIFG0 );
        }

        //while((UCB2IFG & 0x0002) == 0x2 );
        while((UCB2IFG & 0x0001) == 1 );
        //UCB2CTLW0 = UCB2CTLW0 & 0xFFFD;
        //while((UCB2IFG & 0x0001) == 1 );
        //while((UCB2STATW & 0x0010) == 0x10);
    }

    //Generate Stop Condition once
    UCB2CTLW0 = UCB2CTLW0 | UCTXSTP;
    while(UCB2CTLW0 & UCTXSTP);     //Wait for stop condition to clear

}

void init_RGBLEDS(){
    uint16_t UNIT_OFF = 0x0000;

    //Software reset enable
    UCB2CTLW0 = UCSWRST;

    /////////////////////
    ///Configure ports///
    ////////////////////

    /*
     * I2C master
     * -Master (bit 11 UCMST)
     * -I2C mode (Bits 10-9 UCMODEx)
     * -Clock Sync (Bit 8 UCSYNC)
     * -SMCLK Source (Bits 7-6 UCSSELx)
     * -Transmitter (Bit 4 UCTR)
     */
    UCB2CTLW0 |= 0b0000111110010000;

    /*
     * Fsmclk = 12 MHz
     * Want Fclk = 400Khz
     * Set Prescaler
     *
     * Fclk = Fsmclk / Prescaler
     */
    UCB2BRW = 30;

    /*
     *Want P3.6 = UCB2_SDA -> Want P3SEL1 = 0 and P3SEL0 = 1
     *Want P3.7 = UCB2_SCL -> Want P3SEL1 = 0 and P3SEL0 = 1
     */
    P3SEL0 |= 0b11000000;
    P3SEL1 &= 0b00111111;

    /*
     * Clear UCSWRST (Remove software reset)
     */
    UCB2CTLW0 &= ~UCSWRST;

    /*
     * Enable Interrupts (Optional)
     */
    //UCB2IE = 0x7FFF;

    LP3943_LedModeSet(0, UNIT_OFF);
    LP3943_LedModeSet(1, UNIT_OFF);
    LP3943_LedModeSet(2, UNIT_OFF);
}



