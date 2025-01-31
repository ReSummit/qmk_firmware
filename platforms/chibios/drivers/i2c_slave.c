/*  Copyright (C) 2019 Elia Ritterbusch
 +
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/* Library made by: g4lvanix
 * GitHub repository: https://github.com/g4lvanix/I2C-slave-lib
 */

// #include <stddef.h>
#include <stdbool.h>

#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>

#if defined(USE_I2C) && defined(SPLIT_COMMON_TRANSACTIONS)
#    include "transactions.h"

static volatile bool is_callback_executor = false;
#endif // defined(USE_I2C) && defined(SPLIT_COMMON_TRANSACTIONS)

volatile uint8_t i2c_slave_reg[I2C_SLAVE_REG_COUNT];

static volatile uint8_t buffer_address;
static volatile bool    slave_has_register_set = false;

void i2c_slave_init(uint8_t address) {
    gpio_init(I2C1_SDA_PIN);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);

    gpio_init(I2C1_SCL_PIN);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SCL_PIN);

    i2c_init(i2c0, I2C1_CLOCK_SPEED);
    // configure I2C0 for slave mode
    i2c_slave_init(i2c0, address, &ISR);
}

void i2c_slave_stop(void) {
    i2c_slave_deinit(i2c0);
}

ISR(i2c_inst_t *i2c, i2c_slave_event_t event) {
    uint8_t ack = 1;

    switch (event) {
        case TW_SR_SLA_ACK:
            // The device is now a slave receiver
            slave_has_register_set = false;
#if defined(USE_I2C) && defined(SPLIT_COMMON_TRANSACTIONS)
            is_callback_executor = false;
#endif // defined(USE_I2C) && defined(SPLIT_COMMON_TRANSACTIONS)
            break;

        case TW_SR_DATA_ACK:
            // This device is a slave receiver and has received data
            // First byte is the location then the bytes will be writen in buffer with auto-increment
            if (!slave_has_register_set) {
                buffer_address = TWDR;

                if (buffer_address >= I2C_SLAVE_REG_COUNT) { // address out of bounds dont ack
                    ack            = 0;
                    buffer_address = 0;
                }
                slave_has_register_set = true; // address has been received now fill in buffer

#if defined(USE_I2C) && defined(SPLIT_COMMON_TRANSACTIONS)
                // Work out if we're attempting to execute a callback
                is_callback_executor = buffer_address == split_transaction_table[I2C_EXECUTE_CALLBACK].initiator2target_offset;
#endif // defined(USE_I2C) && defined(SPLIT_COMMON_TRANSACTIONS)
            } else {
                i2c_slave_reg[buffer_address] = TWDR;
                buffer_address++;

#if defined(USE_I2C) && defined(SPLIT_COMMON_TRANSACTIONS)
                // If we're intending to execute a transaction callback, do so, as we've just received the transaction ID
                if (is_callback_executor) {
                    split_transaction_desc_t *trans = &split_transaction_table[split_shmem->transaction_id];
                    if (trans->slave_callback) {
                        trans->slave_callback(trans->initiator2target_buffer_size, split_trans_initiator2target_buffer(trans), trans->target2initiator_buffer_size, split_trans_target2initiator_buffer(trans));
                    }
                }
#endif // defined(USE_I2C) && defined(SPLIT_COMMON_TRANSACTIONS)
            }
            break;

        case TW_ST_SLA_ACK:
        case TW_ST_DATA_ACK:
            // This device is a slave transmitter and master has requested data
            TWDR = i2c_slave_reg[buffer_address];
            buffer_address++;
            break;

        case TW_BUS_ERROR:
            // We got an error, reset i2c
            TWCR = 0;
        default:
            break;
    }
}
