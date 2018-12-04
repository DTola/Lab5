/*
 * Game.c
 *
 *  Created on: Nov 27, 2018
 *      Author: Daniel & Luis
 */
#include "msp.h"
#include <driverlib.h>
#include "G8RTOS.h"
#include "G8RTOS_Semaphores.h"
#include <BSP.h>
#include "RGBLeds.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "G8RTOS_CriticalSection.h"
#include "Game.h"
#include "G8RTOS_Scheduler.h"

/*
 * Looking from the UF board
 *
 * ------------------------------
 * |0,0                    320,0|
 * |                            |
 * |0,240                320,240|
 * ------------------------------
 *
 * So top of board is at y = 0 for us :/
 */

/*********************************************** Globals *********************************************************************/
//Recommended Volatile for global
//Removed volatile so warning about incompatibility goes away
GameState_t Current_GameState;

/*
 * Need arrays for previous players and previous ball positions
 *
 * Position 0: Host
 * Position 1: Client
 */
//Don't use volatile for this or get incompatible warnings
PrevPlayer_t Prev_Player_Positions[NUM_OF_PLAYERS_PLAYING];
PrevBall_t Prev_Ball_Positions[MAX_NUM_OF_BALLS];


/*********************************************** Semaphores *********************************************************************/
semaphore_t I2C_Semaphore;  //Sensor stuff
semaphore_t LCD_Semaphore;
semaphore_t GameState_Semaphore;
semaphore_t CC3100_WIFI_Semaphore;
semaphore_t Client_Player_Semaphore;

/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 *
 * Yeet
 */
void IdleThread(){
    while(1){

    }
}

/*
 * Thread to draw all the objects in the game
 *
 * Description/Requirement:
 * -Should hold arrays of previous players and ball positions
 * -Draw and/or update balls (use alive attribute to tell if draw new or update position)
 * -Update players
 * -Sleep for 20ms (reasonable refresh rate)
 */
void DrawObjects(){
    //Initialize balls at somewhere off the screen
    //This covers initial ball creation
    int i = 0;  //Because compiler is dumb sometimes
    for(i = 0; i < MAX_NUM_OF_BALLS; i++){
        Prev_Ball_Positions[i].CenterX = ARENA_MIN_X - 10; //More than 3 so edge doesn't show up
        Prev_Ball_Positions[i].CenterY = ARENA_MIN_Y - 10; //More than 3 so edge doesn't show up
    }

    //Place players at center of the sides in the beginning
    Prev_Player_Positions[0].Center = PADDLE_X_CENTER;
    Prev_Player_Positions[1].Center = PADDLE_X_CENTER;

    /*
     * *Recommended
     * Used to store the current game state
     *
     * Allows for using the game state while not holding the game state semaphore
     */
    GameState_t GameState_Copy;

    while(1){
        G8RTOS_WaitSemaphore(&GameState_Semaphore);
        GameState_Copy = Current_GameState;
        G8RTOS_SignalSemaphore(&GameState_Semaphore);

        //Updating ball positions
        for(i = 0; i < MAX_NUM_OF_BALLS; i++){
            //Make sure to only update currently (not previous) alive balls
            if(GameState_Copy.balls[i].alive){
/////////////////////////////////////////////////////////////// CHECK OR DELETE THIS////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                //Use geometry of shapes to detect collision
//                //Previous balls are used to erase (time t-l)
//                //GameState balls are used to create the new ball ("movement") (time t_
//
//                /*
//                 * Check if ball safe from hitting top or bottom
//                 * D2 = Dimension of ball from center to edge
//                 *
//                 * Check if edge hits future edge
//                 */
//                if( ((Prev_Ball_Positions[i].CenterY + BALL_SIZE_D2) < VERT_CENTER_MAX_BALL - BALL_SIZE_D2)
//                        && ((Prev_Ball_Positions[i].CenterY - BALL_SIZE_D2) > VERT_CENTER_MIN_BALL) ){
//
//                }

                UpdateBallOnScreen(&(Prev_Ball_Positions[i]), &(GameState_Copy.balls[i]), GameState_Copy.balls[i].color);

            }
        }

        //Host
        UpdatePlayerOnScreen(&(Prev_Player_Positions[0]), &(GameState_Copy.players[0]));
        //Client
        UpdatePlayerOnScreen(&(Prev_Player_Positions[1]), &(GameState_Copy.players[1]));

        //Recommended sleep time
        sleep(20);
    }
}

/*
 * Thread to update LEDs based on score
 *
 * Description: Responsible for updating the LED array with current scores
 *
 * *Assumes scores don't go above 8
 */
void MoveLEDs(){
    /*
     * *Recommended
     * Used to store the current game state
     *
     * Allows for using the game state while not holding the game state semaphore
     */
    GameState_t GameState_Copy;

    //Use to compare if scores change (saves time if scores didnt change)
    uint8_t Prev_LEDScores[2] = {0, 0};

    //Led goes: 0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF
    //Maximum score is thus 8
    //Since scores are separated only by shifting, can use shift for easy calculation
    uint16_t LED_DATA_SET[9] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

    while(1){
        G8RTOS_WaitSemaphore(&GameState_Semaphore);
        GameState_Copy = Current_GameState;
        G8RTOS_SignalSemaphore(&GameState_Semaphore);

        //Save time/resources if score doesnt change
        //0 is host
        if(GameState_Copy.overallScores[0] != Prev_LEDScores[0]){
            //This is the only thread with LEDs so no need for Semaphore
            LP3943_LedModeSet(RED, LED_DATA_SET[GameState_Copy.LEDScores[0]]);
            Prev_LEDScores[0] = GameState_Copy.LEDScores[0];
        }

        //1 is client
        if(GameState_Copy.overallScores[1] != Prev_LEDScores[1]){
            LP3943_LedModeSet(BLUE, LED_DATA_SET[GameState_Copy.LEDScores[1]]);
            Prev_LEDScores[1] = GameState_Copy.LEDScores[1];
        }

        sleep(175); //Testing for now
    }
}

/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 *
 * Description:
 * We know GPIO P4.4 and P4.5 are buttons
 */
