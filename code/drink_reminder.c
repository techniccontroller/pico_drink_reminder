/*****************************************************************************
* | File      	:   drink_reminder.c
* | Author      :   techniccontroller
* | Function    :   drink reminder and pong game
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2022-12-30
* | Info        :
******************************************************************************/
#include "drink_reminder.h"
#include "LCD_1in3.h"
#include "timer_functions.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DEBOUNCE_TIME 10
#define PADDLE_WIDTH 100
#define BALL_RADIUS 10
#define GAME_DELAY 40         // in ms
#define BALL_DELAY_MAX   150  // in ms
#define BALL_DELAY_MIN    20  // in ms
#define BALL_DELAY_STEP   15  // in ms
#define RENDER_DELAY 20
#define BALL_SPEED 10
#define WIN_SCORE 25
#define DRINK_REMIND_DELAY 5000
#define GAMEOVER_DISPLAY_DELAY 5000
#define DRINK_DELAY 1800000

enum {UP=-10, DOWN=+10, STAY=0} typedef PlayerMovement;
enum {IDLE, PONG, PONG_OVER, DRINK_INIT, DRINK_PULS} typedef State;


struct{
	uint8_t ballx;
    uint8_t bally;
    int8_t balldx;
    int8_t balldy;
    uint8_t playerAy;
    uint8_t playerBy;
    uint32_t ballDelay;
    uint8_t score;
    PlayerMovement playerMovement;
} typedef PongState;



