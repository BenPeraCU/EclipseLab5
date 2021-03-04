#include "msp.h"
#define TICKS ((uint16_t)0x4E1F)        //20,000 - 1    meaning 20 mS period
#define PulseLength ((uint16_t)0x0005)  //10    pulse length of 10 ticks

/**
 * main.c
 */


volatile uint16_t CaptureValue = 0; //stores length of recorded pulse

void timerA_launch(void){
    //Stop Timer prior to configuration
    TIMER_A0->CTL &= ~TIMER_A_CTL_MC_MASK;  //Set timer to halted mode

    //***********
    //Configuring the Timer
    //***********

    //Configuring clock to a frequency of 1Mhz, giving 1uS ticks
    //TIMER_A0->CTL       |= TIMER_A_CTL_CLR;       // clears TimerA0
    TIMER_A0->CTL       |= TIMER_A_CTL_SSEL__SMCLK; //Use SMCLK 3MHz
    TIMER_A0->CTL       |= TIMER_A_CTL_ID_0;        // Sets timer ID to 0 ---- division by 1
    TIMER_A0->EX0       |= TIMER_A_EX0_TAIDEX_2;    //Sets timer ID to 3 ---- division by 3

    //configure CCTL2(P2.5) as a falling edge capture input
    TIMER_A0->CCTL[2]   |= TIMER_A_CCTLN_CM__FALLING;   //set as capture input on falling edge
    TIMER_A0->CCTL[2]   |= TIMER_A_CCTLN_CAP;   //set as capture mode
    TIMER_A0->CCTL[2]   |= TIMER_A_CCTLN_SCS;   //set as synchronous capture

    //Set up CCR1 to trigger an interrupt after time PulseLength
    TIMER_A0->CCR[1]    =  PulseLength;

    //clear interrupt flags on relevant registers
    TIMER_A0->CCTL[1]   &= ~TIMER_A_CCTLN_CCIFG; //clear CaptureCompare unit 1 interrupt flag
    TIMER_A0->CCTL[2]   &= ~TIMER_A_CCTLN_CCIFG; //clear CaptureCompare unit 2 interrupt flag
    TIMER_A0->CTL       &= ~TIMER_A_CTL_IFG; //clear timer_A0 interrupt flag

    //configuring interrupts to be enabled on capture compare units 0-2
    TIMER_A0->CCTL[0]   |= TIMER_A_CCTLN_CCIE;
    TIMER_A0->CCTL[1]   |= TIMER_A_CCTLN_CCIE;
    TIMER_A0->CCTL[2]   |= TIMER_A_CCTLN_CCIE;


    //*************
    //Starting the timer
    //***********

    //Set TIMER_A0 to up mode
    TIMER_A0->CTL |= TIMER_A_CTL_MC__UP;

    //Start TIMER_A0 by loading CCR0 with a non-zero value
    TIMER_A0->CCR[0]  = TICKS;
}

void config_NVIC(void){
    //enables timer A interrupt for non-zero CaptureCompare Units
    __NVIC_EnableIRQ(TA0_N_IRQn);
    //enables timer A interrupt for CaptureCompare Unit 0
    __NVIC_EnableIRQ(TA0_0_IRQn);
}

void TA0_0_IRQHandler(void){

    P2->DIR     |= BIT5;           //configure P2.5 to output

    P2->SEL0    &=  ~BIT5;         //enable GPIO on P2.5
    P2->SEL1    &=  ~BIT5;

    P2->OUT     |=  BIT5;          //Enable P2.5 as HIGH

    // Clear the Interrupt Source Flag
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
}

void TA0_N_IRQHandler(void){
    __NVIC_DisableIRQ(TA0_N_IRQn); //disable since we're in the interrupt

    //if interrupt is on Capture Compare Unit 2
    if(TIMER_A0->CCTL[2] & TIMER_A_CCTLN_CCIFG){

        //Store captured value into useful variable
        CaptureValue = TIMER_A0->CCR[2];

        // Clear the Interrupt Source Flag
        TIMER_A0->CCTL[2] &= ~TIMER_A_CCTLN_CCIFG;
    }

    //if interrupt is on Capture Compare Unit 1
    else if(TIMER_A0->CCTL[1] & TIMER_A_CCTLN_CCIFG){

        //set output to LOW on P2.5
        P2->OUT &= ~BIT5;

        //configure P2.5 as an input
        P2->DIR     &=  ~BIT5;           //configure P2.5 to input
        P2->SEL0    |=  BIT5;           //enable P2.5 as an input to TIMER_A0 (primary module function)
        P2->SEL1    &=  ~BIT5;

        // Clear the Interrupt Source Flag
        TIMER_A0->CCTL[1] &= ~TIMER_A_CCTLN_CCIFG;
    }

    __NVIC_EnableIRQ(TA0_N_IRQn); //enable interrupt since we are about to exit handler
}

//change
void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer

    timerA_launch();

    config_NVIC();

    while(1){
        //processor just chilling waiting for interrupts
    }

}
