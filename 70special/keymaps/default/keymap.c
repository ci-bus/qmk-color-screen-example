#include QMK_KEYBOARD_H
#include "qp.h"

#include "./images/car.qgf.c"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {[0] = LAYOUT_65_all(KC_A), [1] = LAYOUT_65_all(KC_B)};

static painter_device_t tft;
static painter_image_handle_t car_image;

void keyboard_post_init_user(void) {
    tft = qp_st7735_make_spi_device(128, 128, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN, 4, 0);
    qp_init(tft, QP_ROTATION_0);

    qp_set_viewport_offsets(tft, 2, 1);

    qp_rect(tft, 0, 0, 127, 127, 0, 0, 0, true);
    qp_flush(tft);

    car_image = qp_load_image_mem(gfx_car);
    if (car_image != NULL) {
        qp_drawimage(tft, 0, 0, car_image);
    }
}
