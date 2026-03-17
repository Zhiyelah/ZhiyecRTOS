#include <driver/i2c_software.h>
#include <utility/delay.h>

#define HIGH_LEVEL 0x01
#define LOW_LEVEL 0x00

/**
 * @brief  发送I2C起始信号
 * @param i2c_software_interface I2C读写接口
 */
static inline void i2c_software_start(i2c_software_interface *i2c_software_interface) {
    i2c_software_interface->write_sda(HIGH_LEVEL);
    i2c_software_interface->write_scl(HIGH_LEVEL);
    i2c_software_interface->write_sda(LOW_LEVEL);
    i2c_software_interface->write_scl(LOW_LEVEL);
}

/**
 * @brief  发送I2C停止信号
 * @param i2c_software_interface I2C读写接口
 */
static inline void i2c_software_stop(i2c_software_interface *i2c_software_interface) {
    i2c_software_interface->write_sda(LOW_LEVEL);
    i2c_software_interface->write_scl(HIGH_LEVEL);
    i2c_software_interface->write_sda(HIGH_LEVEL);
}

/**
 * @brief  等待从设备应答
 * @param i2c_software_interface I2C读写接口
 * @param wait_us 等待时间
 * @return true: 收到应答; false: 未收到应答
 */
static inline bool i2c_software_wait_ack(i2c_software_interface *i2c_software_interface,
                                         uint32_t wait_us) {
    i2c_software_interface->write_sda(HIGH_LEVEL);

    i2c_software_interface->write_scl(HIGH_LEVEL);

    while (i2c_software_interface->read_sda()) {
        uint32_t timeout = 0;

        timeout++;
        if (timeout > wait_us) {
            i2c_software_stop(i2c_software_interface);
            return false;
        }
        delay_us(1);
    }

    i2c_software_interface->write_scl(LOW_LEVEL);
    return true;
}

/**
 * @brief  发送应答信号
 * @param i2c_software_interface I2C读写接口
 */
static inline void i2c_software_send_ack(i2c_software_interface *i2c_software_interface) {
    i2c_software_interface->write_sda(LOW_LEVEL);
    i2c_software_interface->write_scl(HIGH_LEVEL);
    i2c_software_interface->write_scl(LOW_LEVEL);
}

/**
 * @brief  发送非应答信号
 * @param i2c_software_interface I2C读写接口
 */
static inline void i2c_software_send_no_ack(i2c_software_interface *i2c_software_interface) {
    i2c_software_interface->write_sda(HIGH_LEVEL);
    i2c_software_interface->write_scl(HIGH_LEVEL);
    i2c_software_interface->write_scl(LOW_LEVEL);
}

/**
 * @brief  发送一个字节数据
 * @param i2c_software_interface I2C读写接口
 * @param  byte: 要发送的字节
 */
static inline void i2c_software_send_byte(i2c_software_interface *i2c_software_interface,
                                          uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++, byte <<= 1) {
        if (byte & 0x80) {
            i2c_software_interface->write_sda(HIGH_LEVEL);
        } else {
            i2c_software_interface->write_sda(LOW_LEVEL);
        }

        i2c_software_interface->write_scl(HIGH_LEVEL);
        i2c_software_interface->write_scl(LOW_LEVEL);
    }
}

/**
 * @brief 读取一个字节数据
 * @param i2c_software_interface I2C读写接口
 * @return 读取到的字节数据
 */
static inline uint8_t i2c_software_read_byte(i2c_software_interface *i2c_software_interface) {
    uint8_t byte = 0;

    i2c_software_interface->write_sda(HIGH_LEVEL);

    for (uint8_t i = 0; i < 8; ++i) {
        byte <<= 1;

        i2c_software_interface->write_scl(HIGH_LEVEL);

        byte |= i2c_software_interface->read_sda();

        i2c_software_interface->write_scl(LOW_LEVEL);
    }

    return byte;
}

/* 写数据 */
bool i2c_software_write(i2c_software_interface *i2c_software_interface,
                        uint8_t dev_addr,
                        bool has_reg,
                        uint8_t reg_addr,
                        uint8_t *data,
                        uint16_t len,
                        uint32_t wait_ack_us) {

    i2c_software_start(i2c_software_interface);

    // 发送设备地址(写模式)
    i2c_software_send_byte(i2c_software_interface, (dev_addr << 1));
    if (!i2c_software_wait_ack(i2c_software_interface, wait_ack_us)) {
        i2c_software_stop(i2c_software_interface);
        return false;
    }

    // 发送寄存器地址
    if (has_reg) {
        i2c_software_send_byte(i2c_software_interface, reg_addr);
        if (!i2c_software_wait_ack(i2c_software_interface, wait_ack_us)) {
            i2c_software_stop(i2c_software_interface);
            return false;
        }
    }

    // 发送数据
    for (uint16_t i = 0; i < len; i++) {
        i2c_software_send_byte(i2c_software_interface, data[i]);
        if (!i2c_software_wait_ack(i2c_software_interface, wait_ack_us)) {
            i2c_software_stop(i2c_software_interface);
            return false;
        }
    }

    i2c_software_stop(i2c_software_interface);
    return true;
}

/* 读数据 */
bool i2c_software_read(i2c_software_interface *i2c_software_interface,
                       uint8_t dev_addr,
                       bool has_reg,
                       uint8_t reg_addr,
                       uint8_t *data,
                       uint16_t len,
                       uint32_t wait_ack_us) {

    i2c_software_start(i2c_software_interface);

    // 发送设备地址(写模式)
    i2c_software_send_byte(i2c_software_interface, (dev_addr << 1));
    if (!i2c_software_wait_ack(i2c_software_interface, wait_ack_us)) {
        i2c_software_stop(i2c_software_interface);
        return false;
    }

    // 发送寄存器地址
    if (has_reg) {
        i2c_software_send_byte(i2c_software_interface, reg_addr);
        if (!i2c_software_wait_ack(i2c_software_interface, wait_ack_us)) {
            i2c_software_stop(i2c_software_interface);
            return false;
        }
    }

    // 重新开始
    i2c_software_start(i2c_software_interface);

    // 发送设备地址(读模式)
    i2c_software_send_byte(i2c_software_interface, (dev_addr << 1) | 0x01);
    if (!i2c_software_wait_ack(i2c_software_interface, wait_ack_us)) {
        i2c_software_stop(i2c_software_interface);
        return false;
    }

    // 读取数据
    for (uint16_t i = 0; i < len; i++) {
        data[i] = i2c_software_read_byte(i2c_software_interface);

        // 最后一个字节发送NACK，其他发送ACK
        if (i == (len - 1)) {
            i2c_software_send_no_ack(i2c_software_interface);
        } else {
            i2c_software_send_ack(i2c_software_interface);
        }
    }

    i2c_software_stop(i2c_software_interface);
    return true;
}
