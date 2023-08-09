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

unsigned short crc;
unsigned short crc_in_message;

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

    for(int i=0;i<3;i++) {
    
        smart_meter_v[i] = 0;
        smart_meter_p[i] = 0;
        smart_meter_p[i+3] = 0;
    }

    Log.info("smartmeter_rx_size = %d",smartmeter_rx_size);
}

boolean smart_meter_is_usable() {

    return(dsmr_version == 50);
}

boolean smart_meter_is_testmode() {

    return(dsmr_version == -1);
}


char smart_meter_read() {

    unsigned long till = millis() + 1000;

    while(till > millis()) {

        int next_char = Serial2.read();

        if(next_char >= 0) {

            return(next_char & CHAR_MASK);
        }
    }

    Log.error("No data!");

    return(-1);
}

void smart_meter_size_meter() {
    
    unsigned char c;
    unsigned char line_index = 0;
    char line[256];
    unsigned long till = millis() + 2000;

    // read flush, for timing
    int read = Serial2.available();
    for(int i=0; i<read; i++) {
        Serial2.read();
    }

    do {

        c = smart_meter_read();
        
        telegram_index++;
        line[line_index++] = c;

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

    } while((c != '!') && (till > millis()));


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


char smart_meter_read_update() {

    while(1==1) {

        int next_char = Serial2.read();

        if(next_char>=0) {

            return(next_char & CHAR_MASK);
        }
    }

    return(-1);
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

        if(c < 0) {

            Log.error("no smartmeter data found!");
            return;
        }

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

            for(int i=0; i<3; i++) {

                smart_meter_p[i]-=smart_meter_p[i+3];
                smart_meter_p[i] *= 1000.;

                if((int)smart_meter_p[i] == 0) {
                    smart_meter_a[i] = 0;
                } else {
                    smart_meter_a[i] = smart_meter_p[i]/smart_meter_v[i];
                }
           }

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
