#ifndef UTIL_h
#define UTIL_h

void crc16_update(unsigned short *crc, unsigned char c);

unsigned short a_to_us(char* str, unsigned char digital_after_comma);

unsigned char from_hex(unsigned char v);
unsigned char from_hex(unsigned char v1, unsigned char v2);

void to_string_boolean(unsigned short value, char *string);
void to_string_decimal(unsigned short value, char *string);
void to_string_state(unsigned short value, char *string);

char to_hex(unsigned char v);
char to_hex_hi(unsigned char v);
char to_hex_lo(unsigned char v);

#endif