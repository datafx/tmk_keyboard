/*
Copyright 2012,2013 Jun Wako <wakojun@gmail.com>

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
#include "host.h"
#include "keycode.h"
#include "keyboard.h"
#include "mousekey.h"
#include "command.h"
#include "led.h"
#include "backlight.h"
#include "action_layer.h"
#include "action_tapping.h"
#include "action_macro.h"
#include "action_util.h"
#include "action.h"
#include "hook.h"
#include "wait.h"

#ifdef __AVR__
#include <avr/wdt.h>
#endif

#ifdef DEBUG_ACTION
#include "debug.h"
#else
#include "nodebug.h"
#endif


void action_exec(keyevent_t event)
{
    if (!IS_NOEVENT(event)) {
        dprint("\n---- action_exec: start -----\n");
        dprint("EVENT: "); debug_event(event); dprintln();
        //hook_matrix_change(event); // this makes it run twice
    }

    keyrecord_t record = { .event = event };

#ifndef NO_ACTION_TAPPING
    action_tapping_process(record);
#else
    process_action(&record);
    if (!IS_NOEVENT(record.event)) {
        dprint("processed: "); debug_record(record); dprintln();
    }
#endif
}

void process_action(keyrecord_t *record)
{
    keyevent_t event = record->event;
#ifndef NO_ACTION_TAPPING
    uint8_t tap_count = record->tap.count;
#endif

    if (IS_NOEVENT(event)) { return; }
    action_t action;
    if (event.time != 0b10) { //if 0b10��process it as action code
        action = layer_switch_get_action(event);
    } else {
        action.code = *(uint16_t *)&event.key;
    }

    dprint("ACTION: "); debug_action(action);
#ifndef NO_ACTION_LAYER
    dprint(" layer_state: "); layer_debug();
    dprint(" default_layer_state: "); default_layer_debug();
#endif
    dprintln();

    switch (action.kind.id) {
        /* Key and Mods */
        case ACT_LMODS:
        case ACT_RMODS:
            {
                uint8_t mods = (action.kind.id == ACT_LMODS) ?  action.key.mods :
                                                                action.key.mods<<4;
                if (event.pressed) {
                    block_mods |= (mods & get_mods());
                    if (mods && action.key.code) has_mods_key = 1;
                    /*
                     if ((get_mods() & MODS_SHIFT_MASK) && (mods & MODS_SHIFT_MASK)) {
                        block_mods |= MODS_SHIFT_MASK;
                        mods &= ~MODS_SHIFT_MASK;
                    }*/
                    if (mods) {
                        add_weak_mods(mods);
                        send_keyboard_report();
                    }
                    register_code(action.key.code);
                } else {
                    block_mods &= ~mods;
                    has_mods_key = 0;
                    //block_mods &= ~MODS_SHIFT_MASK;
                    unregister_code(action.key.code);
                    if (mods) {
                        del_weak_mods(mods);
                        send_keyboard_report();
                    }
                }
            }
            break;
