#ifndef I2C_H
#define I2C_H

#include <avr/io.h>
#include <stdint.h>

// --- Initialize I2C (TWI) hardware ---
static inline void I2C_init(void) {
    // Prescaler = 1
    TWSR = 0x00;
    // Bit rate register: SCL freq = F_CPU / (16 + 2*TWBR*Prescaler)
    // For F_CPU = 16MHz and ~100kHz SCL:
    TWBR = 72;
    // Enable TWI
    TWCR = (1 << TWEN);
}

// --- Send START condition ---
static inline void I2C_start(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT))); // Wait until START transmitted
}

// --- Send STOP condition ---
static inline void I2C_stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

// --- Write one byte to I2C bus ---
static inline void I2C_write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT))); // Wait until transmission complete
}

// --- Optional: Read one byte with ACK ---
static inline uint8_t I2C_read_ack(void) {
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

// --- Optional: Read one byte with NACK ---
static inline uint8_t I2C_read_nack(void) {
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

#endif // I2C_H