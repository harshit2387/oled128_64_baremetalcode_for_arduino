#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

// ===== PIN CONFIG (CORRECTED) =====
#define TFT_CS   PB2   // Arduino 10
#define TFT_RST  PB1   // Arduino 9
#define TFT_DC   PB0   // Arduino 8

// ===== MACROS =====
#define CS_LOW()   PORTB &= ~(1 << TFT_CS)
#define CS_HIGH()  PORTB |=  (1 << TFT_CS)

#define DC_LOW()   PORTB &= ~(1 << TFT_DC)
#define DC_HIGH()  PORTB |=  (1 << TFT_DC)

#define RST_LOW()  PORTB &= ~(1 << TFT_RST)
#define RST_HIGH() PORTB |=  (1 << TFT_RST)

// ===== SPI INIT =====
void SPI_init() {
    DDRB |= (1 << PB3) | (1 << PB5) | (1 << TFT_CS) | (1 << TFT_DC) | (1 << TFT_RST);

    // SPI Master, safe slow speed (fosc/128)
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
}

// ===== SPI WRITE =====
void SPI_write(uint8_t data) {
    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
}

// ===== LOW LEVEL =====
void cmd(uint8_t c) {
    DC_LOW();
    CS_LOW();
    SPI_write(c);
    CS_HIGH();
}

void data(uint8_t d) {
    DC_HIGH();
    CS_LOW();
    SPI_write(d);
    CS_HIGH();
}

void data16(uint16_t d) {
    DC_HIGH();
    CS_LOW();
    SPI_write(d >> 8);
    SPI_write(d & 0xFF);
    CS_HIGH();
}

// ===== TFT INIT (MINIMAL + STABLE) =====
void tft_init() {
    RST_LOW(); _delay_ms(100);
    RST_HIGH(); _delay_ms(100);

    cmd(0x01); _delay_ms(150); // SWRESET
    cmd(0x11); _delay_ms(150); // SLPOUT

    cmd(0x3A); data(0x05);     // 16-bit color
    cmd(0x36); data(0x00);     // rotation

    cmd(0x29); _delay_ms(100); // DISPON
}

//draw pixel 
void drawPixel(uint8_t x, uint8_t y, uint16_t color) {
    // Set column (X)
    cmd(0x2A);
    data(0x00); data(x);
    data(0x00); data(x);

    // Set row (Y)
    cmd(0x2B);
    data(0x00); data(y);
    data(0x00); data(y);

    // Write pixel
    cmd(0x2C);
    data16(color);
}
// ===== SET FULL SCREEN WINDOW =====
void setWindow() {
    cmd(0x2A); // CASET
    data(0x00); data(0x00);
    data(0x00); data(127);

    cmd(0x2B); // RASET
    data(0x00); data(0x00);
    data(0x00); data(127);

    cmd(0x2C); // RAMWR
}

// ===== FILL SCREEN =====
void fill(uint16_t color) {
    setWindow();

    for (uint32_t i = 0; i < 128UL * 128UL; i++) {
        data16(color);
    }
}

// ===== MAIN =====
int main(void) {
    SPI_init();

    // Set pins HIGH initially
    PORTB |= (1 << TFT_CS) | (1 << TFT_DC) | (1 << TFT_RST);

    tft_init();

    // TEST LOOP (VISIBLE OUTPUT)
    while (1) {
    fill(0x0000); // clear screen (black)

    drawPixel(10, 10, 0xF800);  // red
    drawPixel(20, 20, 0x07E0);  // green
    drawPixel(30, 30, 0x001F);  // blue

    _delay_ms(2000);
}
}