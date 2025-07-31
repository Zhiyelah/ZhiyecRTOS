/**
 * @file I2C_Software.h
 * @author Zhiyelah
 * @brief 软件模拟I2C通信
 */

#ifndef _I2C_Sortware_h
#define _I2C_Sortware_h

#include "Time.h"
#include <stdbool.h>
#include <stdint.h>

/**
 *  引脚初始化
 * 将SCL和SDA引脚设置为开漏输出, 并初始设置为高电平(高阻态)
 */

/**
 * @brief I2C读写接口
 * @note 在接口函数中, 你可能需要:
 *      a. 改变引脚电平
 *      b. 延时10us
 */
typedef struct {
    uint8_t (*readSDA)(void);
    void (*writeSDA)(uint8_t bit);
    void (*writeSCL)(uint8_t bit);
} I2C_Software_Interface;

/**
 * @brief 向I2C设备写入数据
 * @param i2c_software_interface I2C读写接口
 * @param dev_addr: 7位设备地址(函数内已左移一位)
 * @param has_reg 是否有寄存器
 * @param reg_addr: 寄存器地址
 * @param data: 要写入的数据缓冲区
 * @param len: 数据长度
 * @param wait_ack_us 等待应答时间
 * @return true: 写入成功; false: 写入失败
 */
bool I2C_Software_write(I2C_Software_Interface *i2c_software_interface,
                        uint8_t dev_addr,
                        bool has_reg,
                        uint8_t reg_addr,
                        uint8_t *data,
                        uint16_t len,
                        Time_t wait_ack_us);

/**
 * @brief 从I2C设备读取数据
 * @param i2c_software_interface I2C读写接口
 * @param dev_addr: 7位设备地址(函数内已左移一位)
 * @param has_reg 是否有寄存器
 * @param reg_addr: 寄存器地址
 * @param data: 用于存储读取数据的缓冲区(数组)
 * @param len: 要读取的数据长度
 * @param wait_ack_us 等待应答时间
 * @return true: 读取成功; false: 读取失败
 */
bool I2C_Software_read(I2C_Software_Interface *i2c_software_interface,
                       uint8_t dev_addr,
                       bool has_reg,
                       uint8_t reg_addr,
                       uint8_t *data,
                       uint16_t len,
                       Time_t wait_ack_us);

#endif /* _I2C_Sortware_h */
