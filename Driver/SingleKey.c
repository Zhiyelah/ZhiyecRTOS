#include "SingleKey.h"

void Key_pressedEvent(struct SingleKey *single_key) {

    if (!single_key->is_pressed) {
        return;
    }

    switch (single_key->key_status) {
    case KeyStatus_Idle:
        single_key->key_count = 0;
        if (single_key->is_pressed()) {
            single_key->key_status = KeyStatus_Delay;
        }
        break;

    case KeyStatus_Delay:
        single_key->key_count++;
        if (single_key->key_count < 20) {
            break;
        }

        if (single_key->is_pressed()) {
            single_key->key_status = KeyStatus_Pressed;
        } else {
            single_key->key_status = KeyStatus_Idle;
        }
        break;

    case KeyStatus_Pressed:
        if (single_key->is_pressed()) {
            single_key->key_count++;
            if (single_key->key_count >= 700) {
                single_key->key_status = KeyStatus_LongPressed;
                if (single_key->on_long_pressed) {
                    single_key->on_long_pressed();
                }
                break;
            }
        } else {
            if (single_key->on_pressed) {
                single_key->on_pressed();
            }
            single_key->key_status = KeyStatus_Idle;
        }
        break;

    case KeyStatus_LongPressed:
        if (!single_key->is_pressed()) {
            single_key->key_status = KeyStatus_Idle;
        }
        break;

    default:
        break;
    }
}