playerType GetPlayerRole(){
    //We wait until the button is pressed
    //While loops suck but its not like anything else is running
    //gpio.h
    uint8_t ButtonHost = 1;
    uint8_t ButtonClient = 1;

    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN4);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN5);

    //BUttons have pull up so a press is 0
    //We wait for a press
    while((ButtonHost == 1) && (ButtonClient == 1)){
        ButtonHost = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN4);
        ButtonClient = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN5);
    }

    if(ButtonHost == 0){
        return Host;
    }
    else{
        return Client;
    }
}

/*
 * Draw players given center X center coordinate
 *
 * Used to initially draw the player at Center
 */
void DrawPlayer(GeneralPlayerInfo_t * player){
    if(player->position == TOP){
        LCD_DrawRectangle(player->currentCenter - PADDLE_LEN_D2,
                          player->currentCenter + PADDLE_LEN_D2,
                          TOP_PADDLE_EDGE - PADDLE_WID,
                          TOP_PADDLE_EDGE,
                          player->color);
    }
    else{
        LCD_DrawRectangle(player->currentCenter - PADDLE_LEN_D2,
                          player->currentCenter + PADDLE_LEN_D2,
                          BOTTOM_PADDLE_EDGE,
                          BOTTOM_PADDLE_EDGE + PADDLE_WID,
                          player->color);
    }
}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer){
    //Make sure to not waste time/resources if paddle didn't move
    if(prevPlayerIn->Center == outPlayer->currentCenter){
        return;
    }

    //Need to use LCD
    G8RTOS_WaitSemaphore(&LCD_Semaphore);
    int16_t Paddle_Difference = outPlayer->currentCenter - prevPlayerIn->Center;

    if(outPlayer->position == TOP){
        //If top then use top paddle dimensions
        if(Paddle_Difference > 0){
            //0  2  4  6       Prev Center at 2. Current at 4
            //[  |  ]
            //   [  |  ]
            LCD_DrawRectangle(prevPlayerIn->Center - PADDLE_LEN_D2,
                              outPlayer->currentCenter - PADDLE_LEN_D2,
                              TOP_PADDLE_EDGE - PADDLE_WID,
                              TOP_PADDLE_EDGE,
                              LCD_BLACK);
            LCD_DrawRectangle(prevPlayerIn->Center + PADDLE_LEN_D2,
                              outPlayer->currentCenter + PADDLE_LEN_D2,
                              TOP_PADDLE_EDGE - PADDLE_WID,
                              TOP_PADDLE_EDGE,
                              outPlayer->color);
        }
        else{
            //0  2  4  6       Prev Center at 4. Current at 2
            //   [  |  ]
            //[  |  ]
            LCD_DrawRectangle(outPlayer->currentCenter + PADDLE_LEN_D2,
                              prevPlayerIn->Center + PADDLE_LEN_D2,
                              TOP_PADDLE_EDGE - PADDLE_WID,
                              TOP_PADDLE_EDGE,
                              LCD_BLACK);
            LCD_DrawRectangle(outPlayer->currentCenter + PADDLE_LEN_D2,
                              prevPlayerIn->Center + PADDLE_LEN_D2,
                              TOP_PADDLE_EDGE - PADDLE_WID,
                              TOP_PADDLE_EDGE,
                              outPlayer->color);
        }
    }
    else{
        //If bottom then use bottom paddle dimensions
        if(Paddle_Difference > 0){
            //0  2  4  6       Prev Center at 2. Current at 4
            //[  |  ]
            //   [  |  ]
            LCD_DrawRectangle(prevPlayerIn->Center - PADDLE_LEN_D2,
                              outPlayer->currentCenter - PADDLE_LEN_D2,
                              BOTTOM_PADDLE_EDGE,
                              BOTTOM_PADDLE_EDGE + PADDLE_WID,
                              LCD_BLACK);
            LCD_DrawRectangle(prevPlayerIn->Center + PADDLE_LEN_D2,
                              outPlayer->currentCenter + PADDLE_LEN_D2,
                              BOTTOM_PADDLE_EDGE,
                              BOTTOM_PADDLE_EDGE + PADDLE_WID,
                              outPlayer->color);
        }
        else{
            //0  2  4  6       Prev Center at 4. Current at 2
            //   [  |  ]
            //[  |  ]

            //*Want to draw from smaller to large number
            LCD_DrawRectangle(outPlayer->currentCenter + PADDLE_LEN_D2,
                              prevPlayerIn->Center + PADDLE_LEN_D2,
                              BOTTOM_PADDLE_EDGE,
                              BOTTOM_PADDLE_EDGE + PADDLE_WID,
                              LCD_BLACK);
            LCD_DrawRectangle(outPlayer->currentCenter + PADDLE_LEN_D2,
                              prevPlayerIn->Center + PADDLE_LEN_D2,
                              BOTTOM_PADDLE_EDGE,
                              BOTTOM_PADDLE_EDGE + PADDLE_WID,
                              outPlayer->color);
        }
    }

    G8RTOS_SignalSemaphore(&LCD_Semaphore);

    //Make sure to update previous player position now that previous is gone
    prevPlayerIn->Center = outPlayer->currentCenter;
}

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor){
    //Check if ball moved just in case to not waste time/resources
    if(previousBall->CenterX == currentBall->currentCenterX &&
            previousBall->CenterY == currentBall->currentCenterY){
        return;
    }

    //Using the LCD, don't want trouble pardner
    G8RTOS_WaitSemaphore(&LCD_Semaphore);

    //Erase the previous
    LCD_DrawRectangle(previousBall->CenterX - BALL_SIZE_D2,
                      previousBall->CenterX + BALL_SIZE_D2,
                      previousBall->CenterY - BALL_SIZE_D2,
                      previousBall->CenterY + BALL_SIZE_D2,
                      LCD_BLACK);

    LCD_DrawRectangle(currentBall->currentCenterX - BALL_SIZE_D2,
                      currentBall->currentCenterX + BALL_SIZE_D2,
                      currentBall->currentCenterY - BALL_SIZE_D2,
                      currentBall->currentCenterY + BALL_SIZE_D2,
                      currentBall->color);

    G8RTOS_SignalSemaphore(&LCD_Semaphore);

    //Update previous ball now that previous is erased
    previousBall->CenterX = currentBall->currentCenterX;
    previousBall->CenterY = currentBall->currentCenterY;
}

