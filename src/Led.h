#ifndef LED_H
#define LED_H

#define LED_ON_WRITE     LOW
#define LED_OFF_WRITE    HIGH
#define LED_OFF          0
#define LED_ON           1
#define LED_FLASHING     2

#define LED_R1  D19
#define LED_G1  A0
#define LED_G2  A1
#define LED_G3  A2
#define LED_G4  A5

#define LED_ERROR       LED_R1
#define LED_POWER       LED_G4
#define LED_NETWORK     LED_G3
#define LED_CLOUD       LED_G2
#define LED_ACTIVITY    LED_G1

void led_init(void);
void led_set_status(int led, int status);
void led_test_mode(void);
void led_power(void);

void led_handle_cloud_events(system_event_t event, int param);
void led_handle_network_events(system_event_t event, int param);

#endif