#ifndef NO_ACTION_TAPPING
        case ACT_LMODS_TAP:
        case ACT_RMODS_TAP:
            {
                uint8_t mods = (action.kind.id == ACT_LMODS_TAP) ?  action.key.mods :
                                                                    action.key.mods<<4;
                switch (action.key.code) {
    #ifndef NO_ACTION_ONESHOT
                    case MODS_ONESHOT:
                        // Oneshot modifier
                        if (event.pressed) {
                            if (tap_count == 0) {
                                register_mods(mods);
                            }
                            else if (tap_count == 1) {
                                dprint("MODS_TAP: Oneshot: start\n");
                                set_oneshot_mods(mods);
                            }
                            else {
                                register_mods(mods);
                            }
                        } else {
                            if (tap_count == 0) {
                                clear_oneshot_mods();
                                unregister_mods(mods);
                            }
                            else if (tap_count == 1) {
                                // Retain Oneshot mods
                            }
                            else {
                                clear_oneshot_mods();
                                unregister_mods(mods);
                            }
                        }
                        break;
    #endif
                    case MODS_TAP_TOGGLE:
                        if (event.pressed) {
                            if (tap_count <= TAPPING_TOGGLE) {
                                if (mods & get_mods()) {
                                    dprint("MODS_TAP_TOGGLE: toggle mods off\n");
                                    unregister_mods(mods);
                                } else {
                                    dprint("MODS_TAP_TOGGLE: toggle mods on\n");
                                    register_mods(mods);
                                }
                            }
                        } else {
                            if (tap_count < TAPPING_TOGGLE) {
                                dprint("MODS_TAP_TOGGLE: release : unregister_mods\n");
                                unregister_mods(mods);
                            }
                        }
                        break;
                    default:
                        if (event.pressed) {
                            if (tap_count > 0) {
                                if (record->tap.interrupted) {
                                    dprint("MODS_TAP: Tap: Cancel: add_mods\n");
                                    // ad hoc: set 0 to cancel tap
                                    record->tap.count = 0;
                                    register_mods(mods);
                                } else {
                                    dprint("MODS_TAP: Tap: register_code\n");
                                    register_code(action.key.code);
                                    
                                    // Delay for MacOS CapsLock
                                    if (action.key.code == KC_CAPSLOCK) {
                                        WAIT_MS(100);
                                    }
                                }
                            } else {
                                dprint("MODS_TAP: No tap: add_mods\n");
                                register_mods(mods);
                            }
                        } else {
                            if (tap_count > 0) {
                                dprint("MODS_TAP: Tap: unregister_code\n");
                                unregister_code(action.key.code);
                            } else {
                                dprint("MODS_TAP: No tap: add_mods\n");
                                unregister_mods(mods);
                            }
                        }
                        break;
                }
            }
            break;
#endif
#ifdef EXTRAKEY_ENABLE
        /* other HID usage */
        case ACT_USAGE:
            switch (action.usage.page) {
                case PAGE_CONSUMER:
                    if (event.pressed) {
                        host_consumer_send(action.usage.code);
                    } else {
#ifdef VUSB  //if there is no wait and quickly press and release consumer key, it may stuck.
                        WAIT_MS(30);
#endif
                        host_consumer_send(0);
                    }
                    break;
#ifndef NO_SYSTEMKEY
                case PAGE_SYSTEM:
                    if (event.pressed) {
#ifndef NO_ACTION_MACRO
                        if (action.usage.code >= 0x300 ) {
                            xprintf("lanuch shortcut %X%X\n", (action.usage.code&0xF0) >> 4, action.usage.code&0xF);
                            action_macro_play(MACRO( TA(MOD_LGUI|KC_R), W(100), END ));  //Win+R
                            //Use 0x99 as 0d99. this costs 4byte but more readable.
                            type_num(action.key.code >> 4);
#ifdef VUSB  //if 
                            WAIT_MS(100);
#endif
                            type_num(action.key.code & 0xF);
                            action_macro_play(MACRO( T(ENTER), END ));
                            break;
                        } else
#endif
                        host_system_send(action.usage.code);
                    } else {
                        host_system_send(0);
                    }
                    break;
#endif
            }
            break;
#endif
#ifdef MOUSEKEY_ENABLE
        /* Mouse key */
        case ACT_MOUSEKEY:
            if (event.pressed) {
                mousekey_on(action.key.code);
                mousekey_send();
                if (action.key.code < 70) {
                    rapidfire_key[action.key.code] = true;
                    rapidfire_mode = true;
                }
            } else {
                mousekey_off(action.key.code);
                mousekey_send();
                if (action.key.code < 70) {
                    rapidfire_key[action.key.code] = false;
                    uint16_t rapidfire_key_code = action.key.code?action.key.code:0x50F4;
                    process_action_code(rapidfire_key_code, 0);
                    //rapidfire_mode = false;
                }
            }
            break;
