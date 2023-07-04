#include QMK_KEYBOARD_H
#include "qp.h"
#include "wait.h"
#include "quantum.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "./images/tetris.qgf.c"
#include "./images/boxes.565.c"
#include "./elements/tetrominos.c"
#include "./fonts/freeroad_11.qff.c"

typedef struct {
    bool ok;
    int  offsetX;
} CheckRotationResults;

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {[0] = LAYOUT_65_all(KC_A, KC_B, KC_C, KC_D), [1] = LAYOUT_65_all(KC_A, KC_B, KC_C, KC_D)};

#define IMAGE_WIDTH 128
#define IMAGE_HEIGHT 128
#define BOX_SIZE 6
#define H_BOXES 10
#define V_BOXES 21

static painter_device_t      display;
static uint16_t              pixelBuffer[IMAGE_WIDTH * IMAGE_HEIGHT];
static painter_font_handle_t font;
static bool                  playing;
static uint16_t              timeNextMove = 300;
static uint32_t              waitTimeStart;
static uint32_t              waitTimeEnd;
static int                   points = 1;

/*
Playing tetromino
0 Type
1 Box (color)
2 Position x
3 Position y
*/
static int       playerTetromino[4] = {0, 0, 0, 0};
static Tetromino playerTetrominoPiece;
static int       nextTetromino[4] = {0, 0, 0, 0};
static Tetromino nextTetrominoPiece;
static int       markDownTetromino;

/*
Tetrominos statuses
0 Used
1 Box (color)
*/
static uint8_t playerBoxes[H_BOXES][V_BOXES][2];

// Control key pressed
static bool pressed_keys[8] = {
    0, // Player left
    0, // Player right
    0, // Player rotate
    0, // Player down
    0, // Player 2 left
    0, // Player 2 right
    0, // Player 2 rotate
    0  // Player 2 down
};

void reset_time(uint16_t time) {
    waitTimeStart = timer_read();
    waitTimeEnd   = (waitTimeStart + time) % UINT16_MAX;
}

bool check_time(void) {
    uint16_t time = timer_read();
    if (waitTimeEnd > waitTimeStart) {
        return (time > waitTimeEnd || time < waitTimeStart);
    } else {
        return (time > waitTimeEnd && time < waitTimeStart);
    }
}

void clear_screen(uint8_t width) {
    for (uint8_t y = 0; y < IMAGE_HEIGHT; y++) {
        for (uint8_t x = 0; x < width; x++) {
            pixelBuffer[y * IMAGE_WIDTH + x] = 0x0000;
        }
    }
}

void clear_boxes(void) {
    for (uint8_t y = 0; y < V_BOXES; y++) {
        for (uint8_t x = 0; x < H_BOXES; x++) {
            playerBoxes[x][y][0] = 0;
            playerBoxes[x][y][1] = 0;
        }
    }
}

uint16_t generateRandomColor(void) {
    uint8_t  red   = rand() % 32;
    uint8_t  green = rand() % 64;
    uint8_t  blue  = rand() % 32;
    uint16_t color = (red << 11) | (green << 5) | blue;
    return color;
}

void draw_background(void) {
    for (uint8_t y = 0; y < IMAGE_HEIGHT; y++) {
        for (uint8_t x = H_BOXES * BOX_SIZE + 1; x < 128; x++) {
            pixelBuffer[y * IMAGE_WIDTH + x] = generateRandomColor();
        }
    }
    for (uint8_t y = 20; y < 54; y++) {
        for (uint8_t x = 77; x < 111; x++) {
            if (rand() % 2) {
                pixelBuffer[y * IMAGE_WIDTH + x] = 0x8410;
            } else {
                pixelBuffer[y * IMAGE_WIDTH + x] = 0x0000;
            }
        }
    }
}

void rotate_array(bool *array, int size, bool right) {
    for (int i = 0; i < size / 2; i++) {
        for (int j = i; j < size - i - 1; j++) {
            if (right) {
                uint8_t temp                                      = *(array + i * size + j);
                *(array + i * size + j)                           = *(array + (size - 1 - j) * size + i);
                *(array + (size - 1 - j) * size + i)              = *(array + (size - 1 - i) * size + (size - 1 - j));
                *(array + (size - 1 - i) * size + (size - 1 - j)) = *(array + j * size + (size - 1 - i));
                *(array + j * size + (size - 1 - i))              = temp;
            } else {
                uint8_t temp                                      = *(array + i * size + j);
                *(array + i * size + j)                           = *(array + j * size + (size - 1 - i));
                *(array + j * size + (size - 1 - i))              = *(array + (size - 1 - i) * size + (size - 1 - j));
                *(array + (size - 1 - i) * size + (size - 1 - j)) = *(array + (size - 1 - j) * size + i);
                *(array + (size - 1 - j) * size + i)              = temp;
            }
        }
    }
}

