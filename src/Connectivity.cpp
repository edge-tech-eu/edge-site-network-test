#include "Particle.h"
#include "Connectivity.h"
#include "TimeoutTimer.h"

int connectivity_state;
int connectivity_state_previous;
bool connectivity_has_mac;

unsigned long connectivity_time_start;
unsigned long connectivity_time_linkup;
unsigned long connectivity_time_connected;
unsigned long connectivity_time_disconnected;

#define CONNECTIVITY_STATE_UNKNOWN                (-1)
#define CONNECTIVITY_ETHERNET_CONNECT               0
#define CONNECTIVITY_ETHERNET_WAIT_CONNECTED        1
#define CONNECTIVITY_ETHERNET_CLOUD_WAIT_CONNECTED  2
#define CONNECTIVITY_ETHERNET_CLOUD_CONNECTED       3
#define CONNECTIVITY_WIFI_CONNECT                   4
#define CONNECTIVITY_WIFI_WAIT_CONNECTED            5
#define CONNECTIVITY_WIFI_CLOUD_WAIT_CONNECTED      6
#define CONNECTIVITY_WIFI_CLOUD_CONNECTED           7


// https://en.wikipedia.org/wiki/Constrained_Application_Protocol


String connectivity_state_name(int state) {

    switch(state) {

        case -1: return("unknown"); break;
        case  0: return("eth connect"); break;
        case  1: return("eth connecting"); break;
        case  2: return("eth cloud connecting"); break;
        case  3: return("eth cloud connected"); break;
        case  4: return("wifi connect"); break;
        case  5: return("wifi connecting"); break;
        case  6: return("wifi cloud connecting"); break;
        case  7: return("wifi cloud connected"); break;
    }

    return("undefined");
}


void connectivity_init() {

    connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;
    connectivity_state_previous = CONNECTIVITY_STATE_UNKNOWN;

    uint8_t mac[6];
    connectivity_has_mac = (Ethernet.macAddress(mac) != 0);
    // connectivity_has_mac = false;

    connectivity_time_connected = 0;
}

int connectivity_get_state() {

    return(connectivity_state);
}

bool connectivity_has_ethernet() {

   return(connectivity_has_mac);
}

bool connectivity_has_wifi() {

    return(WiFi.hasCredentials());
}

