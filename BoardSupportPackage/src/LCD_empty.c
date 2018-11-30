/*
 * LCDLib.c
 *
 *  Created on: Mar 2, 2017
 *      Author: Danny
 */

#include "LCD_empty.h"
#include "msp.h"
#include "driverlib.h"
#include "AsciiLib.h"

/************************************  Structures  **************************************************/

static const eUSCI_SPI_MasterConfig SPI_LCD_Config = {
                                                      EUSCI_SPI_CLOCKSOURCE_SMCLK,  //UCAxCTLW0-> UCSSELx (clock source select)-> SMCLK
                                                      12000000, //Clock Source Frequency
                                                      12000000, //Desired SPI Clock
                                                      EUSCI_SPI_MSB_FIRST,  //msbFirst (UCMSB)
                                                      EUSCI_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT,    //clockPhase (UCCKP)
                                                      //EUSCI_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT,
                                                      EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_HIGH,   //clockPolarity (UCCKPL)
                                                      //EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_LOW,
                                                      EUSCI_SPI_3PIN    //SPImode (UCMODEx)
};

/************************************  Private Functions  *******************************************/

/*
 * Delay x ms
 */
static void Delay(unsigned long interval)
{
    while(interval > 0)
    {
        __delay_cycles(48000);
        interval--;
    }
}

/*******************************************************************************
 * Function Name  : LCD_initSPI
 * Description    : Configures LCD Control lines
 * Input          : None
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
static void LCD_initSPI()
{
    /* P10.1 - CLK
     * P10.2 - MOSI
     * P10.3 - MISO
     * P10.4 - LCD CS 
     * P10.5 - TP CS 
     */

//    //UCB3CLK For P10.1
//    P10SEL1.1 = 0;
//    P10SEL0.1 = 1;
//
//    //UCB3SIMO For P10.2
//    P10SEL1.2 = 0;
//    P10SEL0.2 = 1;
//
//    //UCB3SOMI For P10.3
//    P10SEL1.3 = 0;
//    P10SEL0.3 = 1;

    P10SEL0 |= 0x0E;
    P10SEL1 &= 0xC1;
    P10DIR |= 0x30;

    SPI_initMaster(EUSCI_B3_BASE, &SPI_LCD_Config);
    SPI_enableModule(EUSCI_B3_BASE);

//    //I/O For P10.4
//    P10SEL1.4 = 0;
//    P10SEL0.4 = 0;
//
//    //I/O For P10.5
//    P10SEL1.5 = 0;
//    P10SEL0.5 = 0;

}

/*******************************************************************************
 * Function Name  : LCD_reset
 * Description    : Resets LCD
 * Input          : None
 * Output         : None
 * Return         : None
 * Attention      : Uses P10.0 for reset
 *******************************************************************************/
static void LCD_reset()
{
    P10DIR |= BIT0;
    P10OUT |= BIT0;  // high
    Delay(100);
    P10OUT &= ~BIT0; // low
    Delay(100);
    P10OUT |= BIT0;  // high
}

/************************************  Private Functions  *******************************************/


/************************************  Public Functions  *******************************************/

/*******************************************************************************
 * Function Name  : LCD_DrawRectangle
 * Description    : Draw a rectangle as the specified color
 * Input          : xStart, xEnd, yStart, yEnd, Color
 * Output         : None
 * Return         : None
 * Attention      : Must draw from left to right, top to bottom!
 *******************************************************************************/
