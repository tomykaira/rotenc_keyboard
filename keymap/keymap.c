#include QMK_KEYBOARD_H

#include "pincontrol.h"
#include "timer.h"

extern keymap_config_t keymap_config;

// Each layer gets a name for readability, which is then used in the keymap matrix below.
// The underscores don't mean anything - you can have a layer called STUFF or any other name.
// Layer names don't all need to be of the same length, obviously, and you can also skip them
// entirely and just use numbers.
#define _QWERTY 0
#define _SYMBOL 1
#define _NUM 2
#define _FUNC 3

#define ROTENC_A D1
#define ROTENC_B C6

// Fillers to make layering more clear
#define _______ KC_TRNS
#define XXXXXXX KC_NO

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
[_QWERTY] = LAYOUT_ortho_4x12(
   KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    _______, _______, KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,   \
   KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    _______, _______, KC_H,    KC_J,    KC_K,    KC_L,    KC_LSFT, \
   KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    _______, _______, KC_N,    KC_M,    KC_LCTL, KC_LALT, KC_LGUI, \
   _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______ \
),

 [_SYMBOL] = LAYOUT_ortho_4x12( \
  KC_EXCLAIM, KC_DQUO, KC_HASH, KC_DLR,  KC_PERC, _______, _______, KC_AMPR, KC_QUOTE,KC_LPRN, KC_RPRN, KC_MINS, \
  KC_QUES,    KC_ASTR, KC_GRV,  KC_PIPE, KC_BSLS, _______, _______, KC_LCBR, KC_RCBR, KC_LBRC, KC_RBRC, _______,  \
  KC_PLUS,    KC_TILD, KC_AT,   KC_CIRC, KC_SLSH, _______, _______, KC_UNDS, KC_EQL,  _______, _______, _______, \
  _______,    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______ \
),

[_NUM] = LAYOUT_ortho_4x12( \
  KC_7,    KC_8,    KC_9,    KC_ASTR,  KC_F5,  _______, _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,  \
  KC_4,    KC_5,    KC_6,    KC_PLUS,  KC_5,   _______, _______, KC_F6,   KC_F7,    KC_5,    KC_6,    _______,  \
  KC_1,    KC_2,    KC_3,    KC_0,     KC_DOT, _______, _______, KC_0,    KC_1,    _______, _______, _______, \
  _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______ \
),

[_FUNC] = LAYOUT_ortho_4x12( \
  KC_ESC,   _______, _______, _______, _______, _______, _______, _______, KC_COLON,KC_SCLN, _______, KC_BSPC, \
  KC_TAB ,  KC_UP,   _______, KC_LT,   KC_GT,   _______, _______, _______, KC_COMMA,KC_DOT,  KC_ENT,  _______, \
  KC_LEFT,  KC_DOWN, KC_RIGHT,KC_COMMA,KC_SPC,  _______, _______, _______, KC_PLUS, _______, _______, _______, \
  _______,  _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______ \
),

};

static int16_t last_direction = 0;
static uint8_t dial_layer = 0;
static uint8_t matrix_scan_interval = 0;
static uint16_t last_changed = 0;

void matrix_init_user()
{
  startup_user();
}

void pinModeInputPullUp(uint8_t pin)
{
  uint8_t bv = _BV(pin & 0xf);
  _SFR_IO8((pin >> 4) + 1) &= ~bv;
  _SFR_IO8((pin >> 4) + 2) |= bv;
}

void startup_user()
{
  DIDR0 = 0;
  DIDR1 = 0;

  // JTAG disable for PORT F. write JTD bit twice within four cycles.
  MCUCR |= (1<<JTD);
  MCUCR |= (1<<JTD);

  pinModeInputPullUp(ROTENC_A);
  pinModeInputPullUp(ROTENC_B);
  pinMode(F4, PinDirectionOutput);
  pinMode(B6, PinDirectionOutput);
  pinMode(F5, PinDirectionInput);
  // Enable INT0
  EIMSK |= _BV(INT1);
  // Any logic change
  EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (1 << ISC10);
}

ISR(INT1_vect, ISR_BLOCK) {
  bool a = digitalRead(ROTENC_A);
  bool b = digitalRead(ROTENC_B);
  if ((a && b) || (!a && !b)) {
    last_direction++;
  } else {
    last_direction--;
  }
  if (matrix_scan_interval == 0) {
    matrix_scan_interval = 1;
  }
}

void matrix_scan_user(void)
{
  uint8_t changed = false;

  if (timer_elapsed(last_changed) > 300 && matrix_scan_interval > 64) {
    if (last_direction > 2) {
      dial_layer ++;
      changed = true;
    } else if (last_direction < -2) {
      dial_layer --;
      changed = true;
    }
  }

  if (digitalRead(F5)) {
    dial_layer = 0;
    last_direction = 0;
    changed = true;
  }

  if (changed) {
    last_direction = 0;
    matrix_scan_interval = 0;
    last_changed = timer_read();

    dial_layer = dial_layer & 0x3;
    set_single_persistent_default_layer(dial_layer);

    digitalWrite(F4, (dial_layer & 0x1) ? PinLevelLow : PinLevelHigh);
    digitalWrite(B6, (dial_layer & 0x2) ? PinLevelLow : PinLevelHigh);
  }

  if (matrix_scan_interval > 0) {
    matrix_scan_interval ++;
  }
}