void embed_part(const uint16_t *part, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    for (uint8_t y2 = 0; y2 < h; y2++) {
        for (uint8_t x2 = 0; x2 < w; x2++) {
            pixelBuffer[(y2 + y) * IMAGE_WIDTH + (x2 + x)] = part[y2 * w + x2];
        }
    }
}

void add_next_tetromino(void) {
    // Random values
    uint8_t colorRandom     = rand() % 6;
    uint8_t tetrominoRandom = rand() % 7;
    uint8_t rotateRandom    = rand() % 4;
    uint8_t x               = 14;
    uint8_t y               = 4;
    // Get tetromino
    Tetromino piece         = tetrominos[tetrominoRandom];
    uint8_t   tetrominoSize = sqrt(piece.length);
    // Clone tetromino
    if (nextTetrominoPiece.data) {
        free(nextTetrominoPiece.data);
    }
    uint8_t size  = piece.length * sizeof(bool);
    bool   *clone = malloc(size);
    memcpy(clone, piece.data, size);
    // Rotate tetromino
    if (rotateRandom > 0) {
        for (uint8_t r = 0; r < rotateRandom; r++) {
            rotate_array(clone, tetrominoSize, true);
        }
    }
    // Store values
    nextTetrominoPiece.length = piece.length;    // Tetromino length
    nextTetrominoPiece.data   = clone;           // Tetromino data
    nextTetromino[0]          = tetrominoRandom; // Tipe
    nextTetromino[1]          = colorRandom;     // Color
    nextTetromino[2]          = x;               // x
    nextTetromino[3]          = y;               // y
}

void add_player_tetromino(void) {
    // Get next tetromino
    Tetromino piece         = nextTetrominoPiece;
    uint8_t   tetrominoSize = sqrt(piece.length);
    // Clone tetromino
    if (playerTetrominoPiece.data) {
        free(playerTetrominoPiece.data);
    }
    uint8_t size  = piece.length * sizeof(bool);
    bool   *clone = malloc(size);
    memcpy(clone, piece.data, size);
    // Store values
    playerTetrominoPiece.length = piece.length;
    playerTetrominoPiece.data   = clone;
    playerTetromino[0]          = nextTetromino[0];       // Tipe
    playerTetromino[1]          = nextTetromino[1];       // Color
    playerTetromino[2]          = 3;                      // x
    playerTetromino[3]          = tetrominoSize * -1 - 1; // y
}

bool check_movement(uint8_t tetrominoSize, bool *data, int offsetX, int offsetY) {
    for (uint8_t y = 0; y < tetrominoSize; y++) {
        for (uint8_t x = 0; x < tetrominoSize; x++) {
            if (data[y * tetrominoSize + x] == 1) {
                int xTemp = playerTetromino[2] + x + offsetX;
                int yTemp = playerTetromino[3] + y + offsetY;
                if (xTemp < 0 || xTemp >= H_BOXES || yTemp >= V_BOXES) { // Limits
                    return false;
                } else if (yTemp >= 0 && playerBoxes[xTemp][yTemp][0] == 1) { // Hit box
                    return false;
                }
            }
        }
    }
    return true;
}

CheckRotationResults check_rotation(uint8_t tetrominoSize, bool *data) {
    CheckRotationResults results;
    results.ok      = true;
    results.offsetX = 0;
    // Check left and right limits and put offsets
    for (uint8_t y = 0; y < tetrominoSize; y++) {
        for (uint8_t x = 0; x < tetrominoSize; x++) {
            if (data[y * tetrominoSize + x] == 1) {
                int xTemp = playerTetromino[2] + x;
                while (xTemp < 0) {
                    results.offsetX++;
                    xTemp++;
                }
                while (xTemp >= H_BOXES) {
                    results.offsetX--;
                    xTemp--;
                }
            }
        }
    }
    // Check hits boxes
    for (uint8_t y = 0; y < tetrominoSize; y++) {
        for (uint8_t x = 0; x < tetrominoSize; x++) {
            if (data[y * tetrominoSize + x] == 1) {
                int xTemp = playerTetromino[2] + x + results.offsetX;
                int yTemp = playerTetromino[3] + y;
                if (yTemp >= 0 && playerBoxes[xTemp][yTemp][0] == 1) {
                    results.ok = false;
                }
            }
        }
    }
    return results;
}