void LCD_DrawRectangle(int16_t xStart, int16_t xEnd, int16_t yStart, int16_t yEnd, uint16_t Color)
{
    //Uses continuous data transmission to remove cs toggling requirement
    //Uses auto incrementing of data transmission

    //Set the rectangle area size
    LCD_WriteReg(HOR_ADDR_START_POS, yStart);
    LCD_WriteReg(HOR_ADDR_END_POS, yEnd);
    LCD_WriteReg(VERT_ADDR_START_POS, xStart);
    LCD_WriteReg(VERT_ADDR_END_POS, xEnd);

    //Y is horizontal and X is vertical
//    LCD_WriteReg(GRAM_HORIZONTAL_ADDRESS_SET, yStart);
//    LCD_WriteReg(GRAM_VERTICAL_ADDRESS_SET, xStart);
    LCD_SetCursor(xStart,yStart);
    //LCD_WriteReg(DATA_IN_GRAM, Color);
    LCD_WriteIndex(DATA_IN_GRAM);
    SPI_CS_LOW;
    LCD_Write_Data_Start();

    int i = 0;
    int j = 0;
    for(i = xStart; i < xEnd+1; i++){
        for(j = yStart; j < yEnd+1; j++){
            LCD_Write_Data_Only(Color);
        }
    }
    SPI_CS_HIGH;
    //LCD_SetPoint(uint16_t Xpos, uint16_t Ypos, uint16_t color)

}

/******************************************************************************
 * Function Name  : PutChar
 * Description    : Lcd screen displays a character
 * Input          : - Xpos: Horizontal coordinate
 *                  - Ypos: Vertical coordinate
 *                  - ASCI: Displayed character
 *                  - charColor: Character color
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void PutChar( uint16_t Xpos, uint16_t Ypos, uint8_t ASCI, uint16_t charColor)
{
    uint16_t i, j;
    uint8_t buffer[16], tmp_char;
    GetASCIICode(buffer,ASCI);  /* get font data */
    for( i=0; i<16; i++ )
    {
        tmp_char = buffer[i];
        for( j=0; j<8; j++ )
        {
            if( (tmp_char >> 7 - j) & 0x01 == 0x01 )
            {
                LCD_SetPoint( Xpos + j, Ypos + i, charColor );  /* Character color */
            }
        }
    }
}

/******************************************************************************
 * Function Name  : GUI_Text
 * Description    : Displays the string
 * Input          : - Xpos: Horizontal coordinate
 *                  - Ypos: Vertical coordinate
 *                  - str: Displayed string
 *                  - charColor: Character color
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
void LCD_Text(uint16_t Xpos, uint16_t Ypos, uint8_t *str, uint16_t Color)
{
    uint8_t TempChar;

    /* Set area back to span the entire LCD */
    LCD_WriteReg(HOR_ADDR_START_POS, 0x0000);     /* Horizontal GRAM Start Address */
    LCD_WriteReg(HOR_ADDR_END_POS, (MAX_SCREEN_Y - 1));  /* Horizontal GRAM End Address */
    LCD_WriteReg(VERT_ADDR_START_POS, 0x0000);    /* Vertical GRAM Start Address */
    LCD_WriteReg(VERT_ADDR_END_POS, (MAX_SCREEN_X - 1)); /* Vertical GRAM Start Address */
//    LCD_WriteReg(HOR_ADDR_START_POS, 100);
//    LCD_WriteReg(HOR_ADDR_END_POS, 200);
//    LCD_WriteReg(VERT_ADDR_START_POS, 100);
//    LCD_WriteReg(VERT_ADDR_END_POS, 200);
    do
    {
        TempChar = *str++;
        PutChar( Xpos, Ypos, TempChar, Color);
        if( Xpos < MAX_SCREEN_X - 8)
        {
            Xpos += 8;
        }
        else if ( Ypos < MAX_SCREEN_X - 16)
        {
            Xpos = 0;
            Ypos += 16;
        }
        else
        {
            Xpos = 0;
            Ypos = 0;
        }
    }
    while ( *str != 0 );
}


/*******************************************************************************
 * Function Name  : LCD_Clear
 * Description    : Fill the screen as the specified color
 * Input          : - Color: Screen Color
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
void LCD_Clear(uint16_t Color)
{
    LCD_DrawRectangle(0x0000, MAX_SCREEN_X-1, 0x0000, MAX_SCREEN_Y-1, Color);

}

/******************************************************************************
 * Function Name  : LCD_SetPoint
 * Description    : Drawn at a specified point coordinates
 * Input          : - Xpos: Row Coordinate
 *                  - Ypos: Line Coordinate
 * Output         : None
 * Return         : None
 * Attention      : 18N Bytes Written
 *******************************************************************************/