//Updates LED array without creating a new thread
void MoveLEDs_noThread(){
    uint16_t LED_DATA_SET[9] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};
    LP3943_LedModeSet(RED, LED_DATA_SET[Current_GameState.overallScores[0]]);
    LP3943_LedModeSet(BLUE, LED_DATA_SET[Current_GameState.overallScores[1]]);
}

/*
 * Initializes and prints initial game state
 *
 * No description was given so here is one:
 * -Reinits the GameState values
 * -Recreates the board (edges, scores, paddles) (don't use draw obj because no previous state)
 * -Clears the LED scores
 */
void InitBoardState(){
    Current_GameState.LEDScores[0] = 0;
    Current_GameState.LEDScores[1] = 0;

    //Update the LEDs
    MoveLEDs_noThread();

    //Reset all the balls for use later
    int i = 0;  //Because compiler is dumb
    for(i = 0; i < MAX_NUM_OF_BALLS; i++){
        Current_GameState.balls[i].alive = false;
        Current_GameState.balls[i].color = LCD_WHITE;

        //Put them off the screen just in case
        Current_GameState.balls[i].currentCenterX = -10;
        Current_GameState.balls[i].currentCenterY = -10;
    }

    //Game is restarting so
    Current_GameState.gameDone = false;

    //No balls in the beginning
    Current_GameState.numberOfBalls = 0;


    //NO NOT CHANGE OVERALL SCORES! That stays alive between games
    //NO change player

    //Reset player positions
    Current_GameState.players[0].currentCenter = PADDLE_X_CENTER;
    Current_GameState.players[1].currentCenter = PADDLE_X_CENTER;

    //No need to change winner bool

    //Clear the board
    LCD_Clear(LCD_BLACK);

    //Draw the board
    //Draw the edges
    LCD_DrawRectangle(0, ARENA_MIN_X, 0, ARENA_MAX_Y, LCD_WHITE);
    LCD_DrawRectangle(ARENA_MAX_X, MAX_SCREEN_X, 0, ARENA_MAX_Y, LCD_WHITE);

    //Draw the scores
    //Need char array
    char Host_Score[2];
    char Client_Score[2];
    sprintf(Host_Score, "Host: %d", Current_GameState.overallScores[0]);
    sprintf(Client_Score, "Client: %d", Current_GameState.overallScores[1]);

    //For some reason char is uint8_t here
    LCD_Text(0, 10, (uint8_t *)Host_Score, LCD_RED);
    LCD_Text(0, 30, (uint8_t *)Client_Score, LCD_BLUE);

}

/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 *
 * Description:
 * -Only thread created before launching the OS
 * -Inits the players
 * -Establishes connection with client (Use LED on launchpad to indicate WIFI connection)
 * -Should be trying to receive a packet from the client
 * -Should ack the client once client has joined
 * -Add threads:
 *  -GenerateBall, DrawObject, ReadJoystickHost, SendDataToClient,
 *  -ReceiveDataFromClient, MoveLEDs (lower priority), Idle
 * -Kill Self
 */
void CreateGame(){
    //No need for game state copy because nothing else exists :(
    //First thing is to init the players
    Current_GameState.players[0].color = PLAYER_RED;
    Current_GameState.players[0].currentCenter = PADDLE_X_CENTER;
    Current_GameState.players[0].position = BOTTOM;

    Current_GameState.players[1].color = PLAYER_BLUE;
    Current_GameState.players[1].currentCenter = PADDLE_X_CENTER;
    Current_GameState.players[1].position = TOP;

    //Set player role as host
    initCC3100(Host);

    //Establish connection
    _i32 UDP_retVal = -4;

    //Get specific player info which is struct sent from client to host
    //In this case we are waiting for the ip address from the client
    while(UDP_retVal < 0){
        //this came out nasty
//        UDP_retVal = ReceiveData(&(Current_GameState.player.IP_address), 4);  //Get client info
        //Need to cast to unsigned char or u8 (or else it doesn't work) but somehow it works
        UDP_retVal = ReceiveData((_u8*)&(Current_GameState.player),
                                 sizeof(Current_GameState.player));
    }

    //At this point client can be ACK
    //Now we got the client ip addr so now we can test it
    Current_GameState.player.acknowledge = true;
    P2OUT |= RED_LED;   //Indicates ACK was made

    //This also tests the IP
    SendData((_u8*)&(Current_GameState.player),
             Current_GameState.player.IP_address,
             sizeof(Current_GameState.player));

    //Now we wait for the device to be ready
    while(UDP_retVal < 0 || !(Current_GameState.player.ready)){
        UDP_retVal = ReceiveData((_u8*)&(Current_GameState.player),
                                 sizeof(Current_GameState.player));
    }

    //Now we know the client is ready
    P2OUT |= BLUE_LED;

    //Tell the client it is has joined the game
    Current_GameState.player.joined = true;
    SendData((_u8*)&(Current_GameState.player),
             Current_GameState.player.IP_address,
             sizeof(Current_GameState.player));

    //We should send the general player state information to the client
    //client is responsible for translation
    //1 is client
    SendData((_u8*)&(Current_GameState.players[1]),
             Current_GameState.player.IP_address,
             sizeof(Current_GameState.players[1]));

    InitBoardState();

    G8RTOS_InitSemaphore(&I2C_Semaphore, 1);
    G8RTOS_InitSemaphore(&LCD_Semaphore, 1);
    G8RTOS_InitSemaphore(&GameState_Semaphore, 1);
    G8RTOS_InitSemaphore(&CC3100_WIFI_Semaphore, 1);
    G8RTOS_InitSemaphore(&Client_Player_Semaphore, 1);

    //Now we add all the threads
    G8RTOS_AddThread(&GenerateBall, 250, "GenBall");
    G8RTOS_AddThread(&DrawObjects, 250, "DrwObj");
    G8RTOS_AddThread(&ReadJoystickHost, 250, "ReadJoyStkHst");
    G8RTOS_AddThread(&SendDataToClient, 250, "SendData2Client");
    G8RTOS_AddThread(&ReceiveDataFromClient, 250, "RecDataFromClient");
    G8RTOS_AddThread(&MoveLEDs, 254, "MoveLEDs");   //Lower priority
    G8RTOS_AddThread(&IdleThread, 255, "Idle");

    //Now we commit soduku
    G8RTOS_KillSelf();
}

