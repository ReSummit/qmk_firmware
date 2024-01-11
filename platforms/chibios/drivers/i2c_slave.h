/* Library made by: g4lvanix
 * GitHub repository: https://github.com/g4lvanix/I2C-slave-lib

 Info: Inititate the library by giving the required address.
       Read or write to the necessary buffer according to the opperation.
 */

#pragma once

#include "lib/pico_i2c_slave/i2c_slave/include/i2c_slave.h"
#include <hardware/irq.h>

#ifndef I2C_SLAVE_REG_COUNT

#    if defined(USE_I2C) && defined(SPLIT_COMMON_TRANSACTIONS)
#        include "transport.h"
#        define I2C_SLAVE_REG_COUNT sizeof(split_shared_memory_t)
#    else // defined(USE_I2C) && defined(SPLIT_COMMON_TRANSACTIONS)
#        define I2C_SLAVE_REG_COUNT 30
#    endif // defined(USE_I2C) && defined(SPLIT_COMMON_TRANSACTIONS)

#endif // I2C_SLAVE_REG_COUNT

_Static_assert(I2C_SLAVE_REG_COUNT < 256, "I2C target registers must be single byte");

extern volatile uint8_t i2c_slave_reg[I2C_SLAVE_REG_COUNT];

void i2c_slave_init(uint8_t address);
void i2c_slave_stop(void);