#endif
#ifndef NO_ACTION_LAYER
        case ACT_LAYER:
            if (action.layer_bitop.on == 0) {
#ifndef NO_DEFAULT_LAYER_SET  //344B
                /* Default Layer Bitwise Operation */
                if (!event.pressed) {
                    uint8_t shift = action.layer_bitop.part*4;
                    uint32_t bits = ((uint32_t)action.layer_bitop.bits)<<shift;
                    uint32_t mask = (action.layer_bitop.xbit) ? ~(((uint32_t)0xf)<<shift) : 0;
                    switch (action.layer_bitop.op) {
                        case OP_BIT_AND: default_layer_and(bits | mask); break;
                        case OP_BIT_OR:  default_layer_or(bits | mask);  break;
                        case OP_BIT_XOR: default_layer_xor(bits | mask); break;
                        case OP_BIT_SET: default_layer_and(mask); default_layer_or(bits); break;
                    }
                }
#endif
            } else {
                /* Layer Bitwise Operation */
                if (event.pressed ? (action.layer_bitop.on & ON_PRESS) :
                                    (action.layer_bitop.on & ON_RELEASE)) {
                    uint8_t shift = action.layer_bitop.part*4;
                    uint32_t bits = ((uint32_t)action.layer_bitop.bits)<<shift;
                    uint32_t mask = (action.layer_bitop.xbit) ? ~(((uint32_t)0xf)<<shift) : 0;
                    switch (action.layer_bitop.op) {
                        case OP_BIT_AND: layer_and(bits | mask); break;
                        case OP_BIT_OR:  layer_or(bits | mask);  break;
                        case OP_BIT_XOR: layer_xor(bits | mask); break;
                        case OP_BIT_SET: layer_and(mask); layer_or(bits); break;
                    }
                }
            }
            break;
    #ifndef NO_ACTION_TAPPING
        case ACT_LAYER_TAP:
        case ACT_LAYER_TAP_EXT:
            switch (action.layer_tap.code) {
                case 0xe0 ... 0xef:
                    /* layer On/Off with modifiers(left only) */
                    if (event.pressed) {
                        layer_on(action.layer_tap.val);
                        register_mods(action.layer_tap.code & 0x0f);
                    } else {
                        layer_off(action.layer_tap.val);
                        unregister_mods(action.layer_tap.code & 0x0f);
                    }
            #if 0
                case 0xc0 ... 0xdf:
                    if (event.pressed) {
                        layer_on(action.layer_tap.val);
                        register_mods((action.layer_tap.code & 0x10) ?
                                (action.layer_tap.code & 0x0f) << 4 :
                                (action.layer_tap.code & 0x0f));
                    } else {
                        layer_off(action.layer_tap.val);
                        unregister_mods((action.layer_tap.code & 0x10) ?
                                (action.layer_tap.code & 0x0f) << 4 :
                                (action.layer_tap.code & 0x0f));
                    }
            #endif
                    break;
                case OP_TAP_TOGGLE:
                    /* tap toggle */
                    if (event.pressed) {
                        if (tap_count < TAPPING_TOGGLE) {
                            layer_invert(action.layer_tap.val);
                        }
                    } else {
                        if (tap_count <= TAPPING_TOGGLE) {
                            layer_invert(action.layer_tap.val);
                        }
                    }
                    break;
                case OP_ON_OFF:
                    event.pressed ? layer_on(action.layer_tap.val) :
                                    layer_off(action.layer_tap.val);
                    break;
                case OP_OFF_ON:
                    event.pressed ? layer_off(action.layer_tap.val) :
                                    layer_on(action.layer_tap.val);
                    break;
                case OP_SET_CLEAR:
                    event.pressed ? layer_move(action.layer_tap.val) :
                                    layer_clear();
                    break;
                default:
                    /* tap key */
#ifndef IMPROVED_TAP
                    if (event.pressed) {
                        if (tap_count > 0) {
                            dprint("KEYMAP_TAP_KEY: Tap: register_code\n");
                            register_code(action.layer_tap.code);

                            // Delay for MacOS CapsLock
                            if (action.layer_tap.code == KC_CAPSLOCK) {
                                WAIT_MS(100);
                            }
                        } else {
                            dprint("KEYMAP_TAP_KEY: No tap: On on press\n");
                            layer_on(action.layer_tap.val);
                        }
                    } else {
                        if (tap_count > 0) {
                            dprint("KEYMAP_TAP_KEY: Tap: unregister_code\n");
                            unregister_code(action.layer_tap.code);
                        } else {
                            dprint("KEYMAP_TAP_KEY: No tap: Off on release\n");
                            layer_off(action.layer_tap.val);
                        }
                    }
                    break;
#else
                    if (event.pressed) {
                        if (tap_count >0 && !((action.layer_tap.val & 0x8) && record->tap.interrupted))  {
                            dprint("KEYMAP_TAP_KEY: Tap: register_code\n");
                            register_code(action.layer_tap.code);

                            // Delay for MacOS CapsLock
                            if (action.layer_tap.code == KC_CAPSLOCK) {
                                WAIT_MS(100);
                            }
                        } else {
                            dprint("KEYMAP_TAP_KEY: No tap: On on press\n");
                            layer_on(action.layer_tap.val & 0x7);
                            // LT (P)
                            if (action.layer_tap.val & 0x10) {
                                action_t ltp_action = action_for_key(action.layer_tap.val & 0x7, event.key);
                                process_action_code(ltp_action.code, 1);
                            }
                        }
                    } else {
                        if (tap_count >0 && !((action.layer_tap.val & 0x8) && record->tap.interrupted))  {
                            dprint("KEYMAP_TAP_KEY: Tap: unregister_code\n");
                            unregister_code(action.layer_tap.code);
                        } else {
                            dprint("KEYMAP_TAP_KEY: No tap: Off on release\n");
                            layer_off(action.layer_tap.val & 0x7);
                            // LT (P)
                            if (action.layer_tap.val & 0x10) {
                                action_t ltp_action = action_for_key(action.layer_tap.val & 0x7, event.key);
                                process_action_code(ltp_action.code, 0);
                            }
                        }
                    }
                    break;
#endif
            }
            break;
    #endif