/*
 * Thread that sends game state to client
 *
 * Description:
 * -Fill packet for client
 * -Send packet
 * -Check if game is done
 *  -If done, add EndOFGameHost thread with highest priority
 * -SLeep for 5ms (Found xp to be good amount of time for synchronization)
 */
void SendDataToClient(){
    /*
     * *Recommended
     * Used to store the current game state
     *
     * Allows for using the game state while not holding the game state semaphore
     */
    GameState_t GameState_Copy;
    while(1){
        G8RTOS_WaitSemaphore(&GameState_Semaphore);
        GameState_Copy = Current_GameState;
        G8RTOS_SignalSemaphore(&GameState_Semaphore);

        //There isn't anything to fill out
        //Send packet
        G8RTOS_WaitSemaphore(&CC3100_WIFI_Semaphore);
        SendData((_u8*)&GameState_Copy, GameState_Copy.player.IP_address, sizeof(GameState_Copy));
        G8RTOS_SignalSemaphore(&CC3100_WIFI_Semaphore);

        //Check if game is done
        if(GameState_Copy.gameDone){
            //Add end of game host thread with highest priority
            G8RTOS_AddThread(&EndOfGameHost, 1, "EndOfGameHost");
        }

        //We sleep now (well not in real life unfortunately)
        sleep(5);   //Given
    }
}

/*
 * Thread that receives UDP packets from client
 *
 * Description:
 * -Continually receive data until a return value greater than zero is returned (valid data)
 *  -Remember to release and take semaphore again so can still send data
 *  -Sleep for 1 ms to avoid deadlock
 * -Update player's current center with displacement received from the client
 * -Sleep for 2ms (found experimentally/given to us)
 */
void ReceiveDataFromClient(){
    /*
     * *Recommended
     * Used to store the current game state
     *
     * Allows for using the game state while not holding the game state semaphore
     */
    GameState_t GameState_Copy;
    while(1){
        G8RTOS_WaitSemaphore(&GameState_Semaphore);
        GameState_Copy = Current_GameState;
        G8RTOS_SignalSemaphore(&GameState_Semaphore);
        _i32 UDP_retVal = -4;
        while(UDP_retVal < 0){
            G8RTOS_WaitSemaphore(&CC3100_WIFI_Semaphore);
            UDP_retVal = ReceiveData((_u8*)&(GameState_Copy.player), sizeof(GameState_Copy.player));
            G8RTOS_SignalSemaphore(&CC3100_WIFI_Semaphore);

            //Now we sleep because the coder can't
            sleep(1);   //Given
        }

        //Update player current center with displacement from the client
        //1 is client
        //So we already calculated the current center in the client
        //SO no need to update the current center anymore lol
        //GameState_Copy.players[1].currentCenter = GameState_Copy.player.displacement;
        G8RTOS_WaitSemaphore(&GameState_Semaphore);
        Current_GameState.player = GameState_Copy.player;
        G8RTOS_SignalSemaphore(&GameState_Semaphore);

        //Sleeeeeeeeeeeeeeeeeeeeeeeeeep
        sleep(2);   //Given
    }
}

/*
 * Generate Ball thread
 *
 * Description:
 * -Add another MoveBall thread if number of balls is less than max
 * -Sleeps proportional to the number of balls currently in play
 */
void GenerateBall(){
    while(1){
        if(Current_GameState.numberOfBalls < MAX_NUM_OF_BALLS){
            G8RTOS_AddThread(&MoveBall, 255, "MoveBall");

            //Just in case
            G8RTOS_WaitSemaphore(&GameState_Semaphore);
            Current_GameState.numberOfBalls++;
            G8RTOS_SignalSemaphore(&GameState_Semaphore);
        }

        //If there are 8 balls then it sleeps for one second (125 * 8 = 1000ms)
        sleep(Current_GameState.numberOfBalls * 125);
    }
}

/*
 * Thread to read host's joystick
 *
 * Description:
 * -Read joystick ADC using GetJoystickCoordinates
 * -Need to add bias to values (found experimentally) since offset of small displace and noise
 * -Change self.displace accordingly (exp how much to scale ADC value)
 * -Sleep 10ms
 * -Add displace to bottom player in list of players (host!)
 *  -players[0].position += self.displace
 */
void ReadJoystickHost(){
    int16_t x_coord;
    int16_t y_coord;

    /*
     * *Recommended
     * Used to store the current game state
     *
     * Allows for using the game state while not holding the game state semaphore
     */
    GameState_t GameState_Copy;
    while(1){
        G8RTOS_WaitSemaphore(&GameState_Semaphore);
        GameState_Copy = Current_GameState;
        G8RTOS_SignalSemaphore(&GameState_Semaphore);

        GetJoystickCoordinates(&x_coord, &y_coord);

        //Insert bias here

        //Calculate displacement for ADC
        //BEcause joystick is weird, -values actually go right (positive x)
        //I dont think we are supposed to edit the displacement because it is from client...

        //Now sleep
        sleep(10);  //Given

        //Add displacement to host

        //No adjust current center?
        //Nvm we do need it because this isnt the client one...
        if(GameState_Copy.players[0].currentCenter + x_coord/2000 > HORIZ_CENTER_MAX_PL){
            GameState_Copy.players[0].currentCenter = HORIZ_CENTER_MAX_PL;
        }
        else if(GameState_Copy.players[0].currentCenter + x_coord/2000 < HORIZ_CENTER_MIN_PL){
            GameState_Copy.players[0].currentCenter = HORIZ_CENTER_MIN_PL;
        }
        else{
            GameState_Copy.players[0].currentCenter += x_coord/2000;
        }

        G8RTOS_WaitSemaphore(&GameState_Semaphore);
        Current_GameState = GameState_Copy;
        G8RTOS_SignalSemaphore(&GameState_Semaphore);
    }
}

