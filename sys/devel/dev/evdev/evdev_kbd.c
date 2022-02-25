/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <dev/misc/wscons/wsksymdef.h>
#include <dev/misc/wscons/wsksymvar.h>

#include <dev/evdev/input-event-codes.h>

/* evdev scancode to wskbd */
struct kbd_conv {
	keysym_t scancode;
	keysym_t ascii_normal;
	keysym_t ascii_shift;
} keys[] = {
	{ KEY_ESC, 				KS_Escape },
	{ KEY_1, 				KS_1 },
	{ KEY_2, 				KS_2 },
	{ KEY_3, 				KS_3 },
	{ KEY_4, 				KS_4 },
	{ KEY_5, 				KS_5 },
	{ KEY_6, 				KS_6 },
	{ KEY_7, 				KS_7 },
	{ KEY_8, 				KS_8 },
	{ KEY_9, 				KS_9 },
	{ KEY_0, 				KS_0 },
	{ KEY_MINUS, 			KS_minus },
	{ KEY_EQUAL, 			KS_equal },
	{ KEY_BACKSPACE, 		KS_BackSpace },
	{ KEY_TAB, 				KS_Tab },
	{ KEY_Q, 				KS_q, 			KS_Q },
	{ KEY_W, 				KS_w, 			KS_W },
	{ KEY_E, 				KS_e, 			KS_E },
	{ KEY_R, 				KS_r, 			KS_R },
	{ KEY_T, 				KS_t, 			KS_T },
	{ KEY_Y, 				KS_y, 			KS_Y },
	{ KEY_U, 				KS_u, 			KS_U },
	{ KEY_I, 				KS_i, 			KS_I },
	{ KEY_O, 				KS_o, 			KS_O },
	{ KEY_P, 				KS_p, 			KS_P },
	{ KEY_LEFTBRACE, 		KS_braceleft },
	{ KEY_RIGHTBRACE, 		KS_braceright },
	{ KEY_ENTER, 			KS_Return },
	{ KEY_LEFTCTRL, 		KS_Control_L },
	{ KEY_A, 				KS_a, 			KS_A },
	{ KEY_S, 				KS_s, 			KS_S },
	{ KEY_D, 				KS_d, 			KS_D },
	{ KEY_F, 				KS_f, 			KS_F },
	{ KEY_G, 				KS_g, 			KS_G },
	{ KEY_H, 				KS_h, 			KS_H },
	{ KEY_J, 				KS_j, 			KS_J },
	{ KEY_K, 				KS_k, 			KS_K },
	{ KEY_L, 				KS_l, 			KS_L },
	{ KEY_SEMICOLON, 		KS_semicolon },
	{ KEY_APOSTROPHE, 		KS_apostrophe },
	{ KEY_GRAVE, 			KS_grave },
	{ KEY_LEFTSHIFT, 		KS_Shift_L },
	{ KEY_BACKSLASH, 		KS_backslash },
	{ KEY_Z, 				KS_z, 			KS_Z },
	{ KEY_X, 				KS_x, 			KS_X },
	{ KEY_C, 				KS_c, 			KS_C },
	{ KEY_V, 				KS_v, 			KS_V },
	{ KEY_B, 				KS_b, 			KS_B },
	{ KEY_N, 				KS_n, 			KS_N },
	{ KEY_M, 				KS_m, 			KS_M },
	{ KEY_COMMA, 			KS_comma },
	{ KEY_DOT, 				KS_period },
	{ KEY_SLASH, 			KS_slash },
	{ KEY_RIGHTSHIFT, 		KS_Shift_R },
	{ KEY_KPASTERISK, 		KS_KP_Multiply },
	{ KEY_LEFTALT, 			KS_Alt_L },
	{ KEY_SPACE, 			KS_space },
	{ KEY_CAPSLOCK, 		KS_Caps_Lock },
	{ KEY_F1, 				KS_f1 },
	{ KEY_F2, 				KS_f2 },
	{ KEY_F3, 				KS_f3 },
	{ KEY_F4, 				KS_f4 },
	{ KEY_F5, 				KS_f5 },
	{ KEY_F6, 				KS_f6 },
	{ KEY_F7, 				KS_f7 },
	{ KEY_F8, 				KS_f8 },
	{ KEY_F9, 				KS_f9 },
	{ KEY_F10, 				KS_f10 },
	{ KEY_NUMLOCK, 			KS_Num_Lock },
	{ KEY_SCROLLLOCK, 		KS_Hold_Screen },
	{ KEY_KP7, 				KS_KP_7 },
	{ KEY_KP8, 				KS_KP_8 },
	{ KEY_KP9, 				KS_KP_9 },
	{ KEY_KPMINUS, 			KS_KP_Subtract },
	{ KEY_KP4, 				KS_KP_4 },
	{ KEY_KP5, 				KS_KP_5 },
	{ KEY_KP6, 				KS_KP_6 },
	{ KEY_KPPLUS, 			KS_KP_Add },
	{ KEY_KP1, 				KS_KP_1 },
	{ KEY_KP2, 				KS_KP_2 },
	{ KEY_KP3, 				KS_KP_3 },
	{ KEY_KP0, 				KS_KP_0 },
	{ KEY_KPDOT, 			KS_KP_Decimal },
	{ KEY_CAPSLOCK, 		KS_Caps_Lock },
	{ KEY_ZENKAKUHANKAKU, 	KS_Zenkaku_Hankaku },
	{ KEY_102ND, 			},
	{ KEY_F11, 				KS_f11 },
	{ KEY_F12, 				KS_f12 },
	{ KEY_RO, 				},
	{ KEY_KATAKANA, 		},
	{ KEY_HIRAGANA, 		},
	{ KEY_HENKAN, 			KS_Henkan },
	{ KEY_KATAKANAHIRAGANA,	KS_Hiragana_Katakana },
	{ KEY_MUHENKAN, 		KS_Muhenkan },
	{ KEY_KPJPCOMMA, 		},
	{ KEY_KPENTER, 			KS_KP_Enter },
	{ KEY_RIGHTCTRL, 		KS_Control_R },
	{ KEY_KPSLASH, 	 		KS_KP_Divide},
	{ KEY_SYSRQ, 	 		},
	{ KEY_RIGHTALT, 		KS_Alt_R },
	{ KEY_LINEFEED, 		KS_Linefeed },
	{ KEY_HOME, 			KS_Home },
	{ KEY_UP, 				KS_Up },
	{ KEY_PAGEUP, 			},
	{ KEY_LEFT, 			KS_Left },
	{ KEY_RIGHT, 			KS_Right },
	{ KEY_END, 				KS_End },
	{ KEY_DOWN, 			KS_Down },
	{ KEY_PAGEDOWN, 		},
	{ KEY_INSERT, 			KS_Insert },
	{ KEY_MACRO, 			},
	{ KEY_MUTE, 			},
	{ KEY_VOLUMEDOWN, 		},
	{ KEY_VOLUMEUP, 	 	},
	{ KEY_POWER, 		 	KS_Power },
	{ KEY_KPEQUAL, 			KS_KP_Equal },
	{ KEY_KPPLUSMINUS, 		},
	{ KEY_PAUSE, 			KS_Pause },
	{ KEY_SCALE, 		 	},
	{ KEY_KPCOMMA, 		 	},
	{ KEY_HANGEUL, 		 	},
	{ KEY_HANJA, 		 	},
	{ KEY_YEN, 		 		KS_yen },
	{ KEY_LEFTMETA, 		KS_Meta_L },
	{ KEY_RIGHTMETA, 		KS_Meta_R },
	{ KEY_COMPOSE, 		 	},
};

