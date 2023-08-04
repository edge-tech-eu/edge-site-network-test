#include "Particle.h"
#include "Led.h"

// startup condition
int led_status[5] = { LED_OFF,   LED_OFF,      LED_OFF,    LED_OFF,     LED_OFF};
int led_pin[5] =    { LED_ERROR, LED_ACTIVITY, LED_CLOUD,  LED_NETWORK, LED_POWER};
int led_phase = 0;

void flash_led(void);

Timer timer(500, flash_led);

void flash_led() {

    if(led_phase) {

        led_phase=0;

        for(int i=0;i<5;i++) {
            if(led_status[i]==LED_FLASHING) {
                digitalWrite(led_pin[i], LED_ON_WRITE);
            }
        }

    } else {

        led_phase=1;

         for(int i=0;i<5;i++) {
            if(led_status[i]==LED_FLASHING) {
                digitalWrite(led_pin[i], LED_OFF_WRITE);
            }
        }
    }
}

void led_handle_cloud_events(system_event_t event, int param) {

    switch(param) {

        case cloud_status_connecting:       led_set_status(LED_CLOUD, LED_FLASHING); break;
        case cloud_status_connected:        led_set_status(LED_CLOUD, LED_ON);       break;
        case cloud_status_disconnecting:    led_set_status(LED_CLOUD, LED_OFF);      break;
        case cloud_status_disconnected:     led_set_status(LED_CLOUD, LED_OFF);      break;
    }
}

void led_handle_network_events(system_event_t event, int param) {

switch(param) {

        case network_status_powering_on:    led_set_status(LED_NETWORK, LED_FLASHING);   break;
        case network_status_on:             led_set_status(LED_NETWORK, LED_FLASHING);   break;
        case network_status_powering_off:   led_set_status(LED_NETWORK, LED_OFF);        break;
        case network_status_off:            led_set_status(LED_NETWORK, LED_OFF);        break;
        case network_status_connecting:     led_set_status(LED_NETWORK, LED_FLASHING);   break;
        case network_status_connected:      led_set_status(LED_NETWORK, LED_ON);         break;
        case network_status_disconnecting:  led_set_status(LED_NETWORK, LED_OFF);        break;
        case network_status_disconnected:   led_set_status(LED_NETWORK, LED_OFF);        break;
    }
}


int led_pin_to_index(int led) {

    switch(led) {

        case LED_R1: return(0);
        case LED_G1: return(1);
        case LED_G2: return(2);
        case LED_G3: return(3);
        case LED_G4: return(4);
    }

    return(0);
}

void led_set_status(int led, int status) {

    switch(status) {

        case LED_OFF:

            led_status[led_pin_to_index(led)]=LED_OFF;
            digitalWrite(led,LED_OFF_WRITE);

            break;

        case LED_ON:

            led_status[led_pin_to_index(led)]=LED_ON;
            digitalWrite(led, LED_ON_WRITE);

            break;

        case LED_FLASHING:

            led_status[led_pin_to_index(led)]=LED_FLASHING;

            break;
    }
}

void led_power() {

    led_set_status(LED_POWER, LED_ON);

}

void led_test_mode() {

    for(int i=0;i<5;i++) {
        led_status[i]=LED_FLASHING;
    }
}

void led_init() {

    RGB.brightness(128);

    pinMode(LED_R1, OUTPUT);
    pinMode(LED_G1, OUTPUT);
    pinMode(LED_G2, OUTPUT);
    pinMode(LED_G3, OUTPUT);
    pinMode(LED_G4, OUTPUT);

    for(int i=0;i<5;i++) {
        switch(led_status[i]) {
            case LED_OFF:  digitalWrite(led_pin[i], LED_OFF_WRITE); break;
            case LED_ON:   digitalWrite(led_pin[i], LED_ON_WRITE); break;
        }
    }

    timer.start();
}