#include "Particle.h"
#include "Connectivity.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"


int connectivity_state;
int connectivity_state_previous;
bool connectivity_has_mac;
int connectivity_reconnects;

unsigned long connectivity_time_start;
unsigned long connectivity_time_linkup;
unsigned long connectivity_time_connected;
unsigned long connectivity_time_disconnected;

unsigned long timeout_start;
unsigned int timeout_delta;

void timeouttimer_set(unsigned int time) {

    Log.info("timeout set to %u ms",time);

    timeout_start = millis();
    timeout_delta = time;
}

bool timeouttimer_expired() {

    if((millis() - timeout_start) > (unsigned long)timeout_delta) {

        Log.info("timed out");

        return(true);
    }

    return(false);
}

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

bool connectivity_first_connected() {

    return(connectivity_reconnects == 1);
}

int connectivity_get_state() {

    return(connectivity_state);
}

bool connectivity_has_ethernet() {

   return(connectivity_has_mac);
}

bool connectivity_has_wifi() {

    WiFiAccessPoint ap[5];
    int found = WiFi.getCredentials(ap, 5);

    Log.info("found %d credentials",found);

    for (int i = 0; i < found; i++) {
        Log.info("ssid: %s", ap[i].ssid);
        // security is one of WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA, WLAN_SEC_WPA2, WLAN_SEC_WPA_ENTERPRISE, WLAN_SEC_WPA2_ENTERPRISE
        // Log.info("security: %d", (int) ap[i].security);
        // cipher is one of WLAN_CIPHER_AES, WLAN_CIPHER_TKIP or WLAN_CIPHER_AES_TKIP
        // Log.info("cipher: %d", (int) ap[i].cipher);
    }

    return(WiFi.hasCredentials());
}

bool connectivity_connect() {

    bool status = Particle.connected();

    if(!status) {

        switch(connectivity_state) {

            case CONNECTIVITY_ETHERNET_CLOUD_CONNECTED:

                connectivity_time_disconnected = millis();

                Particle.disconnect();
                Ethernet.disconnect();

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

                    Ethernet.disconnect();

                    Log.info("Ethernet network connecting takes too long (disconnect)");
                    
                    connectivity_state = CONNECTIVITY_WIFI_CONNECT;
                }

                break;

            case CONNECTIVITY_ETHERNET_CLOUD_WAIT_CONNECTED:

                if(!Ethernet.ready()) {

                    Log.info("While waiting to connect to cloud, ethernet disconnected (disconnect)");

                    Particle.disconnect();                    
                    Ethernet.disconnect();

                    connectivity_state = CONNECTIVITY_WIFI_CONNECT;

                    break;
                }
                
                if(timeouttimer_expired()) {

                    Log.info("Ethernet cloud connecting takes too long (disconnect)");

                    Particle.disconnect();
                    Ethernet.disconnect();

                    connectivity_state = CONNECTIVITY_WIFI_CONNECT;
                }

                break;

            case CONNECTIVITY_WIFI_CLOUD_CONNECTED:

                connectivity_time_disconnected = millis();

                Particle.disconnect();
                WiFi.disconnect();

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

                    WiFi.disconnect();

                    Log.info("WiFi network connecting takes too long, disconnect");

                    connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;
                }

                break;

            case CONNECTIVITY_WIFI_CLOUD_WAIT_CONNECTED:

                 if(!WiFi.ready()) {
                    
                    Log.info("While waiting to connect to cloud, wifi disconnected (disconnect)");
                    
                    Particle.disconnect();
                    WiFi.disconnect();

                    connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;
                 }

                if(timeouttimer_expired()) {

                    Log.info("Wifi cloud connecting takes too long, disconnect");

                    Particle.disconnect();
                    WiFi.disconnect();

                    connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;
                }

                break;
        }

    } else {

        unsigned long new_time_connected;
        int reconnect_time;

        switch(connectivity_state) {

            case CONNECTIVITY_ETHERNET_CONNECT:

                /* fall through */

            case CONNECTIVITY_ETHERNET_WAIT_CONNECTED:

                Log.info("Weird, connected from state %s", connectivity_state_name(connectivity_state).c_str());
            
            case CONNECTIVITY_ETHERNET_CLOUD_WAIT_CONNECTED:

                new_time_connected =  millis();
                reconnect_time = (int)((new_time_connected-connectivity_time_connected)/1000);
                connectivity_time_connected = new_time_connected;

                connectivity_reconnects++;

                Log.info("Ethernet cloud connected in %d s (link: %d s), (re)connected in %d s",
                    (int)((connectivity_time_connected - connectivity_time_start)/1000),
                    (int)((connectivity_time_linkup - connectivity_time_start)/1000),
                    reconnect_time);

                connectivity_state = CONNECTIVITY_ETHERNET_CLOUD_CONNECTED;

                break;

            case CONNECTIVITY_ETHERNET_CLOUD_CONNECTED:

                break;

            case CONNECTIVITY_WIFI_CONNECT:

                /* fall through */

            case CONNECTIVITY_WIFI_WAIT_CONNECTED:
            
                Log.info("Weird, connected from state %s", connectivity_state_name(connectivity_state).c_str());

            case CONNECTIVITY_WIFI_CLOUD_WAIT_CONNECTED:

                new_time_connected =  millis();
                reconnect_time = (int)((new_time_connected-connectivity_time_connected)/1000);
                connectivity_time_connected = new_time_connected;

                connectivity_reconnects++;
                
                Log.info("WiFi cloud connected in %d s (link: %d s), (re)connected in %d s",
                    (int)((connectivity_time_connected - connectivity_time_start)/1000),
                    (int)((connectivity_time_linkup - connectivity_time_start)/1000),
                    reconnect_time);

                connectivity_state = CONNECTIVITY_WIFI_CLOUD_CONNECTED;

                break;

            case CONNECTIVITY_WIFI_CLOUD_CONNECTED:
            
                break;
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

void connectivity_dump_stats(void) {

    Log.info("State: '%s', reconnects: %d",
            connectivity_state_name(connectivity_state).c_str(),connectivity_reconnects);
}

#pragma GCC diagnostic pop