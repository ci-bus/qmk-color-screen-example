#include QMK_KEYBOARD_H
#include "qp.h"

#include "./images/car.qgf.c"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {[0] = LAYOUT_65_all(KC_A), [1] = LAYOUT_65_all(KC_B)};

void keyboard_pre_init_user(void) {
#ifdef __AVR__
    // BIT-C turn off led
    setPinOutput(F0);
    writePinLow(F0);
#endif
}

static painter_device_t tft;
static painter_image_handle_t car_image;

void keyboard_post_init_user(void) {
    tft = qp_st7789_make_spi_device(128, 128, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN, 8, 0);
    qp_init(tft, QP_ROTATION_0);

    car_image = qp_load_image_mem(gfx_car);
    if (car_image != NULL) {
        qp_drawimage(tft, 128, 128, car_image);
    }
}
