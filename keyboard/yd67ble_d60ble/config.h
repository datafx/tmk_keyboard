/*
Copyright 2011 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CONFIG_H
#define CONFIG_H


#define VENDOR_ID       0xFEED
#define PRODUCT_ID      0x2167
#define DEVICE_VER      0x0001
#define MANUFACTURER    YANG
#define PRODUCT         YD67BLE Keyboard
#define DESCRIPTION     t.m.k. keyboard firmware for YD67BLE


#define MATRIX_ROWS 5  
#define MATRIX_COLS 16

/* debounce for both key up and down */
#define DEBOUNCE_DN 5
#define DEBOUNCE_NK 1
#define DEBOUNCE_UP 5


#define SUSPEND_ACTION

#define TAPPING_TOGGLE  2


/* Locking resynchronize hack */

/* key combination for command */
#define IS_COMMAND() ( \
    (keyboard_report->mods == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT))) || \
    (keyboard_report->mods == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_LCTRL) | MOD_BIT(KC_RSHIFT))) \
)


/*
 * Hardware Serial(UART)
 */
#if defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB1286__)
    /* iom32u4.h has no definition of UCSR1D. copy from iom32u2.h */
    #define UCSR1D _SFR_MEM8(0xCB)
    #define RTSEN 0
    #define CTSEN 1

    #define SERIAL_UART_BAUD        76800
    #define SERIAL_UART_DATA        UDR1
    #define SERIAL_UART_UBRR        ((F_CPU/(8.0*SERIAL_UART_BAUD)-1+0.5))
    #define SERIAL_UART_RXD_VECT    USART1_RX_vect
    #define SERIAL_UART_TXD_READY   (UCSR1A&(1<<UDRE1))
    #define SERIAL_UART_INIT()      do { \
        cli(); \
        UBRR1L = (uint8_t) SERIAL_UART_UBRR;       /* baud rate */ \
        UBRR1H = ((uint16_t)SERIAL_UART_UBRR>>8);  /* baud rate */ \
        UCSR1B |= (1<<RXCIE1) | (1<<RXEN1); /* RX interrupt, RX: enable */ \
        UCSR1B |= (0<<TXCIE1) | (1<<TXEN1); /* TX interrupt, TX: enable */ \
        UCSR1C |= (0<<UPM11) | (0<<UPM10);  /* parity: none(00), even(01), odd(11) */ \
        UCSR1A |= (1<<U2X1); /* 2x speed */ \
        sei(); \
    } while(0)
#else
    #error "USART configuration is needed."
#endif
/* BT Power Control */
#define BT_POWERED    (~PORTF & (1<<7))
#define bt_power_init()    do { DDRF |= (1<<7); PORTF &= ~(1<<7);} while(0)
#define turn_off_bt()    do { PORTF |= (1<<7); UCSR1B &= ~(1<<TXEN1); } while(0)
#define turn_on_bt()    do { PORTF &= ~(1<<7); UCSR1B |= (1<<TXEN1);} while(0)

/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
#define NO_DEBUG

/* disable print */
#define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT
//#define NO_ACTION_MACRO
//#define NO_ACTION_FUNCTION
#endif
