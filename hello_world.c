/*
*
* MTE 325, Lab 1
* --------------
* Oswin Rodrigues, 20520352
* Mahmubul (Momo) Hoque, < INSERT STUDENT NUMBER HERE >
*
*/

// #include <io.h>
#include <stdio.h>
#include <unistd.h>
#include "alt_types.h"
#include "system.h"
#include "sys/alt_irq.h"
#include "altera_avalon_pio_regs.h"

alt_u32 timerPeriod = TIMER_0_FREQ;
volatile alt_u8 buttons = 0x00;
volatile alt_u8 led = 0x00;
volatile alt_u8 pb1_pressed = 0x00;
volatile alt_u8 pb2_pressed = 0x00;
volatile alt_u8 count1 = 0x00;
volatile alt_u8 count2 = 0x00;
volatile alt_u8 switch_state1 = 0x00;
volatile alt_u8 switch_state2 = 0x00;

static void TIMER_0_ISR( void *context, alt_u32 id ){
    IOWR(TIMER_0_BASE, 0, 0x0);

    if ( switch_state1 & (1 << count1) ){
        IOWR(LED_PIO_BASE, 0, 0x02);
    }
    else {
        IOWR(LED_PIO_BASE, 0, 0x00);
    }

    count1 = count1 + 1;

    if (count1 == 8){
        // Stop timer
        IOWR(TIMER_0_BASE, 1, 0x8);
    }
}

void init_timer_0(){
    IOWR( TIMER_0_BASE, 2, (alt_u16)timerPeriod );
    IOWR( TIMER_0_BASE, 3, (alt_u16)(timerPeriod >> 16) );

    IOWR( TIMER_0_BASE, 1, 0x7);

    // Initialize timer interrupt vector
    alt_irq_register( TIMER_0_IRQ, (void *)0, TIMER_0_ISR );

    // Reset to clear the interrupt
    IOWR(TIMER_0_BASE, 0, 0x0);
}

static void TIMER_1_ISR( void *context, alt_u32 id ){
    IOWR(TIMER_1_BASE, 0, 0x0);

    if ( switch_state2 & (1 << count2) ){
        IOWR(SEVEN_SEGMENT_PIO_BASE, 0, 0xCF);
    }
    else {
        IOWR(SEVEN_SEGMENT_PIO_BASE, 0, 0x81);
    }

    count2 = count2 + 1;

    if (count2 == 8){
        // Stop timer
        IOWR(TIMER_1_BASE, 1, 0x8);
    }    
}

void init_timer_1(){
    IOWR( TIMER_1_BASE, 2, (alt_u16)timerPeriod );
    IOWR( TIMER_1_BASE, 3, (alt_u16)(timerPeriod >> 16) );

    IOWR( TIMER_1_BASE, 1, 0x7);

    // Initialize timer interrupt vector
    alt_irq_register( TIMER_1_IRQ, (void *)0, TIMER_1_ISR );

    // Reset to clear the interrupt
    IOWR(TIMER_1_BASE, 0, 0x0);
}

static void BUTTON_ISR( void *context, alt_u32 id ){
    // Get value of last two buttons
    buttons = IORD(BUTTON_PIO_BASE, 3) & 0xf;

    // Figure out if PB1 or PB2 is pressed
    if (buttons == 1){
        pb1_pressed = 1;
    }
    else if (buttons == 2){
        pb2_pressed = 1;
    }

    // Reset to clear the interrupt
    IOWR(BUTTON_PIO_BASE, 3, 0x0);
}

void init_button(){
    // Initialize button interrupt vector
    alt_irq_register( BUTTON_PIO_IRQ, (void *)0, BUTTON_ISR );

    // Reset the edge capture register by writing to it (any value will do)
    IOWR(BUTTON_PIO_BASE, 3, 0x0);

    // Enable interrupts for last two buttons
    IOWR(BUTTON_PIO_BASE, 2, 0xf);
}

volatile alt_u8 count3 = 0x00;

void PULSE_ISR(){
    if (IORD(PIO_PULSE_BASE, 0)){
        IOWR(PIO_RESPONSE_BASE, 0, 1);
    }
    else {
        IOWR(PIO_RESPONSE_BASE, 0, 0);
    }
}

void init_pulse(){
    // Init
    alt_irq_register( PIO_PULSE_IRQ, (void *)0, PULSE_ISR );
    // Reset
    IOWR(PIO_PULSE_BASE, 0, 0);
    // Enable
    IOWR(PIO_PULSE_BASE, 2, 0x1);
}

int main(){
    if((IORD(SWITCH_PIO_BASE,0) >> 15)) {
        //Phase I
        init_button();
        // printf("inited_button\n");
        while (1) {
            while ( !pb1_pressed && !pb2_pressed ) {};
            // printf("exited wait loop\n");
            if ( pb1_pressed ){
                // printf("pb1_pressed\n");
                // printf("%d\n", buttons);
                pb1_pressed = 0;
                count1 = 0;
                switch_state1 = IORD(SWITCH_PIO_BASE, 0) & 0xff;
                init_timer_0();
            }
            else if ( pb2_pressed ){
                // printf("pb2_pressed\n");
                // printf("%d\n", buttons);
                pb2_pressed = 0;
                count2 = 0;
                switch_state2 = IORD(SWITCH_PIO_BASE, 0) & 0xff;
                init_timer_1();
            }
        }
    }
    else {
        //Phase II;

        // Init pulse interrupt
        init_pulse();

        // Init egm
        init(7,7);

        background(500);

        finalize();

        // usleep(10000);

    }

    return 0;
}