/*
 * Thread to move a single ball
 *
 * Description:
 * -Go through array of balls and find one that's not alive
 * -Once found: init random pos and x and y vel as well as color and alive attributes
 * -Check for collision given current center and velocity (use that weird formula)
 * -If collision: adjust velocity and color
 * -If ball passes boundary edge: adj score, end game if needed, and kill self
 * -Else: Move ball in current direction according to velocity
 * -Sleep for 35 ms
 */
void MoveBall(){
    /*
     * *Recommended
     * Used to store the current game state
     *
     * Allows for using the game state while not holding the game state semaphore
     */
    GameState_t GameState_Copy;
    G8RTOS_WaitSemaphore(&GameState_Semaphore);
    GameState_Copy = Current_GameState;
    G8RTOS_SignalSemaphore(&GameState_Semaphore);

    int i = 0;  //Because compiler is dumb
    int Ball_Location = -1;
    for(i = 0; i < MAX_NUM_OF_BALLS; i++){
        if(!(GameState_Copy.balls[i].alive)){
            Ball_Location = i;
            break;
        }
    }

    //No ball was alive
    if(Ball_Location == -1){
        G8RTOS_KillSelf();
    }

    //Time to init the ball
    //No need to set alive because we wouldn't be here if it wasn't
    //SO that was a lie lol
    GameState_Copy.balls[Ball_Location].alive = true;
    GameState_Copy.balls[Ball_Location].color = LCD_WHITE;
    //Use system time from 0 to 255 to give a random position
    GameState_Copy.balls[Ball_Location].currentCenterX = SystemTime & 0xFF;

    //Gotta make sure we don't go past the edge limitations
    if(GameState_Copy.balls[Ball_Location].currentCenterX < HORIZ_CENTER_MIN_BALL){
        GameState_Copy.balls[Ball_Location].currentCenterX = HORIZ_CENTER_MIN_BALL;
    }
    else if(GameState_Copy.balls[Ball_Location].currentCenterX > HORIZ_CENTER_MAX_BALL){
        GameState_Copy.balls[Ball_Location].currentCenterX = HORIZ_CENTER_MAX_BALL;
    }

    GameState_Copy.balls[Ball_Location].currentCenterY = SystemTime & 0x3F;

    //Same tbh
    if(GameState_Copy.balls[Ball_Location].currentCenterY < VERT_CENTER_MIN_BALL){
        GameState_Copy.balls[Ball_Location].currentCenterY = VERT_CENTER_MIN_BALL;
    }
    else if(GameState_Copy.balls[Ball_Location].currentCenterY > VERT_CENTER_MAX_BALL){
        GameState_Copy.balls[Ball_Location].currentCenterY = VERT_CENTER_MAX_BALL;
    }

    //Gotta make variables for the velocity
    //Basically make sure it isn't 7 lol
    int16_t Velocity_X = SystemTime & 0x3;
    if(Velocity_X > MAX_BALL_SPEED){
        Velocity_X--;
    }
    int16_t Velocity_Y = SystemTime & 0x3;
    if(Velocity_Y > MAX_BALL_SPEED){
        Velocity_Y--;
    }

    //Update the game state with the new ball properties
    Current_GameState.balls[Ball_Location].alive = GameState_Copy.balls[Ball_Location].alive;
    Current_GameState.balls[Ball_Location].color = GameState_Copy.balls[Ball_Location].color;
    Current_GameState.balls[Ball_Location].currentCenterX = GameState_Copy.balls[Ball_Location].currentCenterX;
    Current_GameState.balls[Ball_Location].currentCenterY = GameState_Copy.balls[Ball_Location].currentCenterY;

    //Minkowski variables
    int32_t M_w = WIDTH_TOP_OR_BOTTOM;
    int32_t M_h = HEIGHT_TOP_OR_BOTTOM;
    int32_t M_dx_Host;
    int32_t M_dx_Client;
    int32_t M_dy_Host;
    int32_t M_dy_Client;

    while(1){
        //Get dat copy boi
        G8RTOS_WaitSemaphore(&GameState_Semaphore);
        GameState_Copy = Current_GameState;
        G8RTOS_SignalSemaphore(&GameState_Semaphore);

        //Used to tell if collision occurred. If so then no need to check if pass
        bool Collision_Event = false;

        //Lets do paddle collision first
        //Host is on bottom so the paddle will have y of 240ish
        //Client is on top so paddle will have y of 0ish
        M_dx_Host = GameState_Copy.balls[Ball_Location].currentCenterX - GameState_Copy.players[0].currentCenter;
        M_dx_Client = GameState_Copy.balls[Ball_Location].currentCenterX - GameState_Copy.players[1].currentCenter;

        //Might have to add wiggle room
        M_dy_Host = GameState_Copy.balls[Ball_Location].currentCenterY - BOTTOM_PLAYER_CENTER_Y;
        M_dy_Client = GameState_Copy.balls[Ball_Location].currentCenterX - TOP_PLAYER_CENTER_Y;

        //Lets do host collision first
        if(abs(M_dx_Host) <= M_w && abs(M_dy_Host) <= M_h){
            Collision_Event = true;
            //Collision!!!!!!!!!!!!!
            int32_t M_wy = M_w * M_dy_Host;
            int32_t M_hx = M_h * M_dx_Host;

            if(M_wy > M_hx){
                if(M_wy > (M_hx * -1)){
                    //Collision at the top
                    //Basically it hit the middle of the paddle so it should go up (-y)
                    Velocity_X = 0;
                    Velocity_Y = Velocity_Y * -1;
                }
                else{
                    //collision on the left
                    //Must go left
                    //Take abs just in case it was already going left
                    Velocity_X = abs(Velocity_X) * -1;
                    Velocity_Y = Velocity_Y * -1;
                }
            }
            else{
                if(M_wy > (M_hx * -1)){
                    //Collision on the right
                    //Must go right
                    Velocity_X = abs(Velocity_X);
                    Velocity_Y = Velocity_Y * -1;
                }
                else{
                    //No way to hit the bottom sooooooooooooo
                    //Collision on the bottom
                    Velocity_X = 0;
                    Velocity_Y = abs(Velocity_Y);
                }
            }

            //Update ball color
            GameState_Copy.balls[Ball_Location].color = PLAYER_RED;
        }
        else if(abs(M_dx_Client) <= M_w && abs(M_dy_Client) <= M_h){
            Collision_Event = true;
            //Collision!!!!!!!!!!!!!
            int32_t M_wy = M_w * M_dy_Client;
            int32_t M_hx = M_h * M_dx_Client;

            if(M_wy > M_hx){
                if(M_wy > (M_hx * -1)){
                    //Shouldnt be possible for client
                    //Collision at the top
                    //Basically it hit the middle of the paddle so it should go up (-y)
                    Velocity_X = 0;
                    Velocity_Y = abs(Velocity_Y);
                }
                else{
                    //collision on the left
                    //Must go left
                    //Take abs just in case it was already going left
                    Velocity_X = abs(Velocity_X) * -1;
                    Velocity_Y = abs(Velocity_Y);
                }
            }
            else{
                if(M_wy > (M_hx * -1)){
                    //Collision on the right
                    //This means it must go right
                    Velocity_X = abs(Velocity_X);
                    Velocity_Y = abs(Velocity_Y) ;
                }
                else{
                    //Collision on the bottom
                    Velocity_X = 0;
                    Velocity_Y = abs(Velocity_Y);
                }
                //No way to hit the bottom sooooooooooooo
            }

            //Update ball color
            GameState_Copy.balls[Ball_Location].color = PLAYER_BLUE;
        }

        //Maybe update ball location here with one movement of velocity?

        //Lets check if wall was collided with
        if(GameState_Copy.balls[Ball_Location].currentCenterX >= HORIZ_CENTER_MAX_BALL){
            Collision_Event = true;
            //Hit right side so now go left
            GameState_Copy.balls[Ball_Location].currentCenterX = HORIZ_CENTER_MAX_BALL - BALL_SIZE_D2;  //To avoid re-collision
            Velocity_X = Velocity_X * -1;
        }
        else if(GameState_Copy.balls[Ball_Location].currentCenterX <= HORIZ_CENTER_MIN_BALL){
            Collision_Event = true;
            //Hit left side so now go right
            GameState_Copy.balls[Ball_Location].currentCenterX = HORIZ_CENTER_MAX_BALL + BALL_SIZE_D2;  //To avoid re-collision
            Velocity_X = abs(Velocity_X);
        }

//        GameState_Copy.balls[Ball_Location].currentCenterX += Velocity_X;
//        GameState_Copy.balls[Ball_Location].currentCenterY += Velocity_Y;

        //Now lets check if ball passed the boundary edge
        //Host first of course
        //Used to notify if ball thread should be killed
        bool Kill_Ball = false;

        if(!Collision_Event){   //Don't wanna check if collision occurred
            if(GameState_Copy.balls[Ball_Location].currentCenterY >= VERT_CENTER_MAX_BALL){
                //Need to kill ball thread
                Kill_Ball = true;
                //Set ded
                GameState_Copy.balls[Ball_Location].alive = false;
                //Adjust scores. In this case the client gets one point
                GameState_Copy.LEDScores[1]++;
                GameState_Copy.numberOfBalls--;
            }
            else if(GameState_Copy.balls[Ball_Location].currentCenterY <= VERT_CENTER_MIN_BALL){  //Now we check cif pass client
                //Need to kill ball thread
                Kill_Ball = true;
                //Set ded
                GameState_Copy.balls[Ball_Location].alive = false;
                //Adjust scores. In this case the client gets one point
                GameState_Copy.LEDScores[0]++;
                GameState_Copy.numberOfBalls--;
            }
        }

        //Now we need to see if the game should end
        //Check host
        if(GameState_Copy.LEDScores[0] >= 8){
            GameState_Copy.overallScores[0]++;
            GameState_Copy.gameDone = true;
            GameState_Copy.winner = false;  //Means host won???
        }
        else if(GameState_Copy.LEDScores[1] >= 8){
            GameState_Copy.overallScores[1]++;
            GameState_Copy.gameDone = true;
            GameState_Copy.winner = true;
        }

        if(Kill_Ball){  //Need to kill self
            //Memory copy the copy into the current
            G8RTOS_WaitSemaphore(&GameState_Semaphore);
            Current_GameState = GameState_Copy;
            G8RTOS_SignalSemaphore(&GameState_Semaphore);

            G8RTOS_KillSelf();
        }
        else{   //Else just move the ball
            GameState_Copy.balls[Ball_Location].currentCenterX += Velocity_X;
            GameState_Copy.balls[Ball_Location].currentCenterY += Velocity_Y;

            //Memory copy the copy into the current
            G8RTOS_WaitSemaphore(&GameState_Semaphore);
            Current_GameState = GameState_Copy;
            G8RTOS_SignalSemaphore(&GameState_Semaphore);
        }

        sleep(35);  //Recommended
    }
}

