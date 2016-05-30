/*
*
* MTE 325, Lab 1
* --------------
* Oswin Rodrigues, 20520352
* Mahmubul (Momo) Hoque, < INSERT STUDENT NUMBER HERE >
*
*/

#include <io.h>
#include <stdio.h>
#include <unistd.h>
#include "alt_types.h"
#include "system.h"
#include "sys/alt_irq.h"
#include "altera_avalon_pio_regs.h"

#define PHASE_2_INTERRUPT   // macro for which sync method to use in phase 2
#define MAX_EGM 14          // maximum value for egm's period and duty cycle
#define BG_GRAN_MUL 50      // bg granularity multiplier i.e. increment in chunks for test
#define MAX_BG_GRAN_ITER 10 // max number of test cases to examine bg granularity for

/*
*
* Phase 1 Functions
*
*/

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

void execute_phase_1(){
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

/*
*
* Phase 2 Functions
*
*/

// Defined in egm.c
void init(int, int);
void background(int);
void finalize(void);

// Experiment variables
alt_u8 period = 0;              // egm period
alt_u8 duty = 0;                // egm duty cycle
alt_u8 bg_gran = 0;         // bg task granularity
alt_u8 bg_gran_iterator = 0;    // bg task granularity counter
volatile alt_u8 event_count = 0;    // num events observed

#ifdef PHASE_2_INTERRUPT

// Method 1 for phase 2: interrupts

    void PULSE_ISR(){
        if (IORD(PIO_PULSE_BASE, 0)){
            IOWR(PIO_RESPONSE_BASE, 0, 1);
            event_count = event_count + 1;
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
        IOWR(PIO_PULSE_BASE, 2, 1);
    }

    void execute_phase_2_interrupt(){
        // Init pulse interrupt
        init_pulse();

        for (period = 2; period <= MAX_EGM; ++period){
        // Start from period = 2 and not 1 because want maximum 50 % duty, but smallest duty possible is 1
        // max_duty = period / 2; truncate down because want duty <= 50 % of period

            for (duty = 1; duty <= period / 2; ++duty){

                for (bg_gran_iterator = 1; bg_gran_iterator <= MAX_BG_GRAN_ITER; ++bg_gran_iterator){
                // bg_gran ranges from BG_GRAN_MUL to (MAX_BG_GRAN_ITER * BG_GRAN_MUL) in increments of BG_GRAN_MUL
                
                    bg_gran = bg_gran_iterator * BG_GRAN_MUL;
                    
                    // Init egm
                    init(period, duty);

                    // run bg until event_count hits 100
                    event_count = 0;
                    while (event_count < 100){
                        // Run bg task
                        background(bg_gran);
                    }

                    // Record results
                    finalize();
                    
                    // usleep(10);
                }
            }
        }
    }

#else

// Method 2 for phase 2: periodic polling

    static void TIMER_2_ISR( void *context, alt_u32 id ){
        // Set low
        IOWR(TIMER_2_BASE, 0, 0x0);

        // Stop
        IOWR(TIMER_2_BASE, 1, 0x8);
    }

    void init_timer_2( alt_u32 timerPeriod ){
        IOWR( TIMER_2_BASE, 2, (alt_u16)timerPeriod );
        IOWR( TIMER_2_BASE, 3, (alt_u16)(timerPeriod >> 16) );

        IOWR( TIMER_2_BASE, 1, 0x7);

        // Initialize timer interrupt vector
        alt_irq_register( TIMER_2_IRQ, (void *)0, TIMER_2_ISR );

        // Reset to clear the interrupt
        IOWR(TIMER_2_BASE, 0, 0x0);
    }

    void execute_phase_2_periodic_polling(){

    }

#endif

// TODO: Change repeated use of init_timer to actually stopping and resetting same timer

int main(){
    if(IORD(SWITCH_PIO_BASE, 0) >> 15) {
        execute_phase_1();
    }
    else {
        #ifdef PHASE_2_INTERRUPT
            execute_phase_2_interrupt();
        #else
            execute_phase_2_periodic_polling();
        #endif
    }
    return 0;
}

