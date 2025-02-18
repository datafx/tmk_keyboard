/*
Copyright 2013 Jun Wako <wakojun@gmail.com>

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
#ifndef ACTION_UTIL_H
#define ACTION_UTIL_H

#include <stdint.h>
#include "report.h"

#ifdef __cplusplus
extern "C" {
#endif

extern report_keyboard_t *keyboard_report;
extern uint8_t block_mods;
extern uint8_t lock_mods;
extern bool has_mods_key;
#define MODS_SHIFT_MASK (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT))
#define MODS_GUI_MASK   (MOD_BIT(KC_LGUI) | MOD_BIT(KC_RGUI))

void send_keyboard_report(void);

/* key */
#define add_key(code) update_key(code,1)
#define del_key(code) update_key(code,0)
void update_key(uint8_t key, bool add);
void clear_keys(void);

/* modifier */
uint8_t get_mods(void);
void add_mods(uint8_t mods);
void del_mods(uint8_t mods);
void set_mods(uint8_t mods);
void clear_mods(void);

/* weak modifier */
uint8_t get_weak_mods(void);
void add_weak_mods(uint8_t mods);
void del_weak_mods(uint8_t mods);
void set_weak_mods(uint8_t mods);
void clear_weak_mods(void);

/* oneshot modifier */
void set_oneshot_mods(uint8_t mods);
void clear_oneshot_mods(void);
void oneshot_toggle(void);
void oneshot_enable(void);
void oneshot_disable(void);

/* inspect */
uint8_t has_anykey(void);
uint8_t has_anymod(void);
uint8_t get_first_key(void);

#ifdef __cplusplus
}
#endif

#endif