void create_static_boxes(uint8_t tetrominoSize, bool *data, uint8_t boxColor) {
    for (uint8_t y = 0; y < tetrominoSize; y++) {
        for (uint8_t x = 0; x < tetrominoSize; x++) {
            if (data[y * tetrominoSize + x]) {
                int xTemp = playerTetromino[2] + x;
                int yTemp = playerTetromino[3] + y;
                if (yTemp >= 0) {
                    playerBoxes[xTemp][yTemp][0] = 1;
                    playerBoxes[xTemp][yTemp][1] = boxColor;
                }
            }
        }
    }
}

void draw_static_boxes(void) {
    for (uint8_t y = 0; y < V_BOXES; y++) {
        for (uint8_t x = 0; x < H_BOXES; x++) {
            if (playerBoxes[x][y][0] == 1) {
                embed_part(boxes[playerBoxes[x][y][1]], x * BOX_SIZE, y * BOX_SIZE, BOX_SIZE, BOX_SIZE);
            }
        }
    }
}

void delete_complete_lines(void) {
    for (uint8_t y = V_BOXES - 1; y > 0; y--) {
        bool delete = true;
        bool paint  = false;
        for (uint8_t x = 0; x < H_BOXES; x++) {
            // Check if box is missing
            if (playerBoxes[x][y][0] == 0) { // Missing box
                delete = false;
            }
            if (playerBoxes[x][y][1] != 6) { // No gray color
                paint = true;
            }
        }
        if (delete) {
            if (paint) {
                // paint the lines
                for (uint8_t x = 0; x < H_BOXES; x++) {
                    // Gray color
                    playerBoxes[x][y][1] = 6;
                    // Sum point
                    points++;
                }
            } else {
                // lower the lines
                for (uint8_t y2 = y; y2 > 0; y2--) {
                    for (uint8_t x = 0; x < H_BOXES; x++) {
                        playerBoxes[x][y2][0] = playerBoxes[x][y2 - 1][0];
                        playerBoxes[x][y2][1] = playerBoxes[x][y2 - 1][1];
                    }
                }
                // Check again the some line
                y++;
            }
        }
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // Run rand to energize the game
    rand();
    // Control pressed keys
    switch (keycode) {
        case KC_A:
            pressed_keys[0] = record->event.pressed;
            break;
        case KC_B:
            pressed_keys[1] = record->event.pressed;
            break;
        case KC_C:
            pressed_keys[2] = record->event.pressed;
            break;
        case KC_D:
            pressed_keys[3] = record->event.pressed;
            break;
    }
    return true;
};

void control_actions(uint8_t tetrominoSize, bool *data) {
    // Player left
    if (pressed_keys[0]) {
        // Check limits and static boxes
        if (check_movement(tetrominoSize, data, -1, 0)) {
            // Move
            playerTetromino[2]--;
        }
        pressed_keys[0] = false;
    }
    // Player right
    if (pressed_keys[1]) {
        // Check limits and static boxes
        if (check_movement(tetrominoSize, data, 1, 0)) {
            // Move
            playerTetromino[2]++;
        }
        pressed_keys[1] = false;
    }
    // Player rotate
    if (pressed_keys[2]) {
        // Clone tetromino
        uint8_t size  = tetrominoSize * tetrominoSize * sizeof(bool);
        bool   *clone = malloc(size);
        memcpy(clone, data, size);
        // Rotate clone
        rotate_array(clone, tetrominoSize, true);
        // Check rotation
        CheckRotationResults checks = check_rotation(tetrominoSize, clone);
        if (checks.ok) {
            // Move tetromino offset
            playerTetromino[2] += checks.offsetX;
            // Rotate
            rotate_array(data, tetrominoSize, true);
        }
        free(clone);
        pressed_keys[2] = false;
    }
}

void matrix_scan_user(void) {
    if (playing) {
        // Clear all
        clear_screen(H_BOXES * BOX_SIZE + 1);

        //---------------//
        // Draw player //
        //---------------//

        uint8_t playerTetrominoSize = sqrt(playerTetrominoPiece.length);

        // Controls
        control_actions(playerTetrominoSize, playerTetrominoPiece.data);

        // Check time next movement
        if (check_time()) {
            // Draw background
            draw_background();
            // Delete complete line
            delete_complete_lines();
            // If it can move
            if (check_movement(playerTetrominoSize, playerTetrominoPiece.data, 0, 1)) {
                // Move tetromino
                playerTetromino[3]++;
                // Reset time next movement
                if (pressed_keys[3]) { // If key down is pressed
                    reset_time(timeNextMove / 10);
                } else {
                    reset_time(timeNextMove);
                }
            } else { // Else finish movement
                // Create static single boxes
                create_static_boxes(playerTetrominoSize, playerTetrominoPiece.data, playerTetromino[1]);
                // GAME OVER
                if (playerTetromino[3] < 0) {
                    clear_screen(IMAGE_WIDTH);
                    qp_pixdata(display, pixelBuffer, (IMAGE_WIDTH * IMAGE_HEIGHT));
                    qp_drawtext(display, 10, 10, font, "GAME OVER");
                    char pointsText[20];
                    snprintf(pointsText, sizeof(pointsText), "Points: %u", points);
                    qp_drawtext(display, 10, 30, font, pointsText);
                    playing = false;
                } else {
                    // Parse next to player tetromino
                    add_player_tetromino();
                    // Add next tetromino
                    add_next_tetromino();
                    // Delete complete line
                    delete_complete_lines();
                    // Reset time next movement
                    reset_time(timeNextMove);
                }
            }
        }

        // draw static boxes
        draw_static_boxes();

        // Draw mark down
        markDownTetromino = 0;
        while (check_movement(playerTetrominoSize, playerTetrominoPiece.data, 0, markDownTetromino + 1)) {
            markDownTetromino++;
        }
        if (markDownTetromino) {
            // Draw mark down
            for (uint8_t y = 0; y < playerTetrominoSize; y++) {
                for (uint8_t x = 0; x < playerTetrominoSize; x++) {
                    if (playerTetrominoPiece.data[y * playerTetrominoSize + x]) {
                        int xTemp = playerTetromino[2] + x;
                        int yTemp = playerTetromino[3] + y + markDownTetromino;
                        if (yTemp >= 0) {
                            embed_part(boxes[7], xTemp * BOX_SIZE, yTemp * BOX_SIZE, BOX_SIZE, BOX_SIZE);
                        }
                    }
                }
            }
        }

        // Draw player tetromino
        for (uint8_t y = 0; y < playerTetrominoSize; y++) {
            for (uint8_t x = 0; x < playerTetrominoSize; x++) {
                if (playerTetrominoPiece.data[y * playerTetrominoSize + x]) {
                    int xTemp = playerTetromino[2] + x;
                    int yTemp = playerTetromino[3] + y;
                    if (yTemp >= 0) {
                        embed_part(boxes[playerTetromino[1]], xTemp * BOX_SIZE, yTemp * BOX_SIZE, BOX_SIZE, BOX_SIZE);
                    }
                }
            }
        }

        // Draw next tetromino
        uint8_t nextTetrominoSize = sqrt(nextTetrominoPiece.length);
        for (uint8_t y = 0; y < nextTetrominoSize; y++) {
            for (uint8_t x = 0; x < nextTetrominoSize; x++) {
                if (nextTetrominoPiece.data[y * nextTetrominoSize + x]) {
                    int xTemp = nextTetromino[2] + x;
                    int yTemp = nextTetromino[3] + y;
                    if (yTemp >= 0) {
                        embed_part(boxes[nextTetromino[1]], xTemp * BOX_SIZE, yTemp * BOX_SIZE, BOX_SIZE, BOX_SIZE);
                    }
                }
            }
        }

        // Show changes on display
        qp_pixdata(display, pixelBuffer, (IMAGE_WIDTH * IMAGE_HEIGHT));
    }
}

void start_game(void) {
    clear_screen(IMAGE_WIDTH);
    clear_boxes();
    add_next_tetromino();
    add_player_tetromino();
    add_next_tetromino();
    reset_time(timeNextMove);
    playing = true;
}

void init_game(void) {
    qp_rect(display, 0, 0, 127, 127, 0, 0, 0, true);
    qp_flush(display);

    painter_image_handle_t tetris_image = qp_load_image_mem(gfx_tetris);
    if (tetris_image != NULL) {
        qp_drawimage(display, 0, 0, tetris_image);
    }
    wait_ms(3000);
    qp_close_image(tetris_image);

    qp_rect(display, 0, 0, 127, 127, 0, 0, 0, true);
    qp_flush(display);

    font = qp_load_font_mem(font_freeroad_11);

    start_game();
}

void keyboard_post_init_user(void) {
    display = qp_st7735_make_spi_device(128, 128, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN, 4, 0);
    qp_init(display, QP_ROTATION_0);
    qp_set_viewport_offsets(display, 2, 1);

    // Init game
    init_game();
}
