#include "Particle.h"
#include "Util.h"
#include "SmartMeter.h"

boolean smart_meter_new_telegram;

unsigned long telegram_count;
unsigned long telegram_failed_count;
unsigned long telegram_missed_count;
unsigned int telegram_min_size;
unsigned int telegram_max_size;
unsigned long telegram_transmit_time;

unsigned long telegram_start_ts;
unsigned long telegram_end_ts;

double smart_meter_v[3];
double smart_meter_p[6];
double smart_meter_a[3];

const char *tags[] = {
    "1-0:32.7.0","1-0:52.7.0","1-0:72.7.0", // voltage
    "1-0:21.7.0","1-0:41.7.0","1-0:61.7.0", // active power in
    "1-0:22.7.0","1-0:42.7.0","1-0:62.7.0", // active power out
    "0-0:96.14.0", // tariff
    }; 


double smart_meter_tags[TAGS];
unsigned short crc;
unsigned short crc_in_message;
unsigned char state;
unsigned char state_tag;
unsigned int chars;
char tag[256];
unsigned char tag_index;
char tag_value[256];
unsigned char tag_value_index;
int tags_index;
unsigned char tags_found;

#define STATE_LOOKING_FOR_START   0
#define STATE_PARSING             1
#define STATE_CHECKING_CRC        2

#define STATE_TAG_COLLECTING      0
#define STATE_TAG_SKIPPING        1
#define STATE_TAG_VALUE           2


#define DETECT_STATE_LOOKING_FOR_LF     0
#define DETECT_STATE_COLLECTIN_LINE     1

// dsmr_version: 0 = unknown, 50 = 5.0 meter, -1 = testmode
int dsmr_version;
char dsmr_meter[256];

char telegram[TELEGRAM_MAX_SIZE];
int telegram_index;

unsigned int smartmeter_rx_size = 0;

// called by device-os
hal_usart_buffer_config_t acquireSerial2Buffer() {

    const size_t bufferSize = 3000;

    smartmeter_rx_size = bufferSize;

    hal_usart_buffer_config_t config = {
        .size = sizeof(hal_usart_buffer_config_t),
        .rx_buffer = new (std::nothrow) uint8_t[bufferSize],
        .rx_buffer_size = bufferSize,
        .tx_buffer = new (std::nothrow) uint8_t[128],
        .tx_buffer_size = 128
    };

    return(config);
}


void smart_meter_init() {

    // serial port already opened by logger
    // Serial2.begin(115200);

    telegram_index = 0;

    smart_meter_new_telegram = FALSE;
    dsmr_version = 0;
    telegram_count = 0;
    telegram_failed_count = 0;
    telegram_missed_count = 0;
    telegram_min_size = 99999;
    telegram_max_size = 0;
    telegram_transmit_time = 99999;
    
    telegram_start_ts = 0;
    telegram_end_ts = 0;

    Log.info("smartmeter_rx_size = %d",smartmeter_rx_size);
}

boolean smart_meter_is_usable() {

    return(dsmr_version == 50);
}

boolean smart_meter_is_testmode() {

    return(dsmr_version == -1);
}

void smart_meter_size_meter() {
    
    unsigned char c;
    unsigned char line_index = 0;
    char line[256];

    int read = Serial2.available();
    for(int i=0; i<read; i++) {
        Serial2.read();
    }

    do {

        int next_char = Serial2.read();

        if(next_char>=0) {

            c = next_char & CHAR_MASK;
            telegram_index++;
            line[line_index++]=c;

            if(c == 10) {

                if(line_index>2) {

                    // smartmeter ends lines with \r\n so remove also the \r
                    line[line_index-1]=0;

                    if(!strncmp(line,"1-3:0.2.8",9)) {

                        char *value = &line[10];
                        char *closing = strchr(value, ')');
                        *closing=0;
                        dsmr_version = atoi(value);

                        tags_found++;
                        
                    } else if(line[0]=='/') {
                        
                        strcpy(dsmr_meter,&line[1]);

                        // testing?
                        if(!strcmp(dsmr_meter,TEST_MODE_NAME)) {
                            dsmr_version = -1;
                            return;
                        }

                        tags_found++;
                    }
                }

                line_index = 0;
            }

            if(c == '/') {

                telegram_start_ts = millis();
                telegram_index = 1;
            }
        }
    } while(c != '!');

    // read crc (4x)
    Serial2.read();
    Serial2.read();
    Serial2.read();
    Serial2.read();
    // read cr-lf
    Serial2.read();
    Serial2.read();

    telegram_min_size = telegram_index + 6;
    telegram_max_size = telegram_min_size;

    telegram_end_ts = millis();
    telegram_transmit_time = telegram_end_ts - telegram_start_ts;
}

