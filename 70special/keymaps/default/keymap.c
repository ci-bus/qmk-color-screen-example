#include QMK_KEYBOARD_H
#include "qp.h"
#include "wait.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "./images/tetris.qgf.c"
#include "./images/boxes.565.c"
#include "./elements/tetrominos.c"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {[0] = LAYOUT_65_all(KC_A), [1] = LAYOUT_65_all(KC_B)};

#define IMAGE_WIDTH 128
#define IMAGE_HEIGHT 128

static painter_device_t tft;
static uint16_t         pixelBuffer[IMAGE_WIDTH * IMAGE_HEIGHT];
static bool             playing = false;
/*
Playing tetromino
0 Type
1 Box (color)
2 Rotate
3 Position x
4 Position y
*/
uint8_t player1Tetromino[5] = {0, 0, 0, 0, 0};
/*
Tetrominos statuses
0 Box (color)
1 Position x
2 Position y
*/
uint8_t player1Boxes[200][3];

void clear_screen(void) {
    for (uint8_t y = 0; y < IMAGE_HEIGHT; y++) {
        for (uint8_t x = 0; x < IMAGE_WIDTH; x++) {
            pixelBuffer[y * IMAGE_WIDTH + x] = 0x0000;
        }
    }
}

void embed_part(uint16_t *part, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    for (uint8_t y2 = 0; y2 < h; y2++) {
        for (uint8_t x2 = 0; x2 < w; x2++) {
            pixelBuffer[(y2 + y) * IMAGE_WIDTH + (x2 + x)] = part[y2 * w + x2];
        }
    }
}

void housekeeping_task_user(void) {
    if (playing) {
        // for (uint8_t i = 0; i < IMAGE_HEIGHT - 6; i += 6) {
        //     clear_screen();
        /*
        embed_part(cuboRedBuffer, 0, i, 6, 6);
        embed_part(cuboGreenBuffer, 6, i, 6, 6);
        embed_part(cuboBlueBuffer, 12, i, 6, 6);
        embed_part(cuboYellowBuffer, 18, i, 6, 6);
        embed_part(cuboCyanBuffer, 24, i, 6, 6);
        embed_part(cuboPurpleBuffer, 30, i, 6, 6);
        */
        //    qp_pixdata(tft, pixelBuffer, (IMAGE_WIDTH * IMAGE_HEIGHT));
        //    wait_ms(200);
        //}

        clear_screen();

        // Ramdom box color
        uint8_t  boxSize = 6;
        uint8_t  random  = rand() % 6;
        uint16_t box[36];
        for (int i = 0; i < 36; i++) {
            box[i] = boxes[random][i];
        }

        // Random tetromino
        uint8_t   random2 = rand() % 7;
        Tetromino piece   = tetrominos[random2];
        uint8_t   length  = piece.length;
        bool     *data    = piece.data;

        // Draw tetromino
        uint8_t tetrominoSize = sqrt(length);
        for (uint8_t y = 0; y < tetrominoSize; y++) {
            for (uint8_t x = 0; x < tetrominoSize; x++) {
                if (data[y * tetrominoSize + x]) {
                    embed_part(box, x * boxSize, y * boxSize, boxSize, boxSize);
                }
            }
        }

        qp_pixdata(tft, pixelBuffer, (IMAGE_WIDTH * IMAGE_HEIGHT));

        playing = false;
    }
}

void keyboard_post_init_user(void) {
    tft = qp_st7735_make_spi_device(128, 128, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN, 4, 0);
    qp_init(tft, QP_ROTATION_0);

    qp_set_viewport_offsets(tft, 2, 1);

    qp_rect(tft, 0, 0, 127, 127, 0, 0, 0, true);
    qp_flush(tft);

    painter_image_handle_t tetris_image = qp_load_image_mem(gfx_tetris);
    if (tetris_image != NULL) {
        qp_drawimage(tft, 0, 0, tetris_image);
    }
    wait_ms(3000);
    qp_close_image(tetris_image);

    qp_rect(tft, 0, 0, 127, 127, 0, 0, 0, true);
    qp_flush(tft);

    playing = true;
}