#endif
        /* Extentions */
#ifndef NO_ACTION_MACRO
        case ACT_MACRO:
            action_macro_play(action_get_macro(record, action.func.id, action.func.opt));
            break;
#endif
#if defined(BACKLIGHT_ENABLE) || defined(NOEEP_BACKLIGHT_ENABLE)
        case ACT_BACKLIGHT:
            if (!event.pressed) {
                /* Backwards compatibility */
                if (action.backlight.level != 0 && action.backlight.opt != BACKLIGHT_LEVEL) {
                    action.backlight.opt = action.backlight.level;
                }
                switch (action.backlight.opt) {
                    case BACKLIGHT_INCREASE:
                        backlight_increase();
                        break;
                    case BACKLIGHT_DECREASE:
                        backlight_decrease();
                        break;
                    case BACKLIGHT_TOGGLE:
                        backlight_toggle();
                        break;
                    case BACKLIGHT_STEP:
                        backlight_step();
                        break;
                    case BACKLIGHT_LEVEL:
                        backlight_level(action.backlight.level);
                        break;
                }
            }
            break;
#endif
        case ACT_COMMAND:
            break;
#ifndef NO_ACTION_FUNCTION
        case ACT_FUNCTION:
            action_function(record, action.func.id, action.func.opt);
            break;
#endif
        default:
            break;
    }
}




/*
 * Utilities for actions.
 */