char smart_meter_read() {

    while(1==1) {

        int next_char = Serial2.read();

        if(next_char>=0) {

            return(next_char & CHAR_MASK);
        }
    }

    return(-1);
}

char smart_meter_read_update() {

    while(1==1) {

        int next_char = Serial2.read();

        if(next_char>=0) {

            return(next_char & CHAR_MASK);
        }
    }

    return(-1);
}

void smart_meter_process_serial() {

    if(millis() > telegram_end_ts + 1000) {

        unsigned char c;
        unsigned long start = millis();
        unsigned short crc_read;
        unsigned char line_index = 0;
        char line[256];

        telegram_index = 0;
        crc = 0;

        do {
        
            c = smart_meter_read();

        } while(c != '/');

        telegram_index ++;
        crc16_update(&crc, c);

        do {
        
            c = smart_meter_read();
            telegram_index ++;
            crc16_update(&crc, c);

            if(c == 10) {

                c = smart_meter_read();
                telegram_index ++;
                crc16_update(&crc, c);

                if(c != '!') {

                    // skip: -0:
                    c = smart_meter_read();
                    telegram_index ++;
                    crc16_update(&crc, c);

                    c = smart_meter_read();
                    telegram_index ++;
                    crc16_update(&crc, c);

                    c = smart_meter_read();
                    telegram_index ++;
                    crc16_update(&crc, c);

                    line_index = 0;
                    do {
                        c = smart_meter_read();
                        telegram_index ++;
                        crc16_update(&crc, c);

                        line[line_index++] = c;
                    } while(c!=13);

                    if(!strncmp(line,"32.7",4)) {
                        // 1-0:32.7.0(229.5*V)
                        line[12] = 0;
                        smart_meter_v[0] = atof(&line[7]);
                    } else if(!strncmp(line,"52.7",4)) {
                        line[12] = 0;
                        smart_meter_v[1] = atof(&line[7]);
                    } else if(!strncmp(line,"72.7",4)) {
                        line[12] = 0;
                        smart_meter_v[2] = atof(&line[7]);
                    } else if(!strncmp(line,"21.7",4)) {
                        // 1-0:32.7.0(229.5*V)
                        line[12] = 0;
                        smart_meter_p[0] = atof(&line[7]);
                    } else if(!strncmp(line,"41.7",4)) {
                        line[12] = 0;
                        smart_meter_p[1] = atof(&line[7]);
                    } else if(!strncmp(line,"61.7",4)) {
                        line[12] = 0;
                        smart_meter_p[2] = atof(&line[7]);
                    } else if(!strncmp(line,"22.7",4)) {
                        // 1-0:32.7.0(229.5*V)
                        line[12] = 0;
                        smart_meter_p[3] = atof(&line[7]);
                    } else if(!strncmp(line,"42.7",4)) {
                        line[12] = 0;
                        smart_meter_p[4] = atof(&line[7]);
                    } else if(!strncmp(line,"62.7",4)) {
                        line[12] = 0;
                        smart_meter_p[5] = atof(&line[7]);
                    }
                }
            }
          
        } while(c != '!');

        // read crc (4x)
        crc_read = from_hex(smart_meter_read());
        crc_read <<= 4;
        crc_read += from_hex(smart_meter_read());
        crc_read <<= 4;
        crc_read += from_hex(smart_meter_read());
        crc_read <<= 4;
        crc_read += from_hex(smart_meter_read());

        // read cr-lf
        smart_meter_read();
        smart_meter_read();

        telegram_index += 6;

        telegram_end_ts = millis();
        
        unsigned long duration = telegram_end_ts - start;

        if(crc==crc_read) {

            smart_meter_a[0] = 1000.0*(smart_meter_p[0]-smart_meter_p[3])/smart_meter_v[0];
            smart_meter_a[1] = 1000.0*(smart_meter_p[1]-smart_meter_p[4])/smart_meter_v[1];
            smart_meter_a[2] = 1000.0*(smart_meter_p[2]-smart_meter_p[5])/smart_meter_v[2];

            Log.info("read in %d in %d ms: a=%f, %f, %f",
                telegram_index, (int) duration,
                smart_meter_a[0], smart_meter_a[1], smart_meter_a[2]);
            // Log.info("a=%f, %f, %f",smart_meter_a[0],smart_meter_a[1],smart_meter_a[2]);
            //Log.info("v=%f, %f, %f",smart_meter_v[0],smart_meter_v[1],smart_meter_v[2]);
            //Log.info("p=%f, %f, %f",smart_meter_p[0],smart_meter_p[1],smart_meter_p[2]);
            //Log.info("p=%f, %f, %f",smart_meter_p[3],smart_meter_p[4],smart_meter_p[5]);
        } else {
            Log.warn("bad crc read in %d in %d ms (%x = %x)", telegram_index, (int) duration, (int)crc, (int)crc_read);
        }
    }
}