/*
 * End of game for the host
 *
 * Description:
 * -Wait for all semaphores to be released
 * -Kill all other threads (need to make a function for this :( )
 * -Re-init all semaphores
 * -Clear screen with winner's color
 * -Print some message that waits for the host's action to start new game
 * -Create aperiodic thread that waits for host button press
 *  -Client will be waiting on host to start a new game
 * -Once ready, send notification to client, reinit game and obj, add all threads, kill self
 */
void EndOfGameHost(){
    //This looks like a lot of work

    //Wait for semaphores
    G8RTOS_WaitSemaphore(&I2C_Semaphore);
    G8RTOS_WaitSemaphore(&LCD_Semaphore);
    G8RTOS_WaitSemaphore(&GameState_Semaphore);
    G8RTOS_WaitSemaphore(&CC3100_WIFI_Semaphore);
    G8RTOS_WaitSemaphore(&Client_Player_Semaphore);

    //Kill all other threads
    //I was tempted to call this annihilation
    G8RTOS_KillAllOtherThreads();

    //Re-init all semaphores
    G8RTOS_InitSemaphore(&I2C_Semaphore, 1);
    G8RTOS_InitSemaphore(&LCD_Semaphore, 1);
    G8RTOS_InitSemaphore(&GameState_Semaphore, 1);
    G8RTOS_InitSemaphore(&CC3100_WIFI_Semaphore, 1);
    G8RTOS_InitSemaphore(&Client_Player_Semaphore, 1);

    //Clear screen with winner color
    if(Current_GameState.winner){
        //Client won
        LCD_Clear(LCD_RED);
    }
    else{
        //Host won
        LCD_Clear(LCD_BLUE);
    }

    //Print message that waits for host action to start new game
    //No need to wait for a semaphore here obviously
    LCD_Text(0, 120, "WAITING FOR BUTTON PRESS TO START NEW GAME", LCD_WHITE);

    //Create Aperiodic Thread that waits for host action press
    //Just use the existing getplayerrole function instead and use a lazy while loop
    GetPlayerRole();

    //Send client some data since it is waiting
    //By some data i meant the new gamestate
    //But first we need to init the board state or else gamestate is wrong for client
    InitBoardState();

    //Now we can send the real thing
    //This is the notification to client
    SendData((_u8*)&(Current_GameState), Current_GameState.player.IP_address, sizeof(Current_GameState));

    //Add all threads
    //Now we add all the threads
    G8RTOS_AddThread(&GenerateBall, 250, "GenBall");
    G8RTOS_AddThread(&DrawObjects, 250, "DrwObj");
    G8RTOS_AddThread(&ReadJoystickHost, 250, "ReadJoyStkHst");
    G8RTOS_AddThread(&SendDataToClient, 250, "SendData2Client");
    G8RTOS_AddThread(&ReceiveDataFromClient, 250, "RecDataFromClient");
    G8RTOS_AddThread(&MoveLEDs, 254, "MoveLEDs");   //Lower priority
    G8RTOS_AddThread(&IdleThread, 255, "Idle");

    //Commit sudoku
    G8RTOS_KillSelf();
}

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 *
 * Description:
 * -Only thread to run after launching OS
 * -Set initial SpecificPlayerInfo_t struct attributes
 *  -Use getLocalP() for ip addr
 * -Send player info to host
 * -Wait for server response
 * -If join game, ack join using LED
 * -Init board state, semaphores, add threads:
 *  -ReadJoystickClient, SendDataTOHost, ReceiveDataFromHost
 *  -DrawObjects, MoveLEDs, IDLE
 * -Kill Self
 */
