#include <driver/i2c_software.h>
#include <utility/delay.h>

#define HIGH 0x01
#define LOW 0x00

/**
 * @brief  发送I2C起始信号
 * @param i2c_software_interface I2C读写接口
 */
static __forceinline void I2C_Software_start(I2C_Software_Interface *i2c_software_interface) {
    i2c_software_interface->writeSDA(HIGH);
    i2c_software_interface->writeSCL(HIGH);
    i2c_software_interface->writeSDA(LOW);
    i2c_software_interface->writeSCL(LOW);
}

/**
 * @brief  发送I2C停止信号
 * @param i2c_software_interface I2C读写接口
 */
static __forceinline void I2C_Software_stop(I2C_Software_Interface *i2c_software_interface) {
    i2c_software_interface->writeSDA(LOW);
    i2c_software_interface->writeSCL(HIGH);
    i2c_software_interface->writeSDA(HIGH);
}

/**
 * @brief  等待从设备应答
 * @param i2c_software_interface I2C读写接口
 * @param wait_us 等待时间
 * @return true: 收到应答; false: 未收到应答
 */
static __forceinline bool I2C_Software_waitAck(I2C_Software_Interface *i2c_software_interface,
                                               uint32_t wait_us) {
    i2c_software_interface->writeSDA(HIGH);

    i2c_software_interface->writeSCL(HIGH);

    while (i2c_software_interface->readSDA()) {
        uint32_t timeout = 0;

        timeout++;
        if (timeout > wait_us) {
            I2C_Software_stop(i2c_software_interface);
            return false;
        }
        Delay_us(1);
    }

    i2c_software_interface->writeSCL(LOW);
    return true;
}

/**
 * @brief  发送应答信号
 * @param i2c_software_interface I2C读写接口
 */
static __forceinline void I2C_Software_sendAck(I2C_Software_Interface *i2c_software_interface) {
    i2c_software_interface->writeSDA(LOW);
    i2c_software_interface->writeSCL(HIGH);
    i2c_software_interface->writeSCL(LOW);
}

/**
 * @brief  发送非应答信号
 * @param i2c_software_interface I2C读写接口
 */
static __forceinline void I2C_Software_sendNAck(I2C_Software_Interface *i2c_software_interface) {
    i2c_software_interface->writeSDA(HIGH);
    i2c_software_interface->writeSCL(HIGH);
    i2c_software_interface->writeSCL(LOW);
}

/**
 * @brief  发送一个字节数据
 * @param i2c_software_interface I2C读写接口
 * @param  byte: 要发送的字节
 */
static __forceinline void I2C_Software_sendByte(I2C_Software_Interface *i2c_software_interface,
                                                uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++, byte <<= 1) {
        if (byte & 0x80) {
            i2c_software_interface->writeSDA(HIGH);
        } else {
            i2c_software_interface->writeSDA(LOW);
        }

        i2c_software_interface->writeSCL(HIGH);
        i2c_software_interface->writeSCL(LOW);
    }
}

/**
 * @brief 读取一个字节数据
 * @param i2c_software_interface I2C读写接口
 * @return 读取到的字节数据
 */
static __forceinline uint8_t I2C_Software_readByte(I2C_Software_Interface *i2c_software_interface) {
    uint8_t byte = 0;

    i2c_software_interface->writeSDA(HIGH);

    for (uint8_t i = 0; i < 8; ++i) {
        byte <<= 1;

        i2c_software_interface->writeSCL(HIGH);

        byte |= i2c_software_interface->readSDA();

        i2c_software_interface->writeSCL(LOW);
    }

    return byte;
}

/* 写数据 */
bool I2C_Software_write(I2C_Software_Interface *i2c_software_interface,
                        uint8_t dev_addr,
                        bool has_reg,
                        uint8_t reg_addr,
                        uint8_t *data,
                        uint16_t len,
                        uint32_t wait_ack_us) {

    I2C_Software_start(i2c_software_interface);

    // 发送设备地址(写模式)
    I2C_Software_sendByte(i2c_software_interface, (dev_addr << 1));
    if (!I2C_Software_waitAck(i2c_software_interface, wait_ack_us)) {
        I2C_Software_stop(i2c_software_interface);
        return false;
    }

    // 发送寄存器地址
    if (has_reg) {
        I2C_Software_sendByte(i2c_software_interface, reg_addr);
        if (!I2C_Software_waitAck(i2c_software_interface, wait_ack_us)) {
            I2C_Software_stop(i2c_software_interface);
            return false;
        }
    }

    // 发送数据
    for (uint16_t i = 0; i < len; i++) {
        I2C_Software_sendByte(i2c_software_interface, data[i]);
        if (!I2C_Software_waitAck(i2c_software_interface, wait_ack_us)) {
            I2C_Software_stop(i2c_software_interface);
            return false;
        }
    }

    I2C_Software_stop(i2c_software_interface);
    return true;
}

/* 读数据 */
bool I2C_Software_read(I2C_Software_Interface *i2c_software_interface,
                       uint8_t dev_addr,
                       bool has_reg,
                       uint8_t reg_addr,
                       uint8_t *data,
                       uint16_t len,
                       uint32_t wait_ack_us) {

    I2C_Software_start(i2c_software_interface);

    // 发送设备地址(写模式)
    I2C_Software_sendByte(i2c_software_interface, (dev_addr << 1));
    if (!I2C_Software_waitAck(i2c_software_interface, wait_ack_us)) {
        I2C_Software_stop(i2c_software_interface);
        return false;
    }

    // 发送寄存器地址
    if (has_reg) {
        I2C_Software_sendByte(i2c_software_interface, reg_addr);
        if (!I2C_Software_waitAck(i2c_software_interface, wait_ack_us)) {
            I2C_Software_stop(i2c_software_interface);
            return false;
        }
    }

    // 重新开始
    I2C_Software_start(i2c_software_interface);

    // 发送设备地址(读模式)
    I2C_Software_sendByte(i2c_software_interface, (dev_addr << 1) | 0x01);
    if (!I2C_Software_waitAck(i2c_software_interface, wait_ack_us)) {
        I2C_Software_stop(i2c_software_interface);
        return false;
    }

    // 读取数据
    for (uint16_t i = 0; i < len; i++) {
        data[i] = I2C_Software_readByte(i2c_software_interface);

        // 最后一个字节发送NACK，其他发送ACK
        if (i == (len - 1)) {
            I2C_Software_sendNAck(i2c_software_interface);
        } else {
            I2C_Software_sendAck(i2c_software_interface);
        }
    }

    I2C_Software_stop(i2c_software_interface);
    return true;
}