void LCD_SetPoint(uint16_t Xpos, uint16_t Ypos, uint16_t color)
{
    LCD_WriteReg(GRAM_HORIZONTAL_ADDRESS_SET, Ypos);
    LCD_WriteReg(GRAM_VERTICAL_ADDRESS_SET, Xpos);
    LCD_WriteReg(DATA_IN_GRAM, color);
}

/*******************************************************************************
 * Function Name  : LCD_Write_Data_Only
 * Description    : Data writing to the LCD controller
 * Input          : - data: data to be written
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void LCD_Write_Data_Only(uint16_t data)
{
    SPISendRecvByte((data >>   8));                    /* Write D8..D15                */
    SPISendRecvByte((data & 0xFF));                    /* Write D0..D7                 */
}

/*******************************************************************************
 * Function Name  : LCD_WriteData
 * Description    : LCD write register data
 * Input          : - data: register data
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void LCD_WriteData(uint16_t data)
{
    SPI_CS_LOW;

    SPISendRecvByte(SPI_START | SPI_WR | SPI_DATA);    /* Write : RS = 1, RW = 0       */
    SPISendRecvByte((data >>   8));                    /* Write D8..D15                */
    SPISendRecvByte((data & 0xFF));                    /* Write D0..D7                 */

    SPI_CS_HIGH;
}

/*******************************************************************************
 * Function Name  : LCD_WriteReg
 * Description    : Reads the selected LCD Register.
 * Input          : None
 * Output         : None
 * Return         : LCD Register Value.
 * Attention      : None
 *******************************************************************************/
inline uint16_t LCD_ReadReg(uint16_t LCD_Reg)
{
    LCD_WriteIndex(LCD_Reg);
    uint16_t temp;
    temp = LCD_ReadData();
    return temp;
}

/*******************************************************************************
 * Function Name  : LCD_WriteIndex
 * Description    : LCD write register address
 * Input          : - index: register address
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void LCD_WriteIndex(uint16_t index)
{
    SPI_CS_LOW;

    /* SPI write data */
    SPISendRecvByte(SPI_START | SPI_WR | SPI_INDEX);   /* Write : RS = 0, RW = 0  */
    SPISendRecvByte(0);
    SPISendRecvByte(index);

    SPI_CS_HIGH;
}

/*******************************************************************************
 * Function Name  : SPISendRecvByte
 * Description    : Send one byte then receive one byte of response
 * Input          : uint8_t: byte
 * Output         : None
 * Return         : Recieved value 
 * Attention      : None
 *******************************************************************************/
inline uint8_t SPISendRecvByte (uint8_t byte)
{
    uint16_t temp;
    SPI_transmitData(EUSCI_B3_BASE, byte);
    temp = SPI_receiveData(EUSCI_B3_BASE);
    return temp;
}

/*******************************************************************************
 * Function Name  : LCD_Write_Data_Start
 * Description    : Start of data writing to the LCD controller
 * Input          : None
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void LCD_Write_Data_Start(void)
{
    SPISendRecvByte(SPI_START | SPI_WR | SPI_DATA);    /* Write : RS = 1, RW = 0 */
}

/*******************************************************************************
 * Function Name  : LCD_ReadData
 * Description    : LCD read data
 * Input          : None
 * Output         : None
 * Return         : return data
 * Attention      : Diagram (d) in datasheet
 *******************************************************************************/
inline uint16_t LCD_ReadData()
{
    uint16_t value;
    SPI_CS_LOW;

    SPISendRecvByte(SPI_START | SPI_RD | SPI_DATA);   /* Read: RS = 1, RW = 1   */
    SPISendRecvByte(0);                               /* Dummy read 1           */
    value = (SPISendRecvByte(0) << 8);                /* Read D8..D15           */
    value |= SPISendRecvByte(0);                      /* Read D0..D7            */

    SPI_CS_HIGH;
    return value;
}

