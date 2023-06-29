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

static painter_device_t oled;
static painter_image_handle_t car_image;

void keyboard_post_init_user(void) {
    oled = qp_st7735_make_spi_device(128, 128, OLED_CS_PIN, OLED_DC_PIN, OLED_RST_PIN, 8, 0);
    qp_init(oled, QP_ROTATION_0);
    qp_rect(oled, 0, 0, 127, 127, 0, 0, 0, true);
    qp_flush(oled);

    car_image = qp_load_image_mem(gfx_car);
    if (car_image != NULL) {
        qp_drawimage(oled, 128, 128, car_image);
    }
}
