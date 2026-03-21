#ifndef I2C_H
#define I2C_H

#include <avr/io.h>
#include <stdint.h>

/* ================== INIT ================== */
void i2c_init()
{
    TWSR = 0x00;      // prescaler = 1
    TWBR = 72;        // 100kHz @ 16MHz
    TWCR = (1<<TWEN); // enable TWI
}

/* ================== START ================== */
void i2c_start()
{
    TWCR = (1<<TWSTA) | (1<<TWINT) | (1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
}

/* ================== STOP ================== */
void i2c_stop()
{
    TWCR = (1<<TWSTO) | (1<<TWINT) | (1<<TWEN);
}

/* ================== WRITE ================== */
void i2c_write(uint8_t data)
{
    TWDR = data;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
}

#endif