/*******************************************************************************
 * Function Name  : LCD_WriteReg
 * Description    : Writes to the selected LCD register.
 * Input          : - LCD_Reg: address of the selected register.
 *                  - LCD_RegValue: value to write to the selected register.
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void LCD_WriteReg(uint16_t LCD_Reg, uint16_t LCD_RegValue)
{
    //SPI_CS_LOW;
    LCD_WriteIndex(LCD_Reg);
    LCD_WriteData(LCD_RegValue);
    //SPI_CS_HIGH;
}

/*******************************************************************************
 * Function Name  : LCD_SetCursor
 * Description    : Sets the cursor position.
 * Input          : - Xpos: specifies the X position.
 *                  - Ypos: specifies the Y position.
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos )
{
    LCD_WriteReg(GRAM_HORIZONTAL_ADDRESS_SET, Ypos);
    LCD_WriteReg(GRAM_VERTICAL_ADDRESS_SET, Xpos);
}

/*******************************************************************************
 * Function Name  : LCD_Init
 * Description    : Configures LCD Control lines, sets whole screen black
 * Input          : bool usingTP: determines whether or not to enable TP interrupt 
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
void LCD_Init(bool usingTP)
{
    LCD_initSPI();

    if (usingTP)
    {
        /* Configure low true interrupt on P4.0 for TP */ 
        P4 -> DIR &= ~BIT0;
        P4 -> IFG &= ~BIT0; //IFG Cleared on P4.0
        P4 -> IE |= BIT0;   //Enables interrupt on P4.0
        P4 -> IES |= BIT0;  //HIgh to low transition trigger
        P4 -> REN |= BIT0;  //Resistor enabled
        P4 -> OUT |= BIT0;  //Resistor set to pull up

        //Maybe not
        //NVIC_EnableIRQ(PORT4_IRQn); //Enable da interrupt
    }

    LCD_reset();

    //uint16_t value = 0; //Just for debugging purposes

    LCD_WriteReg(0xE5, 0x78F0); /* set SRAM internal timing */

    //value = LCD_ReadReg(0xE5);

    LCD_WriteReg(DRIVER_OUTPUT_CONTROL, 0x0100); /* set Driver Output Control */
    LCD_WriteReg(DRIVING_WAVE_CONTROL, 0x0700); /* set 1 line inversion */
    LCD_WriteReg(ENTRY_MODE, 0x1038); /* set GRAM write direction and BGR=1 */
    LCD_WriteReg(RESIZING_CONTROL, 0x0000); /* Resize register */
    LCD_WriteReg(DISPLAY_CONTROL_2, 0x0207); /* set the back porch and front porch */
    LCD_WriteReg(DISPLAY_CONTROL_3, 0x0000); /* set non-display area refresh cycle ISC[3:0] */
    LCD_WriteReg(DISPLAY_CONTROL_4, 0x0000); /* FMARK function */
    LCD_WriteReg(RGB_DISPLAY_INTERFACE_CONTROL_1, 0x0000); /* RGB interface setting */
    LCD_WriteReg(FRAME_MARKER_POSITION, 0x0000); /* Frame marker Position */
    LCD_WriteReg(RGB_DISPLAY_INTERFACE_CONTROL_2, 0x0000); /* RGB interface polarity */

    /* Power On sequence */
    LCD_WriteReg(POWER_CONTROL_1, 0x0000); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
    LCD_WriteReg(POWER_CONTROL_2, 0x0007); /* DC1[2:0], DC0[2:0], VC[2:0] */
    LCD_WriteReg(POWER_CONTROL_3, 0x0000); /* VREG1OUT voltage */
    LCD_WriteReg(POWER_CONTROL_4, 0x0000); /* VDV[4:0] for VCOM amplitude */
    LCD_WriteReg(DISPLAY_CONTROL_1, 0x0001);
    Delay(200);

    /* Dis-charge capacitor power voltage */
    LCD_WriteReg(POWER_CONTROL_1, 0x1090); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
    LCD_WriteReg(POWER_CONTROL_2, 0x0227); /* Set DC1[2:0], DC0[2:0], VC[2:0] */
    Delay(50); /* Delay 50ms */
    LCD_WriteReg(POWER_CONTROL_3, 0x001F);
    Delay(50); /* Delay 50ms */
    LCD_WriteReg(POWER_CONTROL_4, 0x1500); /* VDV[4:0] for VCOM amplitude */
    LCD_WriteReg(POWER_CONTROL_7, 0x0027); /* 04 VCM[5:0] for VCOMH */
    LCD_WriteReg(FRAME_RATE_AND_COLOR_CONTROL, 0x000D); /* Set Frame Rate */
    Delay(50); /* Delay 50ms */
    LCD_WriteReg(GRAM_HORIZONTAL_ADDRESS_SET, 0x0000); /* GRAM horizontal Address */
    LCD_WriteReg(GRAM_VERTICAL_ADDRESS_SET, 0x0000); /* GRAM Vertical Address */

    /* Adjust the Gamma Curve */
    LCD_WriteReg(GAMMA_CONTROL_1,    0x0000);
    LCD_WriteReg(GAMMA_CONTROL_2,    0x0707);
    LCD_WriteReg(GAMMA_CONTROL_3,    0x0307);
    LCD_WriteReg(GAMMA_CONTROL_4,    0x0200);
    LCD_WriteReg(GAMMA_CONTROL_5,    0x0008);
    LCD_WriteReg(GAMMA_CONTROL_6,    0x0004);
    LCD_WriteReg(GAMMA_CONTROL_7,    0x0000);
    LCD_WriteReg(GAMMA_CONTROL_8,    0x0707);
    LCD_WriteReg(GAMMA_CONTROL_9,    0x0002);
    LCD_WriteReg(GAMMA_CONTROL_10,   0x1D04);

    /* Set GRAM area */
    LCD_WriteReg(HOR_ADDR_START_POS, 0x0000);     /* Horizontal GRAM Start Address */
    LCD_WriteReg(HOR_ADDR_END_POS, (MAX_SCREEN_Y - 1));  /* Horizontal GRAM End Address */
    LCD_WriteReg(VERT_ADDR_START_POS, 0x0000);    /* Vertical GRAM Start Address */
    LCD_WriteReg(VERT_ADDR_END_POS, (MAX_SCREEN_X - 1)); /* Vertical GRAM Start Address */
    LCD_WriteReg(GATE_SCAN_CONTROL_0X60, 0x2700); /* Gate Scan Line */
    LCD_WriteReg(GATE_SCAN_CONTROL_0X61, 0x0001); /* NDL,VLE, REV */
    LCD_WriteReg(GATE_SCAN_CONTROL_0X6A, 0x0000); /* set scrolling line */

    /* Partial Display Control */
    LCD_WriteReg(PART_IMAGE_1_DISPLAY_POS, 0x0000);
    LCD_WriteReg(PART_IMG_1_START_END_ADDR_0x81, 0x0000);
    LCD_WriteReg(PART_IMG_1_START_END_ADDR_0x82, 0x0000);
    LCD_WriteReg(PART_IMAGE_2_DISPLAY_POS, 0x0000);
    LCD_WriteReg(PART_IMG_2_START_END_ADDR_0x84, 0x0000);
    LCD_WriteReg(PART_IMG_2_START_END_ADDR_0x85, 0x0000);

    /* Panel Control */
    LCD_WriteReg(PANEL_ITERFACE_CONTROL_1, 0x0010);
    LCD_WriteReg(PANEL_ITERFACE_CONTROL_2, 0x0600);
    LCD_WriteReg(DISPLAY_CONTROL_1, 0x0133); /* 262K color and display ON */
    Delay(50); /* delay 50 ms */

    LCD_Clear(LCD_BLACK);
}

