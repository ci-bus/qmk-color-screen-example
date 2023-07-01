#pragma once

#include <qp.h>

typedef struct {
    uint8_t length;
    bool *data;
} Tetromino;

extern const Tetromino tetrominos[];