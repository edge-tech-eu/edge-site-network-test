#ifndef SMARTMETER_H
#define SMARTMETER_H

#define TEST_MODE_NAME  "edgeTest"

#define CHAR_MASK   0xff

#define TELEGRAM_MAX_SIZE   3000

#define TAGS          10
#define TAGS_MIN       6

#define TAG_TARIFF      9

extern double smart_meter_tags[TAGS];
extern boolean smart_meter_new_telegram;

void smart_meter_init(void);
void smart_meter_size_meter(void);
void smart_meter_identify(void);

boolean smart_meter_is_testmode(void);
boolean smart_meter_is_usable(void);
boolean smart_meter_detect(void);

void smart_meter_process_serial(void);

void smart_meter_dump_stats(void);


#endif
