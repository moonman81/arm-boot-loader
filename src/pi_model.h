/* Raspberry Pi Model Definitions */

#ifndef PI_MODEL_H
#define PI_MODEL_H

// Raspberry Pi models - comprehensive list
typedef enum {
    PI_MODEL_UNKNOWN = 0,

    // Pi 1 Family (BCM2835)
    PI_MODEL_1A,
    PI_MODEL_1B,
    PI_MODEL_1A_PLUS,
    PI_MODEL_1B_PLUS,

    // Pi 2 Family (BCM2836/BCM2837)
    PI_MODEL_2B,

    // Pi Zero Family (BCM2835)
    PI_MODEL_ZERO,
    PI_MODEL_ZERO_W,
    PI_MODEL_ZERO_2_W,

    // Pi 3 Family (BCM2837)
    PI_MODEL_3B,
    PI_MODEL_3B_PLUS,
    PI_MODEL_3A_PLUS,

    // Pi 4 Family (BCM2711)
    PI_MODEL_4B,

    // Pi 5 Family (BCM2712)
    PI_MODEL_5B,

    // Special models
    PI_MODEL_400,     // BCM2711

    PI_MODEL_MAX      // Keep last for bounds checking
} pi_model_t;

#endif