void register_code(uint8_t code)
{
    if (code == KC_NO) {
        return;
    }
#ifndef NO_ACTION_MACRO
    if (code == KC_KP_00) {
        action_macro_play(MACRO( T(P0), T(P0),  END ));
        //test ��ǰ���¹�
        //action_macro_play(MACRO(UC(0x5e8a), UC(0x524d), UC(0x660e), UC(0x6708), UC(0x5149),  END ));
        return;     
    }
#endif

#ifdef LOCKING_SUPPORT_ENABLE
    else if (KC_LOCKING_CAPS == code) {
#ifdef LOCKING_RESYNC_ENABLE
        // Resync: ignore if caps lock already is on
        if (host_keyboard_leds() & (1<<USB_LED_CAPS_LOCK)) return;
#endif
        add_key(KC_CAPSLOCK);
        send_keyboard_report();
        wait_ms(100);
        del_key(KC_CAPSLOCK);
        send_keyboard_report();
    }

    else if (KC_LOCKING_NUM == code) {
#ifdef LOCKING_RESYNC_ENABLE
        if (host_keyboard_leds() & (1<<USB_LED_NUM_LOCK)) return;
#endif
        add_key(KC_NUMLOCK);
        send_keyboard_report();
        wait_ms(100);
        del_key(KC_NUMLOCK);
        send_keyboard_report();
    }

    else if (KC_LOCKING_SCROLL == code) {
#ifdef LOCKING_RESYNC_ENABLE
        if (host_keyboard_leds() & (1<<USB_LED_SCROLL_LOCK)) return;
#endif
        add_key(KC_SCROLLLOCK);
        send_keyboard_report();
        wait_ms(100);
        del_key(KC_SCROLLLOCK);
        send_keyboard_report();
    }
#endif

    else if IS_KEY(code) {
        // TODO: should push command_proc out of this block?
        if (command_proc(code)) return;

#ifndef NO_ACTION_ONESHOT
/* TODO: remove
        if (oneshot_state.mods && !oneshot_state.disabled) {
            uint8_t tmp_mods = get_mods();
            add_mods(oneshot_state.mods);

            add_key(code);
            send_keyboard_report();

            set_mods(tmp_mods);
            send_keyboard_report();
            oneshot_cancel();
        } else 
*/
#endif
        {
            add_key(code);
            send_keyboard_report();
        }
    }
    else if IS_MOD(code) {
        add_mods(MOD_BIT(code));
        send_keyboard_report();
    }
#ifndef UNIMAP_ENABLE
#ifndef NO_SYSTEMKEY
    else if IS_SYSTEM(code) {
        host_system_send(KEYCODE2SYSTEM(code));
    }
#endif
    else if IS_CONSUMER(code) {
        host_consumer_send(KEYCODE2CONSUMER(code));
    }
#endif
}

void unregister_code(uint8_t code)
{
    if (code == KC_NO) {
        return;
    }

#ifdef LOCKING_SUPPORT_ENABLE
    else if (KC_LOCKING_CAPS == code) {
#ifdef LOCKING_RESYNC_ENABLE
        // Resync: ignore if caps lock already is off
        if (!(host_keyboard_leds() & (1<<USB_LED_CAPS_LOCK))) return;
#endif
        add_key(KC_CAPSLOCK);
        send_keyboard_report();
        wait_ms(100);
        del_key(KC_CAPSLOCK);
        send_keyboard_report();
    }

    else if (KC_LOCKING_NUM == code) {
#ifdef LOCKING_RESYNC_ENABLE
        if (!(host_keyboard_leds() & (1<<USB_LED_NUM_LOCK))) return;
#endif
        add_key(KC_NUMLOCK);
        send_keyboard_report();
        wait_ms(100);
        del_key(KC_NUMLOCK);
        send_keyboard_report();
    }

    else if (KC_LOCKING_SCROLL == code) {
#ifdef LOCKING_RESYNC_ENABLE
        if (!(host_keyboard_leds() & (1<<USB_LED_SCROLL_LOCK))) return;
#endif
        add_key(KC_SCROLLLOCK);
        send_keyboard_report();
        wait_ms(100);
        del_key(KC_SCROLLLOCK);
        send_keyboard_report();
    }
#endif

    else if IS_KEY(code) {
        del_key(code);
        send_keyboard_report();
    }
    else if IS_MOD(code) {
        del_mods(MOD_BIT(code));
        send_keyboard_report();
    }
#ifndef NO_SYSTEMKEY
    else if IS_SYSTEM(code) {
        host_system_send(0);
    }
#endif
    else if IS_CONSUMER(code) {
        host_consumer_send(0);
    }
}

void register_mods(uint8_t mods)
{
    if (mods) {
        add_mods(mods);
        send_keyboard_report();
    }
}

void unregister_mods(uint8_t mods)
{
    if (mods) {
        del_mods(mods);
        send_keyboard_report();
    }
}

void clear_keyboard(void)
{
    clear_mods();
    clear_keyboard_but_mods();
}

