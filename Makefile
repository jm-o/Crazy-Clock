#
# To use this, you use 'make init TYPE={the kind of clock you want}'
# That will fuse, flash, seed and set the corrective clock offset in the chip.
#
# BE CAREFUL! Make sure that the fuse: target is set to the correct
# alternative for the crystal installed in your hardware. If you fuse
# the chip wrong, you will BRICK it!

all: calibrate.hex crazy.hex early.hex lazy.hex martian.hex normal.hex rhythm.hex sidereal.hex tidal.hex vetinari.hex warpy.hex wavy.hex whacky.hex tuney.hex

# Change this as appropriate! Don't screw it up!

# AVR binaries path
AVR_PATH = ~/arduino-1.6.5/hardware/tools/avr/

# The clock is a 32.768 kHz crystal.
OPTS = -DF_CPU=32768L

# Change this to pick the correct programmer you're using
PROG = usbtiny

# Change this if you're not using a Tiny45
CHIP = attiny45

# The SPI clock must be less than the system clock divided by 6. A -B argument of 250 should
# yield an SPI clock of around 4 kHz, which is fine. If your programmer doesn't respect -B,
# then you will have to find some other way to insure ISP operations don't go too fast.
SPICLOCK = 250

CC = $(AVR_PATH)bin/avr-gcc
OBJCPY = $(AVR_PATH)bin/avr-objcopy
AVRDUDE = $(AVR_PATH)bin/avrdude
AVRSIZE = $(AVR_PATH)bin/avr-size

CFLAGS = -Os -g -mmcu=$(CHIP) -std=c99 $(OPTS) -ffreestanding -Wall

DUDE_OPTS = -C $(AVR_PATH)etc/avrdude.conf -c $(PROG) -p $(CHIP) -B $(SPICLOCK)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

%.hex: %.elf
	$(OBJCPY) -j .text -j .data -O ihex $^ $@

# Calibrate is special - it has its own main()
calibrate.elf: calibrate.o
	$(CC) $(CFLAGS) -o $@ $^

%.elf: %.o base.o
	$(CC) $(CFLAGS) -o $@ $^
	$(AVRSIZE) -C --mcu=$(CHIP) $@

clean:
	rm -f *.o *.elf *.hex test-* *~

# The 32 kHz variant is fused for the extra-low frequency oscillator and no prescaling.
fuse:
	$(AVRDUDE) $(DUDE_OPTS) -U lfuse:w:0xe6:m -U hfuse:w:0xd7:m -U efuse:w:0xff:m

flash: $(TYPE).hex
	$(AVRDUDE) $(DUDE_OPTS) -U flash:w:$(TYPE).hex

# This will perturb the stored PRNG seed.
seed:
	dd if=/dev/urandom bs=4 count=1 of=seedfile
	$(AVRDUDE) $(DUDE_OPTS) -U eeprom:w:seedfile:r
	rm -f seedfile

# Apply a corrective offset to the clock.
# See offset.md
offset:
	$(AVRDUDE) $(DUDE_OPTS) -U eeprom:w:offset.hexi:i

# Set rhythm file into the eeprom. 
# Use 'make rhythm TYPE={the name of rhythm file you want}' e.g.: for rythm-normal.hexi file, 'make rhythm TYPE=normal'
# See rhythm.md
rhythm:
	$(AVRDUDE) $(DUDE_OPTS) -U eeprom:w:rhythm-$(TYPE).hexi:i

init: fuse flash seed offset

# Write EEPROM content into eeprom.hexo file in Intel HEX format.
readeeprom:
	$(AVRDUDE) $(DUDE_OPTS) -U eeprom:r:eeprom.hexo:i

test:
	gcc -c -DUNIT_TEST -O -o test-$(TYPE).o $(TYPE).c
	gcc -c -O test.c
	gcc -o test-$(TYPE) test.o test-$(TYPE).o