/*******************************************************************************
 * Function Name  : TP_ReadXY
 * Description    : Obtain X and Y touch coordinates
 * Input          : None
 * Output         : None
 * Return         : Pointer to "Point" structure
 * Attention      : None
 *******************************************************************************/
Point TP_ReadXY()
{
    Point XY;   //Output data :)
    SPI_CS_TP_LOW;  //CS low for the touch panel
    uint8_t junk;   //Dont really care about what comes back (maybe)
    junk = SPISendRecvByte(CHX);    //Acquire

    //So Data is returned as a 16 bit package received in 2 batches
    //But the actual data is only 12 bits
    //The first package is bits 11:5
    //The second package is bits 4:0
    //So Position: [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    //             [XX 11 10  9  8  7 6 5]
    //                                   [4 3 2 1 0 X X X] (because zero filled)
    //We need to OR them together but that requires making them 16 bits
    //And to save them in the proper place, they must be shifted correctly
    //[15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    //[XX XX XX XX XX XX X X X1110 9 8 7 6 5] Shift left by 5
    //[XX XX XX XX XX XX x X 4 3 2 1 0 X X X] Shift right by 3
    uint8_t top_half_data;
    uint8_t bottom_half_data;
    top_half_data = SPISendRecvByte(0);
    bottom_half_data = SPISendRecvByte(0);
    SPI_CS_TP_HIGH; //CS high for touch panel
    uint16_t complete_data; //Our correctly formatted data
    complete_data = (((uint16_t) top_half_data) << 5) | (((uint16_t) bottom_half_data) >> 3);

    //Now we need to convert the data to fit our LCD (because our LCD isnt as big or accurate)
    float real_data;
    real_data = (float) complete_data;
    //So our data is 12 bits so it goes 4095:0
    //We want to convert it to between 1:0
    //Then multiply it by the max screen size for X. This gives the right coordinate
    real_data = real_data/4095; //Because untouched is 4095
    real_data = real_data * MAX_SCREEN_X;
//    real_data = (real_data - 35) * (1.1636363636363636363636363636364);
    real_data = (real_data-30) * 1.1428571428571428571428571428571;
//    if(real_data > 160){
//        real_data = real_data + 10;
//    }
//    else{
//        real_data = real_data-30;
//    }
    XY.x = (uint16_t) real_data;

    //NOW FOR YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY
    SPI_CS_TP_LOW;  //CS low for the touch panel
    junk = SPISendRecvByte(CHY);    //Acquire

    //So Data is returned as a 16 bit package received in 2 batches
    //But the actual data is only 12 bits
    //The first package is bits 11:5
    //The second package is bits 4:0
    //So Position: [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    //             [XX 11 10  9  8  7 6 5]
    //                                   [4 3 2 1 0 X X X] (because zero filled)
    //We need to OR them together but that requires making them 16 bits
    //And to save them in the proper place, they must be shifted correctly
    //[15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    //[XX XX XX XX XX XX X X X1110 9 8 7 6 5] Shift left by 5
    //[XX XX XX XX XX XX x X 4 3 2 1 0 X X X] Shift right by 3

    top_half_data = SPISendRecvByte(0);
    bottom_half_data = SPISendRecvByte(0);
    SPI_CS_TP_HIGH; //CS high for touch panel
    complete_data = (((uint16_t) top_half_data) << 5) | (((uint16_t) bottom_half_data) >> 3);
    //Now we need to convert the data to fit our LCD (because our LCD isnt as big or accurate)
    //So our data is 12 bits so it goes 4095:0
    //We want to convert it to between 1:0
    //Then multiply it by the max screen size for Y. This gives the right coordinate
    real_data = (float) complete_data;
    real_data = real_data/4095;
    real_data = real_data * MAX_SCREEN_Y;
    real_data = (real_data - 14) * (1.1009174311926605504587155963303);
//    if(real_data > 120){
//        real_data = real_data + 8;
//    }
//    else{
//        real_data = real_data - 10;
//    }
    XY.y = (uint16_t) real_data;

    return XY;
}

/************************************  Public Functions  *******************************************/

