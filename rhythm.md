Rhythm files
============

Rhythm<name>.hex0 files are in Intel HEX file format (see https://en.wikipedia.org/wiki/Intel_HEX) used to set sequences of sleep times for the clock.

Bytes from addresses 6 to 67 can be used:
- byte 6 : sleep times count (1 byte - between 1 and 59)
- bytes 7-8 : wait time between each sequence of sleep times (unsigned int)
- bytes 9-67 : sleep times (1 byte each - between 0 and 255)

To insure that the correct ticking frequency, 60 ticks per minute, is maintained:
- 60 modulo (sleep times count + 1) MUST be equal to 0
- The sum of sleep times + wait time MUST be equal to (sleep times count + 1) * 10

Only the first line had to be modified:
1. Start code, one character, an ASCII colon ':'.
2. Byte count, two hex digits, indicating the number of bytes, from '04' to '3E'.
3. Address, four hex digits, representing the 16-bit beginning memory address offset of the data, '0006'.
4. Record type, two hex digits, defining the meaning of the data field, data '00'.
5. Data, a sequence of n bytes of data, represented by n * 2 hex digits, sleep times count, wait time and sleep times.
6. Checksum, two hex digits, is the two's complement (negative) to the 8 bits checksum of the fields 2. to 5. above.

The wait time is an unsigned 16 bits value. To convert it to hexadecimal, use your calculator or http://www.binaryconvert.com/convert_unsigned_short.html.
Write bytes in little indian order: e.g. for 30 tenths-of-a-second, i.e. 0x001E, write 1E00.

To calculate the checksum, use http://www.lammertbies.nl/comm/info/crc-calculation.html to calculate the sum of the bytes data fields, then convert to hex and take the two's complement.

To set the file rhythm-normal.hexi in eeprom, use 'make rythmn TYPE=normal' and 'make flash TYPE=rhythm' to store the program in flash memory.
