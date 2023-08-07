#include "Particle.h"
#include "Util.h"


void crc16_update(unsigned short *crc, unsigned char c) {
	
	unsigned short new_crc = *crc;

    new_crc ^= c;
    
	for (int i = 0; i < 8; ++i) {
		if (new_crc & 1) {
			new_crc= (new_crc >> 1) ^ 0xA001;
		} else {
			new_crc = (new_crc >> 1);
		}
	}

	*crc = new_crc;
}

unsigned short a_to_us(char* str, unsigned char digital_after_comma) {

    unsigned short value = 0;
    
    while(*str != 0) {
        unsigned short digit = (*str++) - '0';
        if(digit<10) {
            value *= 10;
            value += digit;
        } else {
            break;
        }
    }
    
    unsigned char cnt = digital_after_comma;

    while((*str != 0) && (cnt>0)) {
        unsigned short digit = (*str++) - '0';
        if(digit<10) {
            value *= 10;
            value += digit;
        } else {
            break;
        }
        
        cnt--;
    }

    while(cnt--) {
        value *= 10;
    }
    
    return(value);
}

unsigned char from_hex(unsigned char v) {

    if(v>'9') {
        if(v>'F') {
            return(v+10-'a');
        } else {
            return(v+10-'A');
        }
    } else {
        return(v-'0');
    }
}

unsigned char from_hex(unsigned char v1, unsigned char v2) {

    return((from_hex(v1)<<4)+from_hex(v2));
}

void to_string_boolean(unsigned short value, char *string) {

    if(value) {
        sprintf(string,"\"true\"");
    } else {
        sprintf(string,"\"false\"");
    }
}

void to_string_decimal(unsigned short value, char *string) {

    sprintf(string,"\"%d\"",value);
}

char to_hex(unsigned char v) {

    if(v<10) {
        return('0'+v);
    } else {
        return('a'+(v-10));
    }
}

char to_hex_hi(unsigned char v) {

    return(to_hex(v>>4));
}

char to_hex_lo(unsigned char v) {

    return(to_hex(v&0x0f));
}