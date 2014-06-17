#include <string.h>
#include <stdio.h>


// based on code from:
// http://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format?rq=1

const char *to_binary(unsigned int x) {
    static char bits[17];
    bits[0] = '\0';
    unsigned int z;
    for (z = 1 << 15; z > 0; z >>= 1) {
        strcat(bits, (x & z) ? "1" : "0");
    }
    return bits;
}


/*Return 1 iff bit i in x is set, 0 otherwise.
 *Remember that the bits are numbered 0 from the right:
 *b15b14b13 ... b3b2b1b0
*/

short is_set(unsigned short x, int bit) {

    x >>= bit; 
    return x%2;

}

/* Swap the first 8 bits and the last 8 bits of 16-bit value.
 *For example, 00000001 11001100
 *becomes 11001100 00000001
 */
 
unsigned short swap_bytes(unsigned short x) {
    unsigned short left = 0b1111111100000000;
    unsigned short right = 0b0000000011111111;
    unsigned short out;
    left &= x;
    right &= x;
    left >>= 8;
    right <<= 8;
    out = left | right;
    return out;
}

/*A value has even parity if it has an even number of 1 bits.
 *A value has an odd parity if it has an odd number of 1 bits.
 *For example, 0110 has even parity, and 1110 has odd parity.
 *Return 1 iff x has even parity.
 */
 
int has_even_parity(unsigned int x) {
    int z;
    unsigned int checker = 0;
    for (z = 0; z < sizeof(unsigned int)*8; ++z)
    {
        checker ^= x;
        checker >>= 1;
    }
    if(checker % 2 == 0){
        return 1;
    }
    return 0;
}

/*Does the following version of the function work?*/

int has_even_parity_tricky(unsigned int x) {
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return !x;
}

int main(void) {
    unsigned short sample = 0b0000000111001100;
    printf("Test value: %s\n", to_binary(sample));
    printf("Is bit 3 set? Expect: 1. Got: %d\n", is_set(sample, 3));
    printf("Is bit 4 set? Expect: 0. Got: %d\n", is_set(sample, 4));
    printf("Swapping the bytes gives: %s\n", to_binary(swap_bytes(sample)));
    printf("Even parity? Expect: 0. Got: %d\n", has_even_parity(sample));

    
    return 0;
}