void JoinGame(){
    //No need to init the players so go straight to setting the role
    //Set player role as host
    initCC3100(Client);

    //Need to set initial specific player attributes tho!
    Current_GameState.player.IP_address = getLocalIP();
    //Every other attribute isn't needed but set false just in case
    Current_GameState.player.acknowledge = false;
    Current_GameState.player.joined = false;
    Current_GameState.player.ready = false;

    _i32 UDP_retVal = -4;

    //Lets send the IP addr to the host
    SendData( (_u8*)&(Current_GameState.player),
              HOST_IP_ADDR,
              sizeof(Current_GameState.player));

    //Now let's wait for ack
    while(UDP_retVal < 0 || !(Current_GameState.player.acknowledge)){
        UDP_retVal = ReceiveData( (_u8*)&(Current_GameState.player),
                                  sizeof(Current_GameState.player));
    }

    //Now lets show we got it!
    P2OUT |= RED_LED;

    //Now we gotta tell the device we are ready to join
    Current_GameState.player.ready = true;
    SendData( (_u8*)&(Current_GameState.player),
              HOST_IP_ADDR,
              sizeof(Current_GameState.player));

    //Now lets wait for a response telling us we have joined
    while(UDP_retVal < 0 || !(Current_GameState.player.joined)){
        UDP_retVal = ReceiveData( (_u8*)&(Current_GameState.player),
                                  sizeof(Current_GameState.player));
    }

    //Lets show that we know we joined!
    P2OUT |= BLUE_LED;

    //Now lets get our general info
    while(UDP_retVal < 0){
        UDP_retVal = ReceiveData( (_u8*)&(Current_GameState.players[1]),
                                  sizeof(Current_GameState.players[1]));
    }

    //Now init the board
    InitBoardState();

    //Now init all the semaphores we need
    G8RTOS_InitSemaphore(&I2C_Semaphore, 1);
    G8RTOS_InitSemaphore(&LCD_Semaphore, 1);
    G8RTOS_InitSemaphore(&GameState_Semaphore, 1);
    G8RTOS_InitSemaphore(&CC3100_WIFI_Semaphore, 1);
    G8RTOS_InitSemaphore(&Client_Player_Semaphore, 1);

    //Now add all the threads
    G8RTOS_AddThread(&ReadJoystickClient, 250, "ReadJoyStkClient");
    G8RTOS_AddThread(&SendDataToHost, 250, "SendData2Host");
    G8RTOS_AddThread(&ReceiveDataFromHost, 250, "ReceiveDataFromHost");
    G8RTOS_AddThread(&DrawObjects, 250, "DrawObjects");
    G8RTOS_AddThread(&MoveLEDs, 254, "MoveLEDs");   //Lower priority
    G8RTOS_AddThread(&IdleThread, 255, "Idle");

    //Sudoku
    G8RTOS_KillSelf();
}

/*
 * Thread that receives game state packets from host
 *
 * Description:
 * -Continually receive data until return value greater than 0 is returned
 *  -Remember to release and take semaphore again so can send data
 *  -Sleep for 1 ms to avoid deadlock
 * -Empty received packet
 * -If game done, add endofgameclient thread with highest priority
 * -Sleep 5 ms
 */
void ReceiveDataFromHost(){
    /*
     * *Recommended
     * Used to store the current game state
     *
     * Allows for using the game state while not holding the game state semaphore
     */
    GameState_t GameState_Copy;
    while(1){
        G8RTOS_WaitSemaphore(&GameState_Semaphore);
        GameState_Copy = Current_GameState;
        G8RTOS_SignalSemaphore(&GameState_Semaphore);

        _i32 UDP_retVal = -4;
        while(UDP_retVal < 0){
            G8RTOS_WaitSemaphore(&CC3100_WIFI_Semaphore);
            UDP_retVal = ReceiveData((_u8*)&(GameState_Copy), sizeof(GameState_Copy));
            G8RTOS_SignalSemaphore(&CC3100_WIFI_Semaphore);

            //Now we sleep because the coder can't
            sleep(1);   //Given
        }

        //Update player current center with displacement from the client
        //1 is client

        //Empty the packet (copy it boi)
        G8RTOS_WaitSemaphore(&GameState_Semaphore);
        Current_GameState = GameState_Copy;
        G8RTOS_SignalSemaphore(&GameState_Semaphore);

        if(GameState_Copy.gameDone){
            G8RTOS_AddThread(EndOfGameClient, 1, "EndOfGameClient");
        }

        //Sleeeeeeeeeeeeeeeeeeeeeeeeeep
        sleep(5);   //Given
    }
}

