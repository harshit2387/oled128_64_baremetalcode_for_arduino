# === Project Settings ===
MCU     = atmega328p
F_CPU   = 16000000UL
TARGET  = main

# === Toolchain ===
CC      = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

# === Flags ===
CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -std=c99

# === Programmer Settings ===
PROGRAMMER = arduino
PORT       = COM5
BAUDRATE   = 115200

# === Build ===
all: $(TARGET).hex

$(TARGET).elf: main.c
	$(CC) $(CFLAGS) $< -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

flash: $(TARGET).hex
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -b $(BAUDRATE) -U flash:w:$<

clean:
	del *.o *.elf *.hex