bool connectivity_connect() {

    bool status = Particle.connected();

    if(!status) {

        switch(connectivity_state) {

            case CONNECTIVITY_ETHERNET_CLOUD_CONNECTED:

                connectivity_time_disconnected = millis();

                Log.info("Ethernet cloud disconnected (after %d s), try Ethernet",
                   (int)((connectivity_time_disconnected - connectivity_time_connected)/1000));

                connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;

            break;

            case CONNECTIVITY_ETHERNET_CONNECT:

                if(!connectivity_has_ethernet()) {

                    connectivity_state = CONNECTIVITY_WIFI_CONNECT;

                    break;
                }

                Log.info("Connecting Ethernet");

                WiFi.disconnect();
                Ethernet.connect();

                connectivity_time_start = millis();
                
                timeouttimer_set(TIMEOUT_ETHERNET_CONNECT);

                connectivity_state = CONNECTIVITY_ETHERNET_WAIT_CONNECTED;
                
                break;

            case CONNECTIVITY_ETHERNET_WAIT_CONNECTED:

                if(Ethernet.ready()) {

                    connectivity_time_linkup = millis();

                    Log.info("Ethernet network connected in %d s",
                        (int)((connectivity_time_linkup - connectivity_time_start)/1000));
                    Log.info("Ethernet IP address: %s", Ethernet.localIP().toString().c_str());
                
                    connectivity_state = CONNECTIVITY_ETHERNET_CLOUD_WAIT_CONNECTED;

                    Particle.keepAlive(25s);
                    Particle.connect();

                    timeouttimer_set(TIMEOUT_CLOUD_CONNECT);

                } else if(timeouttimer_expired()) {

                    Log.info("Ethernet network connecting takes too long");
                    
                    connectivity_state = CONNECTIVITY_WIFI_CONNECT;
                }

                break;

            case CONNECTIVITY_ETHERNET_CLOUD_WAIT_CONNECTED:

                if(timeouttimer_expired()) {

                    Log.info("Ethernet cloud connecting takes too long");

                    Particle.disconnect();
                    Ethernet.disconnect();

                    connectivity_state = CONNECTIVITY_WIFI_CONNECT;
                }

                break;

            case CONNECTIVITY_WIFI_CLOUD_CONNECTED:

                connectivity_time_disconnected = millis();

                Log.info("WiFi cloud disconnected (after %d s), try Ethernet",
                   (int)((connectivity_time_disconnected - connectivity_time_connected)/1000));

                connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;

            break;

            case CONNECTIVITY_WIFI_CONNECT:

                if(!connectivity_has_wifi()) {

                    connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;

                    break;
                }

                Log.info("Connecting WiFi");

                Ethernet.disconnect();
                WiFi.connect();
                
                connectivity_time_start = millis();

                timeouttimer_set(TIMEOUT_WIFI_CONNECT);

                connectivity_state = CONNECTIVITY_WIFI_WAIT_CONNECTED;
                
                break;

            case CONNECTIVITY_WIFI_WAIT_CONNECTED:

                if(WiFi.ready()) {

                    connectivity_time_linkup = millis();

                    Log.info("WiFi network connected in %d s",
                        (int)((connectivity_time_linkup - connectivity_time_start)/1000));
                    Log.info("WiFi IP address: %s", WiFi.localIP().toString().c_str());

                    connectivity_state = CONNECTIVITY_WIFI_CLOUD_WAIT_CONNECTED;
                    
                    Particle.keepAlive(25s);
                    Particle.connect();

                    timeouttimer_set(TIMEOUT_CLOUD_CONNECT);

                }   else if(timeouttimer_expired()) {

                    Log.info("WiFi network connecting takes too long");

                    connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;
                }

                break;

            case CONNECTIVITY_WIFI_CLOUD_WAIT_CONNECTED:

                if(timeouttimer_expired()) {

                    Log.info("Wifi cloud connecting takes too long");

                    Particle.disconnect();
                    WiFi.disconnect();

                    connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;
                }

                break;
        }

    } else {

        unsigned long new_time_connected;

        switch(connectivity_state) {

            case CONNECTIVITY_ETHERNET_CONNECT:
            case CONNECTIVITY_ETHERNET_WAIT_CONNECTED:

                Log.info("Weird, connected from state %s", connectivity_state_name(connectivity_state).c_str());
            
            case CONNECTIVITY_ETHERNET_CLOUD_WAIT_CONNECTED:

                new_time_connected =  millis();
                Log.info("reconnection time %d s",(int)((new_time_connected-connectivity_time_connected)/1000));
                connectivity_time_connected = new_time_connected;

                Log.info("Ethernet cloud connected in %d s (link: %d s)",
                    (int)((connectivity_time_connected - connectivity_time_start)/1000),
                    (int)((connectivity_time_connected - connectivity_time_linkup)/1000));

                connectivity_state = CONNECTIVITY_ETHERNET_CLOUD_CONNECTED;

                break;

            case CONNECTIVITY_ETHERNET_CLOUD_CONNECTED: break;

            case CONNECTIVITY_WIFI_CONNECT:
            case CONNECTIVITY_WIFI_WAIT_CONNECTED:
            
                Log.info("Weird, connected from state %s", connectivity_state_name(connectivity_state).c_str());

            case CONNECTIVITY_WIFI_CLOUD_WAIT_CONNECTED:

                new_time_connected =  millis();
                Log.info("reconnection time %d s",(int)((new_time_connected-connectivity_time_connected)/1000));
                connectivity_time_connected = new_time_connected;

                Log.info("WiFi cloud connected in %d s (link: %d s)",
                    (int)((connectivity_time_connected - connectivity_time_start)/1000),
                    (int)((connectivity_time_connected - connectivity_time_linkup)/1000));

                connectivity_state = CONNECTIVITY_WIFI_CLOUD_CONNECTED;

                break;

            case CONNECTIVITY_WIFI_CLOUD_CONNECTED: break;
        }
    }

    if(connectivity_state_previous != connectivity_state) {

        Log.info("State: '%s' (previous: '%s')",
            connectivity_state_name(connectivity_state).c_str(),
            connectivity_state_name(connectivity_state_previous).c_str());

        connectivity_state_previous = connectivity_state;
    }

    return(status);
}