void clear_keyboard_but_mods(void)
{
    clear_weak_mods();
    clear_keys();
    send_keyboard_report();
#ifdef MOUSEKEY_ENABLE
    mousekey_clear();
    mousekey_send();
#endif
#ifdef EXTRAKEY_ENABLE
#ifndef NO_SYSTEMKEY
    host_system_send(0);
#endif
    host_consumer_send(0);
#endif
}

bool is_tap_key(keyevent_t event)
{
    action_t action = layer_switch_get_action(event);

    switch (action.kind.id) {
        case ACT_LMODS_TAP:
        case ACT_RMODS_TAP:
            switch (action.key.code) {
                case MODS_ONESHOT:
                case MODS_TAP_TOGGLE:
                case KC_A ... KC_EXSEL:                 // tap key
                case KC_LCTRL ... KC_RGUI:              // tap key
                    return true;
            }
        case ACT_LAYER_TAP:
        case ACT_LAYER_TAP_EXT:
            switch (action.layer_tap.code) {
                case 0xe0 ... 0xef:
                //case 0xc0 ... 0xdf:         // with modifiers
                    return false;
                case KC_A ... KC_EXSEL:     // tap key
                case OP_TAP_TOGGLE:
                    return true;
            }
            return false;
        case ACT_MACRO:
        case ACT_FUNCTION:
            if (action.func.opt & FUNC_TAP) { return true; }
            return false;
    }
    return false;
}

void process_action_code(uint16_t code, bool pressed) {
    keyrecord_t keyr;
    keyr.event =(keyevent_t){
            .key = (keypos_t){ .row = code >> 8, .col = code},
            .pressed = pressed,
            .time = 0b10 /* bit0 should normally be 1 */
            };
    process_action(&keyr);
}

void tap_action_code(uint16_t code) {
    process_action_code(code, 1);
    process_action_code(code, 0);
}


void type_number(uint8_t num, bool numpad)
{
    if (num == 0) num = 10;   //1->0 0x1E->0x27
    if (numpad) num += 0x58;
    else num += 0x1D;
    tap_action_code(num);
}
/*
void type_pnum(uint8_t pnum)
{
    if (pnum == 0) pnum = 10;   //P1->P0 0x59->0x62
    pnum += 0x58;
    tap_action_code(pnum);
}
*/

void type_numbers(uint16_t value, bool numpad)
{
    for (uint16_t i=10000; i>=1; i=i/10) {
        uint8_t this_num = value/i % 10;
        if (value/i) {
            type_number(this_num, numpad);
        }
    }
}
/*
 * debug print
 */
void debug_event(keyevent_t event)
{
    dprintf("%04X%c(%u)", (event.key.row<<8 | event.key.col), (event.pressed ? 'd' : 'u'), event.time);
}

void debug_record(keyrecord_t record)
{
    debug_event(record.event);
#ifndef NO_ACTION_TAPPING
    dprintf(":%u%c", record.tap.count, (record.tap.interrupted ? '-' : ' '));
#endif
}

void debug_action(action_t action)
{
    switch (action.kind.id) {
        case ACT_LMODS:             dprint("ACT_LMODS");             break;
        case ACT_RMODS:             dprint("ACT_RMODS");             break;
        case ACT_LMODS_TAP:         dprint("ACT_LMODS_TAP");         break;
        case ACT_RMODS_TAP:         dprint("ACT_RMODS_TAP");         break;
        case ACT_USAGE:             dprint("ACT_USAGE");             break;
        case ACT_MOUSEKEY:          dprint("ACT_MOUSEKEY");          break;
        case ACT_LAYER:             dprint("ACT_LAYER");             break;
        case ACT_LAYER_TAP:         dprint("ACT_LAYER_TAP");         break;
        case ACT_LAYER_TAP_EXT:     dprint("ACT_LAYER_TAP_EXT");     break;
        case ACT_MACRO:             dprint("ACT_MACRO");             break;
        case ACT_COMMAND:           dprint("ACT_COMMAND");           break;
        case ACT_FUNCTION:          dprint("ACT_FUNCTION");          break;
        default:                    dprint("UNKNOWN");               break;
    }
    dprintf("[%X:%02X]", action.kind.param>>8, action.kind.param&0xff);
}
