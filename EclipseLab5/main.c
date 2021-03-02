#include "msp.h"
#define TICKS ((uint16_t)0xFFFF)
#define TICKS2 ((uint16_t)0x6DDD)

/**
 * main.c
 */

float TickLength = 1.333;   //uS
float SpeedOfSound = 0.034; //cm per uS
int c = 4;

volatile int covDebug = 0;

volatile uint32_t CaptureValues [2] = {0};
volatile uint32_t ElapsedTicks = 0;
volatile float ElapsedTime = 0;
volatile float Distance = 0;
int pulselen = 10;

void timerA_stop(void){
    TIMER_A0->CTL &= TIMER_A_CTL_MC__STOP;
}
void timerA_config(void){

    //Configuring clock
    TIMER_A0->CTL       |= TIMER_A_CTL_CLR; // clears TimerA0
    TIMER_A0->CTL       |= TIMER_A_CTL_SSEL__SMCLK; //Use SMCLK
    TIMER_A0->CTL       |= TIMER_A_CTL_ID_2; // Sets timer ID to 2 ---- division by 4


    //configuring interrupts
    TIMER_A0->CCTL[0]   |= TIMER_A_CCTLN_CCIE;  //Enable interrupts
    TIMER_A0->CCTL[1]   |= TIMER_A_CCTLN_CCIE;
    TIMER_A0->CCTL[2]   |= TIMER_A_CCTLN_CCIE;  //Enable interrupt for  Echo on sensor - P2.5

    TIMER_A0->CCTL[2]   |= TIMER_A_CCTLN_CM__FALLING;   //set as capture input on falling edge
    TIMER_A0->CCTL[2]   |= TIMER_A_CCTLN_CAP;   //set as capture mode

    TIMER_A0->CCTL[2]   &= ~TIMER_A_CCTLN_CCIFG; //clear interrupt flag
    TIMER_A0->CCR[1]    = pulselen;
}

void timerA_start(void){
    TIMER_A0->CTL |= TIMER_A_CTL_MC__UP;

    TIMER_A0->CCR[0]  = TICKS;
}

void config_NVIC(void){
    __NVIC_EnableIRQ(TA0_N_IRQn); //enables timer A interrupt
}

void gpio_config(void){

    P2->DIR     |= BIT5;           //configure P2.5 to output

    P2->SEL0    &=  ~BIT5;           //enable gpio on P2.5
    P2->SEL1    &=  ~BIT5;
}

void TA0_0_IRQHandler(void){

    P2->DIR     |= BIT5;           //configure P2.5 to output

    P2->SEL0    &=  ~BIT5;           //enable gpio on P2.5
    P2->SEL1    &=  ~BIT5;


}

void TA0_N_IRQHandler(void){
    __NVIC_DisableIRQ(TA0_N_IRQn); //disable since we're in the interrupt

    if(TIMER_A0->CCTL[2] & TIMER_A_CCTLN_CCIFG){

        if(TIMER_A0->CCTL[2] & TIMER_A_CCTLN_CCI){
            CaptureValues[0] = TIMER_A0->CCR[2];
        }
        else{
            CaptureValues[1] = TIMER_A0->CCR[2];

            if(CaptureValues[0] < CaptureValues[1]){
                ElapsedTicks = CaptureValues[1] - CaptureValues[0]; //find elapsed ticks
            }else{
                ElapsedTicks = (CaptureValues[1] + TICKS) - CaptureValues[0];
            }
        }
        // Clear the Interrupt Source Flag
        TIMER_A0->CCTL[2] &= ~TIMER_A_CCTLN_CCIFG;
    }

    __NVIC_EnableIRQ(TA0_N_IRQn); //enable interrupt since we are about to exit handler
}

//change
void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer

    timerA_stop();
    timerA_config();
    timerA_start();
    gpio_config();
    config_NVIC();

    while(1){

        ElapsedTime = ElapsedTicks * TickLength;        //convert ticks to time
        Distance = ElapsedTime * SpeedOfSound / 2;      //centimeters
    }

}
