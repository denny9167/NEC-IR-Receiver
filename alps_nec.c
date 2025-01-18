/*
 * File:   alps_nec.c
 * Author: DennyB
 * Target: PIC16F15213
 * Description: Device is designed to control motorized volume control
 * Created on June 2, 2024, 10:57 AM
 */

#include <xc.h>
#include <stdint.h>
#include "Alps_config.h"

// PORTA aliases
#define IRIN         RA2
#define VOLUP        RA4
#define VOLDN        RA5
#define RXLED        RA1

// Device and Command values
#define DEV_ID       0x10
#define VOLU_CMD     0x20
#define VOLD_CMD     0x30

// Global variables
uint8_t address = 0;
uint8_t inv_address = 0;
uint8_t command = 0;
uint8_t inv_command = 0;


// Receive status LED
void Flash_LED(){
   RXLED = 1;
   __delay_ms(100);
   RXLED = 0;
}

// Idle function and holddown timer
void Idle() {
    uint8_t TimerL = 0;
    uint8_t TimerH = 75;
    // This function polls the IR receiver when idleing.
    // When IRIN goes low, a hold down timer starts,and 
    // once it expires the VOLUP and VOLDN ports are forced off.
    while(IRIN == 1) {
        if(--TimerL == 0 && --TimerH == 0) {
            VOLUP = 0;
            VOLDN = 0;
        }
    }
}

// Check if we have NEC preamble
uint8_t NEC_Start() {
    while(IRIN)
    TMR1 = 0;
    TMR1ON = 1;
    while(!IRIN);    // Wait for high
    if(TMR1 > 9500)  // if greater than 9500uS,invalid timing
    return 0;
    else{
    TMR1 = 0;
    while(IRIN);     // Wait for low
    if(TMR1 > 5000)  // if greater than 5000uS,invalid timing
    return 0;        // Not NEC pre-pulse
    else
    return 1;       // We have NEC pre-pulse
   }
}

// NEC Data extraction
uint8_t NEC_Data() {
    uint8_t data = 0;
    
    for(uint8_t i = 0; i < 8; i++) {
        TMR1 = 0;
        while(!IRIN);       // Wait for high
        if(TMR1 > 700)     // Measure space
        break;             // invalid timing restart 
        
        TMR1 = 0;
        while(IRIN);        // Wait for low
        if(TMR1 > 1800)     // Measure pulse
        break;              // invalid timing
        
        data >>= 1;     // Shift byte LSB first
        if(TMR1 > 1000)
        data |= 0x80;   // Set the bit
    }
    return data;
}
void __interrupt()TMR(void){
    if(TMR1IF){   // If overflow,reset decoding
       TMR1IF = 0;
       TMR1 = 0;
       TMR1ON = 0;
       address = inv_address = command = inv_command = 0;
    }
  }

void main(void) {
    device_config();
    
    while(1) {
        Idle(); //Call idle function
        
        if(NEC_Start()) {
            // If we have preamble
            // Extract 32 bits of data
            address     = NEC_Data();
            inv_address = NEC_Data();
            command     = NEC_Data();
            inv_command = NEC_Data();
            // Repeat delay 
               __delay_ms(50);
            if(address == DEV_ID) {
                switch(command) {
                    case VOLU_CMD:
                        VOLUP = 1;
                        break;
                    case VOLD_CMD:
                        VOLDN = 1;
                        break;
                }
            }
        }
          Flash_LED(); // Receiving data,if flashing
        TMR1ON = 0;
    }
}