void smart_meter_identify() {

    smart_meter_size_meter();
    
    if(dsmr_version == 50) {
        Log.info("SmartMeter found: %s",dsmr_meter);
    } else {
        Log.warn("No or incompatible SmartMeter found (%d: %s)",
            dsmr_version, dsmr_meter);
    }

     Log.info("initial telegram size: %d (%lu ms)",
            telegram_min_size, telegram_transmit_time);
}


void smart_meter_dump_stats() {

      Log.info("%lu %lu %lu %u %u",
        telegram_count, telegram_failed_count, telegram_missed_count,
        telegram_min_size, telegram_max_size);
}


void smart_meter_process_serial_() {

    int available = Serial2.available();
  
    if(available) {

        while(available--) {

            int next_char = Serial2.read();
            unsigned char c = next_char & CHAR_MASK;

            telegram[telegram_index++] = c;

            if(state==STATE_PARSING) {
        
                crc16_update(&crc, c);

                chars++;

                if(c==10) {

                    state_tag = STATE_TAG_COLLECTING;
                    tag_index=0;
                    tag_value_index=0;

                } else if(state_tag == STATE_TAG_COLLECTING) {

                    if(c=='(') {
                    
                        tag[tag_index] = 0;

                        state_tag = STATE_TAG_SKIPPING;

                        for (int i = 0; i < TAGS; i++) {

                            if(!strcmp(tag,tags[i])) {
                                state_tag = STATE_TAG_VALUE;
                                tags_index = i;
                                break;
                            }
                        }

                    } else {

                        tag[tag_index++] = c;
                    }

                } else if(state_tag == STATE_TAG_VALUE) {

                    if(c==')') {

                        tags_found++;
                        state_tag = STATE_TAG_SKIPPING;
                        tag_value[tag_value_index] = 0;
                        smart_meter_tags[tags_index] = atof(tag_value);

                    } else {

                        tag_value[tag_value_index++] = c;
                    }
                }
            }

            if(c=='/') {

                unsigned long now = millis();
                if((now-telegram_start_ts) > 1100) {
                    telegram_missed_count++;
                }
                telegram_start_ts = now;

                state = STATE_PARSING;
                crc = 0;
                crc16_update(&crc, c);

                chars = 1;

            } else if(c=='!') {

                state=STATE_CHECKING_CRC;
                crc_in_message = 0;

            } else if(state==STATE_CHECKING_CRC) {

                if(c==13) {

                    state = STATE_LOOKING_FOR_START;

                    telegram_count++;          

                    if(crc != crc_in_message) {

                        telegram_failed_count++;

                        int i=0;
                        while(i<telegram_index) {

                            int j=i;
                            while((telegram[j++]!=13) && (i<telegram_index)) {
                            }
                            telegram[j-1]=0;

                            Log.info("%s",&telegram[i]);

                            i = j+1;
                        }

                    } else if(tags_found<TAGS_MIN) {

                        telegram_failed_count++;

                    } else {

                        smart_meter_new_telegram = TRUE;

                        if(chars > telegram_max_size) {
                            telegram_max_size = chars;
                        }
                        
                        if(chars < telegram_min_size) {
                            telegram_min_size = chars;
                        }
                    }

                    tags_found = 0;
                    telegram_index=0;

                } else {

                    chars++;

                    crc_in_message*=16;
                    crc_in_message += from_hex(c);
                }
            }
        }
    }
}

