Offset file
===========

offset.hex0 file is in Intel HEX file format (see https://en.wikipedia.org/wiki/Intel_HEX) used to apply a corrective offset to the clock.

The two bytes at addresses 4-5 of the EEPROM are the value, as a signed 16 bits value in tenths-of-a-ppm. 
Positive values slow the clock down.

Only the first line had to be modified:
1. Start code, one character, an ASCII colon ':'.
2. Byte count, two hex digits, indicating the number of bytes, '02'.
3. Address, four hex digits, representing the 16-bit beginning memory address offset of the data, '0004'.
4. Record type, two hex digits, defining the meaning of the data field, data '00'.
5. Data, a sequence of 2 bytes of data, represented by 4 hex digits, the offset value.
6. Checksum, two hex digits, is the two's complement (negative) to the 8 bits checksum of the fields 2. to 5. above.

The offset value is a signed 16 bits value. To convert it to hexadecimal, use your calculator and take the two's complement for negative values or use http://www.binaryconvert.com/convert_signed_short.html.
Write bytes in little indian order: e.g. for 30 tenths-of-a-ppm, i.e. 0x001E, write 1E00.

To calculate the checksum, use http://www.lammertbies.nl/comm/info/crc-calculation.html to calculate the sum of the bytes data fields and then take the two's complement.

To set the offset value in eeprom, use 'make offset'.