/*
 * Thread that sends UDP packets to host
 *
 * Description:
 * -Send player info
 * -Sleep 2 ms
 */
void SendDataToHost(){
    /*
     * *Recommended
     * Used to store the current game state
     *
     * Allows for using the game state while not holding the game state semaphore
     */
    GameState_t GameState_Copy;
    while(1){
        G8RTOS_WaitSemaphore(&GameState_Semaphore);
        GameState_Copy = Current_GameState;
        G8RTOS_SignalSemaphore(&GameState_Semaphore);

        //There isn't anything to fill out
        //Send packet
        G8RTOS_WaitSemaphore(&CC3100_WIFI_Semaphore);
        SendData((_u8*)&GameState_Copy, HOST_IP_ADDR, sizeof(GameState_Copy));
        G8RTOS_SignalSemaphore(&CC3100_WIFI_Semaphore);

        //No need for client
//        //Check if game is done
//        if(GameState_Copy.gameDone){
//            //Add end of game host thread with highest priority
//            G8RTOS_AddThread(&EndOfGameHost, 1, "EndOfGameHost");
//        }

        //We sleep now (well not in real life unfortunately)
        sleep(2);   //Given
    }
}

/*
 * Thread to read client's joystick
 *
 * Description:
 * -Read Joytick and add offset
 * -Add displacement to self accordingly
 * -Sleep 10ms
 */
void ReadJoystickClient(){
    //Dare Dare Yaze
    int16_t x_coord;
        int16_t y_coord;

        /*
         * *Recommended
         * Used to store the current game state
         *
         * Allows for using the game state while not holding the game state semaphore
         */
        GameState_t GameState_Copy;
        while(1){
            G8RTOS_WaitSemaphore(&GameState_Semaphore);
            GameState_Copy = Current_GameState;
            G8RTOS_SignalSemaphore(&GameState_Semaphore);

            GetJoystickCoordinates(&x_coord, &y_coord);

            //Insert bias here if there was one

            //Maybe we do di
            //Calculate displacement for ADC
            //BEcause joystick is weird, -values actually go right (positive x)
            GameState_Copy.player.displacement = x_coord/2000;

            //No adjust current center?
            //Actually i think we need this anyways
            //Nvm its too late now lol
            if(GameState_Copy.players[1].currentCenter + GameState_Copy.player.displacement > HORIZ_CENTER_MAX_PL){
                GameState_Copy.players[1].currentCenter = HORIZ_CENTER_MAX_PL;
            }
            else if(GameState_Copy.players[1].currentCenter + GameState_Copy.player.displacement < HORIZ_CENTER_MIN_PL){
                GameState_Copy.players[1].currentCenter = HORIZ_CENTER_MIN_PL;
            }
            else{
                GameState_Copy.players[1].currentCenter += GameState_Copy.player.displacement;
            }


            G8RTOS_WaitSemaphore(&GameState_Semaphore);
            Current_GameState = GameState_Copy;
            G8RTOS_SignalSemaphore(&GameState_Semaphore);

            sleep(10);  //Given;
        }

        //It doesn't tell us to add the displacement to the bottom player so..
}

/*
 * End of game for the client
 *
 * Description:
 * -Wait for all semaphores to be released
 * -Kill all other threads
 * -Re-init semaphores
 * -Clear screen with winner's color
 * -Wait for host to restart game
 * -Add all threads back and restart game variables
 * -Kill self
 */
void EndOfGameClient(){
    //Wait for semaphores
    G8RTOS_WaitSemaphore(&I2C_Semaphore);
    G8RTOS_WaitSemaphore(&LCD_Semaphore);
    G8RTOS_WaitSemaphore(&GameState_Semaphore);
    G8RTOS_WaitSemaphore(&CC3100_WIFI_Semaphore);
    G8RTOS_WaitSemaphore(&Client_Player_Semaphore);

    //Kill all other threads
    G8RTOS_KillAllOtherThreads();

    //Re-init semaphores
    G8RTOS_InitSemaphore(&I2C_Semaphore, 1);
    G8RTOS_InitSemaphore(&LCD_Semaphore, 1);
    G8RTOS_InitSemaphore(&GameState_Semaphore, 1);
    G8RTOS_InitSemaphore(&CC3100_WIFI_Semaphore, 1);
    G8RTOS_InitSemaphore(&Client_Player_Semaphore, 1);


    //Clear screen with winner's color
    if(Current_GameState.winner){
        //Client won
        LCD_Clear(LCD_RED);
    }
    else{
        //Host won
        LCD_Clear(LCD_BLUE);
    }

    //Wait for host to restart game
    //We know it occurs because we get the real game state from host after inits it
    _i32 UDP_retVal = -4;
    LCD_Text(0, 120, "WAITING FOR HOST BUTTON PRESS TO START NEW GAME", LCD_WHITE);
    while(UDP_retVal < 0){
        //No need for semaphores
        UDP_retVal = ReceiveData((_u8*)&(Current_GameState), sizeof(Current_GameState));
    }

    InitBoardState();   //We gotta clear the board still



    //Adds all the CLIENT threads back
    G8RTOS_AddThread(&ReadJoystickClient, 250, "ReadJoyStkClient");
    G8RTOS_AddThread(&SendDataToHost, 250, "SendData2Host");
    G8RTOS_AddThread(&ReceiveDataFromHost, 250, "ReceiveDataFromHost");
    G8RTOS_AddThread(&DrawObjects, 250, "DrawObjects");
    G8RTOS_AddThread(&MoveLEDs, 254, "MoveLEDs");   //Lower priority
    G8RTOS_AddThread(&IdleThread, 255, "Idle");

    //Kill self :(
    G8RTOS_KillSelf();
}

