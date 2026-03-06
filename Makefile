# === Project Settings ===
MCU     = atmega328p
F_CPU   = 16000000UL
BAUD    = 9600
TARGET  = main

# === Toolchain ===
CC      = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

# === Flags ===
CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -std=c99
LDFLAGS = -mmcu=$(MCU)

# === Sources ===
SRC     = main.c OLED.c I2C.c
OBJ     = $(SRC:.c=.o)

# === Programmer Settings (adjust for your setup) ===
PROGRAMMER = arduino
PORT       = COM5        # Change to your serial port (e.g. /dev/ttyUSB0 on Linux)
BAUDRATE   = 115200

# === Rules ===
all: $(TARGET).hex

$(TARGET).o: $(SRC) oled.h i2c.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

flash: $(TARGET).hex
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -b $(BAUDRATE) -U flash:w:$<

clean:
	rm -f *.o *.elf *.hex