bool reserved_addr(uint8_t addr) {
return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

int random_value(int max){
    return (int)((1.0 + max) * rand() / ( RAND_MAX + 1.0 ) );
}

void normalize_ball_speed(PongState * pongState){
    int norm = sqrt((pongState->balldx*pongState->balldx) + (pongState->balldy*pongState->balldy));
    pongState->balldx *= BALL_SPEED;
    pongState->balldy *= BALL_SPEED;
    pongState->balldx /=norm;
    pongState->balldy /=norm;
}

void clip_dy(PongState *pongState) {
    
    if(abs(pongState->balldy) > (2 * abs(pongState->balldx))){
        int diff = abs(pongState->balldy) - 2 * abs(pongState->balldx);
        printf("Clip dy: (%d, %d), %d\n", pongState->balldx, pongState->balldy, diff);
        if(pongState->balldy < 0){
            pongState->balldy += diff;
        }
        else{
            pongState->balldy -= diff;
        }
    }
}

void init_pong_game(PongState * pongState){
    printf("Init pong ...\n");
    pongState->ballx = 120;
    pongState->bally = 120;
    pongState->balldx = (random_value(2) - 1)*5;
    if(pongState->balldx == 0) pongState->balldx = -5;
    pongState->balldy = (random_value(10) - 5);
    pongState->playerAy = 120;
    pongState->playerBy = 120;
    pongState->ballDelay = BALL_DELAY_MAX;
    pongState->playerMovement = STAY;
    pongState->score = 0;
}

void ctrlUp(PongState *pongState){
    static uint32_t _lastButtonClick = 0;
    if (timer_func_millis() > _lastButtonClick + DEBOUNCE_TIME) {
        pongState->playerMovement = UP;
        _lastButtonClick = timer_func_millis();
    }
}


void ctrlDown(PongState *pongState){
    static uint32_t _lastButtonClick = 0;
    if (timer_func_millis() > _lastButtonClick + DEBOUNCE_TIME) {
        pongState->playerMovement = DOWN;
        _lastButtonClick = timer_func_millis();
    }
}

void update_ball(PongState *pongState){

    static uint32_t _lastBallUpdate = 0;
    if ((timer_func_millis() - _lastBallUpdate) < pongState->ballDelay) {
        return;
    }
    _lastBallUpdate = timer_func_millis();

    bool hitBall = false;

    // collision detection for player A
    if (pongState->balldx < 0 && pongState->ballx < (30+BALL_RADIUS)) {
        if(pongState->bally > (pongState->playerAy-PADDLE_WIDTH/2) && pongState->bally < (pongState->playerAy+PADDLE_WIDTH/2)){
            hitBall = true;
            int diff = pongState->bally - pongState->playerAy;
            pongState->balldy += 5*diff/(PADDLE_WIDTH/2);
        }
    }

    // collision detection for player B
    if (pongState->balldx > 0 && pongState->ballx > (210-BALL_RADIUS)) {
        if(pongState->bally > (pongState->playerBy-PADDLE_WIDTH/2) && pongState->bally < (pongState->playerBy+PADDLE_WIDTH/2)){
            hitBall = true;
            int diff = pongState->bally - pongState->playerBy;
            pongState->balldy += 5*diff/(PADDLE_WIDTH/2);
        }
    }

    if (hitBall == true) {
        pongState->balldx *= -1;
        
        if (pongState->ballDelay > BALL_DELAY_MIN) {
            pongState->ballDelay -= BALL_DELAY_STEP;
        }

        pongState->score += 1;
    }

    clip_dy(pongState);
    normalize_ball_speed(pongState);
    pongState->ballx += pongState->balldx;
    pongState->bally += pongState->balldy;

    if(pongState->bally >= (240-BALL_RADIUS)){
        int overshoot = pongState->bally - (240-BALL_RADIUS);
        pongState->bally -= 2 * overshoot;
        pongState->balldy  *= -1;
    } else if (pongState->bally <= BALL_RADIUS) {
        int overshoot = pongState->bally - BALL_RADIUS;
        pongState->bally -= 2 * overshoot;
        pongState->balldy  *= -1;
    }
}

void update_game(PongState *pongState){
    static uint32_t _lastDrawUpdate = 0; 
    
    if ((timer_func_millis() - _lastDrawUpdate) < GAME_DELAY) {
        return;
    }
    _lastDrawUpdate = timer_func_millis();

    // move paddle of player A (conrolled by user)
    if (pongState->playerAy - PADDLE_WIDTH/2 + pongState->playerMovement > 0 && pongState->playerAy + PADDLE_WIDTH/2 + pongState->playerMovement < 240) {
        pongState->playerAy += pongState->playerMovement;
    }
    pongState->playerMovement = STAY;

    // move paddle of player B (controlled by bot)
    int8_t diff = pongState->bally - pongState->playerBy + pongState->balldy * 0.5;
    PlayerMovement botMovement = STAY;
    // no movement if ball moves away from paddle or no difference between ball and paddle
    if(abs(diff) <= 5 || pongState->balldx < 0){
        botMovement = STAY;
    }
    else if(diff > 0){
        botMovement = DOWN; 
    }
    else{
        botMovement = UP;
    }
    if (pongState->playerBy - PADDLE_WIDTH/2 + botMovement > 0 && pongState->playerBy + PADDLE_WIDTH/2 + botMovement < 240) {
        pongState->playerBy += botMovement;
    }

}

bool game_has_ended(PongState *pongState){
    if (pongState->balldx > 0 && pongState->ballx > 220) {
        printf("Game over - Player B lost!\n");
        return true;
    }
    if (pongState->balldx < 0 && pongState->ballx < 20) {
        printf("Game over - Player A lost!\n");
        return true;
    }
    if (pongState->score > WIN_SCORE){
        printf("Game over - Player A won by score!\n");
        return true;
    }
    return false;
}

void draw_pong_state(PongState *pongState){
    Paint_Clear(BLACK);
    char Str[5] = {0};
    sprintf(Str, "%d", pongState->score);
    Paint_DrawString_EN(105, 210, Str, &Font24, BRRED, BLACK);
    Paint_DrawRectangle(10, pongState->playerAy-PADDLE_WIDTH/2, 30, pongState->playerAy+PADDLE_WIDTH/2, WHITE, DOT_PIXEL_1X1,DRAW_FILL_FULL);
    Paint_DrawRectangle(210, pongState->playerBy-PADDLE_WIDTH/2, 230, pongState->playerBy+PADDLE_WIDTH/2, WHITE, DOT_PIXEL_1X1,DRAW_FILL_FULL);
    Paint_DrawCircle(pongState->ballx, pongState->bally, BALL_RADIUS-2, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

int drink_reminder(void)
{

    DEV_Delay_ms(100);
    printf("drink_reminder Demo\r\n");
    if(DEV_Module_Init()!=0){
        return -1;
    }
    DEV_SET_PWM(50);
    /* LCD Init */
    printf("1.3inch LCD demo...\r\n");
    LCD_1IN3_Init(HORIZONTAL);
    LCD_1IN3_Clear(WHITE);
    
    //LCD_SetBacklight(1023);
    UDOUBLE Imagesize = LCD_1IN3_HEIGHT*LCD_1IN3_WIDTH*2;
    UWORD *BlackImage;
    if((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        exit(0);
    }
    // /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
    Paint_NewImage((UBYTE *)BlackImage,LCD_1IN3.WIDTH,LCD_1IN3.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_Clear(WHITE);
    Paint_SetRotate(ROTATE_0);
    Paint_Clear(WHITE);
   
    DEV_SET_PWM(50);

    uint8_t keyA = 15; 
    uint8_t keyB = 17; 
    uint8_t keyX = 19; 
    uint8_t keyY = 21;

    uint8_t up = 2;
	uint8_t down = 18;
	uint8_t left = 16;
	uint8_t right = 20;
	uint8_t ctrl = 3;

    SET_Infrared_PIN(keyA);    
    SET_Infrared_PIN(keyB);
    SET_Infrared_PIN(keyX);
    SET_Infrared_PIN(keyY);
		 
	SET_Infrared_PIN(up);
    SET_Infrared_PIN(down);
    SET_Infrared_PIN(left);
    SET_Infrared_PIN(right);
    SET_Infrared_PIN(ctrl);

    uint32_t last_drink_time = 0;
    uint32_t start_reminder_time = 0;
    uint32_t last_pwm_update = 0;
    uint32_t last_game_render = 0;
    uint32_t start_pong_over_time = 0;
    
    uint32_t button_press_time = 0;
    uint8_t display_drink = 0;
    int value = 0;
    int dvalue = +5;

    bool glas2_image = false;

    State state = DRINK_INIT;

    DEV_SET_PWM(0);

    PongState pongstate;
    

    while(1){

        switch(state){
            case IDLE:
                // idle state
                if(DEV_Digital_Read(ctrl ) == 0){
                    printf("Enter game state ...\n");
                    state = PONG;
                    button_press_time = timer_func_millis();
                    DEV_SET_PWM(50);
                    init_pong_game(&pongstate);
                }

                if(timer_func_millis() - last_drink_time > DRINK_DELAY){
                    last_drink_time = timer_func_millis();
                    printf("Enter drink reminder state ...\n");
                    state = DRINK_INIT;
                }
                break;

            case PONG:
                // game state

                if(DEV_Digital_Read(keyA ) == 0){
                    // Extend time in game state
                    button_press_time = timer_func_millis();
                }

                if(DEV_Digital_Read(left ) == 0){
                    printf("Exit game state ...\n");
                    state = IDLE;
                    DEV_SET_PWM(0);
                }

                if(DEV_Digital_Read(up ) == 0){
                    ctrlUp(&pongstate);
                }

                if(DEV_Digital_Read(down ) == 0){
                    ctrlDown(&pongstate);
                }

                update_ball(&pongstate);
                update_game(&pongstate);
                draw_pong_state(&pongstate);

                if(timer_func_millis() - last_game_render > RENDER_DELAY){
                    LCD_1IN3_Display(BlackImage);
                    last_game_render = timer_func_millis();
                }

                if(game_has_ended(&pongstate)){
                    printf("Exit game state ...\n");
                    state = PONG_OVER;
                    start_pong_over_time = timer_func_millis();
                    Paint_Clear(BLACK);
                    if(pongstate.ballx > 120 || pongstate.score > WIN_SCORE){
                        Paint_DrawString_EN(40, 20, "Gewonnen!!!", &Font24, WHITE, BLACK);
                        Paint_DrawImage1(win, 50, 64, 140, 140);
                    } else {
                        Paint_DrawString_EN(40, 20, "Verloren!!!", &Font24, WHITE, BLACK);
                        Paint_DrawImage1(lose, 50, 88, 140, 93);
                    }
                    LCD_1IN3_Display(BlackImage);
                }
                break;

            case PONG_OVER:
                if(timer_func_millis() - start_pong_over_time > GAMEOVER_DISPLAY_DELAY){
                    printf("Enter IDLE after game...\n");
                    state = IDLE;
                    DEV_SET_PWM(0);
                }
                break;
            
            case DRINK_INIT:
                // drink reminder start state
                if(glas2_image){
                    Paint_DrawImage1(glas2,0,0,240,240);
                } else {
                    Paint_DrawImage1(glas,0,0,240,240);
                }
                glas2_image = !glas2_image;
                LCD_1IN3_Display(BlackImage);
                state = DRINK_PULS;
                start_reminder_time = timer_func_millis();
                value = 0;
                dvalue = +5;
                break;

            case DRINK_PULS:
                // drink reminder pulse state

                if(timer_func_millis() - last_pwm_update > 20){
                    last_pwm_update = timer_func_millis();
                    value += dvalue;
                    if(value >= 100){
                        dvalue = -5;
                        value = 100;
                    }
                    else if(value < 20){
                        dvalue = 5;
                    }
                    DEV_SET_PWM(value);
                }
                

                if(timer_func_millis() - start_reminder_time > DRINK_REMIND_DELAY){
                    printf("Exit drink reminder state ...\n");
                    state = IDLE;
                    DEV_SET_PWM(0);
                    Paint_Clear(BLACK);
                    LCD_1IN3_Display(BlackImage);
                }
                break;

        }

    }


    /* Module Exit */
    free(BlackImage);
    BlackImage = NULL;
    
    
    DEV_Module_Exit();
    return 0;
}
