#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "data.h"

// based on code from:
// http://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format?rq=1

const char *to_binary(int x)
{
    static char bits[17];
    bits[0] = '\0';
    int z;
    for (z = 1 << 15; z > 0; z >>= 1)
    {
        strcat(bits, ((x & z) == z) ? "1" : "0");
    }
    return bits;
}


/*
  Process one bit of the message.
  reg: register (modified by this function)
  key: CRC16 key being used
  next_bit: next bit of message
*/

void crc_bit(unsigned short *reg, unsigned int key, unsigned int next_bit)
{
    unsigned short sigBit = *reg >> 15;
    *reg <<= 1;
    *reg |= next_bit;
    if (sigBit == 1)
    {
        *reg ^= key;
    }
}

/*
  Process one byte of the message.
  reg: register (modified by this function)
  key: CRC16 key being used
  next_byte: next byte of message
*/

void crc_byte(unsigned short *reg, unsigned int key, unsigned int next_byte)
{
    int i;
    unsigned short next_bit;
    for (i = 0; i < 8; ++i)
    {
        // Using "0b00000001" here instead of "1" for clarity/readability sake.
        next_bit = (next_byte >> (7 - i)) & 0b00000001;
        crc_bit(reg, key, next_bit);
    }
}

/*
  Process an entire message using CRC16 and return the CRC.
  key: CRC16 key being used
  message: message for which we are calculating the CRC.
  num_bytes: size of the message in bytes.
*/

unsigned short crc_message(unsigned int key,  unsigned char *message, int num_bytes)
{
    unsigned short *reg = malloc(sizeof(unsigned short));
    *reg = 0;
    int i, y;
    unsigned short crcOut;
    // Crc on all the bytes.
    for (i = 0; i < num_bytes; ++i)
    {
        crc_byte(reg, key, message[i]);
    }

    // Do the additional 16 iterations.
    for (y = 1; y <= 16; ++y)
    {
        crc_bit(reg, key, 0);
    }
    crcOut = *reg;
    free(reg);
    return crcOut;
}



