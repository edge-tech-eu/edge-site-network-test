#include "Particle.h"
#include "Connectivity.h"


unsigned long timeout_delta;
unsigned long now_delta;


#define CONNECTIVITY_STATE_UNKNOWN                (-1)
#define CONNECTIVITY_ETHERNET_CONNECT               0
#define CONNECTIVITY_ETHERNET_WAIT_CONNECTED        1
#define CONNECTIVITY_ETHERNET_CLOUD_WAIT_CONNECTED  2
#define CONNECTIVITY_WIFI_CONNECT                   3
#define CONNECTIVITY_WIFI_WAIT_CONNECTED            4
#define CONNECTIVITY_WIFI_CLOUD_WAIT_CONNECTED      5
#define CONNECTIVITY_CLOUD_CONNECTED                6

String connectivity_state_name(int state) {

    switch(state) {

        case -1: return("unknown"); break;
        case  0: return("eth connect"); break;
        case  1: return("eth connecting"); break;
        case  2: return("eth cloud connecting"); break;
        case  3: return("wifi connect"); break;
        case  4: return("wifi connecting"); break;
        case  5: return("wifi cloud connecting"); break;
        case  6: return("cloud connected"); break;
    }

    return("undefined");
}

int connectivity_state;
int connectivity_state_previous;

void connectivity_init() {

    connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;
    connectivity_state_previous = CONNECTIVITY_STATE_UNKNOWN;
/*
    Ethernet.setConfig(NetworkInterfaceConfig()
        .source(NetworkInterfaceConfigSource::STATIC)
        .address({192,168,1,137}, {255,255,255,0})
        .gateway({192,168,1,1})
        .dns({192,168,1,1})
        .dns({8,8,8,8}));
*/
/*
        Ethernet.setConfig(NetworkInterfaceConfig()
  .source(NetworkInterfaceConfigSource::STATIC)
  .address({192,168,1,137}, {255,255,255,0}).dns(particle::SockAddr {8,8,8,8})
  .dns({8,8,8,8}));
  */
}

int connectivity_get_state() {

    return(connectivity_state);
}

void connectivity_set_timeout(unsigned long time) {

    Log.info("timout set for %d",(int) time);

    unsigned long now = millis();

    now_delta = 0;
    timeout_delta = now + time;

    if(timeout_delta < now) {

        now_delta += time;
        timeout_delta += time;
    }
}

int connectivity_timeout() {

    if(timeout_delta < millis() + now_delta) {

        Log.info("timed out");

        return(1);
    }

    return(0);
}


bool connectivity_connect() {

    bool status = Particle.connected();

    if(!status) {

        switch(connectivity_state) {

            case CONNECTIVITY_CLOUD_CONNECTED:
            case CONNECTIVITY_ETHERNET_CONNECT:

                Log.info("Disconnecting WiFi, connecting Ethernet");

                WiFi.disconnect();
                // Ethernet.on();
                Ethernet.connect();

                uint8_t macAddr[6];
                Ethernet.macAddress(macAddr);

                // no ethernet adaptor will return nullptr
                if (macAddr != nullptr) {
                    Log.info("Ethernet MAC: %02x %02x %02x %02x %02x %02x",
                            macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
                }

                connectivity_set_timeout(TIMEOUT_ETHERNET_CONNECT);

                connectivity_state = CONNECTIVITY_ETHERNET_WAIT_CONNECTED;
                
                break;

            case CONNECTIVITY_ETHERNET_WAIT_CONNECTED:

                if(Ethernet.ready()) {

                    Log.info("Ethernet network connected");

                    Log.info("Ethernet IP address: %s", Ethernet.localIP().toString().c_str());
                    Log.info("Ethernet gateways IP address: %s",Ethernet.gatewayIP().toString().c_str());
                    Log.info("Ethernet DNS IP address: %s",Ethernet.dnsServerIP().toString().c_str());

                    connectivity_state = CONNECTIVITY_ETHERNET_CLOUD_WAIT_CONNECTED;
                    Particle.connect();

                    connectivity_set_timeout(TIMEOUT_CLOUD_CONNECT);

                } else if(connectivity_timeout()) {

                    Log.info("Ethernet network connecting takes too long");

                    connectivity_state = CONNECTIVITY_WIFI_CONNECT;
                }

                break;

            case CONNECTIVITY_ETHERNET_CLOUD_WAIT_CONNECTED:

                if(connectivity_timeout()) {

                    Log.info("Ethernet cloud connecting takes too long");

                    if(WiFi.hasCredentials()) {

                        connectivity_state = CONNECTIVITY_WIFI_CONNECT;

                    } else {

                        Log.info("No WiFi credentials: sticking with ethernet");

                        connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;
                    }
                }

                break;

            case CONNECTIVITY_WIFI_CONNECT:

                Log.info("Disconnecting Ethernet, connecting WiFi");
                Ethernet.disconnect();
                // WiFi.on();
                WiFi.connect();
                
                connectivity_set_timeout(TIMEOUT_WIFI_CONNECT);

                connectivity_state = CONNECTIVITY_WIFI_WAIT_CONNECTED;
                
                break;

            case CONNECTIVITY_WIFI_WAIT_CONNECTED:

                if(WiFi.ready()) {

                    Log.info("WiFi network connected");

                    Log.info("WiFi IP address: %s", WiFi.localIP().toString().c_str());
                    Log.info("WiFi gateways IP address: %s",WiFi.gatewayIP().toString().c_str());
                    Log.info("WiFi DNS IP address: %s",WiFi.dnsServerIP().toString().c_str());
                    
                    connectivity_state = CONNECTIVITY_WIFI_CLOUD_WAIT_CONNECTED;
                    Particle.connect();

                    connectivity_set_timeout(TIMEOUT_CLOUD_CONNECT);

                }   else if(connectivity_timeout()) {

                    Log.info("WiFi network connecting takes too long");

                    connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;
                }

                break;

            case CONNECTIVITY_WIFI_CLOUD_WAIT_CONNECTED:

                if(connectivity_timeout()) {

                    Log.info("Wifi cloud connecting takes too long");

                    connectivity_state = CONNECTIVITY_ETHERNET_CONNECT;
                }

                break;
        }

    } else {

        if(connectivity_state != CONNECTIVITY_CLOUD_CONNECTED) {

            Log.info("Cloud connected");
            connectivity_state = CONNECTIVITY_CLOUD_CONNECTED;
        }
    }

    if(connectivity_state_previous != connectivity_state) {

        Log.info("State: %s (previous: %s)",
            connectivity_state_name(connectivity_state).c_str(),
            connectivity_state_name(connectivity_state_previous).c_str());

        connectivity_state_previous = connectivity_state;
    }

    return(status);
}

/*

int connectivity_connect() {

    if(!Particle.connected()) {
    
        if(!connectivity_connect_ethernet()) {

            return(connectivity_connect_wifi());
        }
    }

    return(0);
}

int connectivity_connect_wifi() {

    unsigned long connect_ready;
    unsigned long connect_cloud_start;
    unsigned long connect_start = millis();

    Log.info("Disconnecting Ethernet, connecting WiFi");
    Ethernet.disconnect();
    WiFi.connect();
    
    Log.info("Waiting for Wifi ready");
    waitFor(WiFi.ready, 30000);

    if (WiFi.ready()) {

        connect_cloud_start = millis();

        Particle.connect();

        Log.info("Waiting for Particle connect");
        waitFor(Particle.connected, 30000);
    }

    if(Particle.connected()) {

        connect_ready = millis();

        Log.info("WiFi connected: %d (%d, %d)",
            (int)(connect_ready-connect_start),
            (int)(connect_cloud_start-connect_start),
            (int)(connect_ready-connect_cloud_start));

        return(1);
    }

    Log.info("WiFi not connected");

    return(0);
}


int connectivity_connect_ethernet() {

    unsigned long connect_ready;
    unsigned long connect_cloud_start;
    unsigned long connect_start = millis();

    Log.info("Disconnecting WiFi, connecting Ethernet");
    WiFi.disconnect();
    Ethernet.connect();

    // this should probably timeout sooner, whiching to wifi takes long now
    Log.info("Waiting for Ethernet ready");
    waitFor(Ethernet.ready, 30000);

    if (Ethernet.ready()) {

        connect_cloud_start = millis();

        Particle.keepAlive(25s);

        Particle.connect();

        Log.info("Waiting for Particle connect");
        waitFor(Particle.connected, 300000);
    }

    if(Particle.connected()) {

        connect_ready = millis();

        Log.info("Ethernet connected: %d (%d, %d)",
            (int)(connect_ready-connect_start),
            (int)(connect_cloud_start-connect_start),
            (int)(connect_ready-connect_cloud_start));

         return(1);
    }

    Log.info("Ethernet not connected");

    return(0);
}

void connectivity_connect_ethernet_old() {

if (Ethernet.isOn()) {

      uint8_t macAddrBuf[8] = {};
      uint8_t* macAddr = Ethernet.macAddress(macAddrBuf);

      // no ethernet adaptor will return nullptr
      if (macAddr != nullptr) {
          Log.info("Ethernet MAC: %02x %02x %02x %02x %02x %02x",
                  macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
      }

      Ethernet.connect();
      waitFor(Ethernet.ready, 30000);

    // Ethernet.ready() means connected to local network and got ip address
      Log.info("Ethernet.ready: %d", Ethernet.ready());
    
  } else {

      Log.info("Ethernet is off or not detected");
  }


  if(Ethernet.ready()) {

    Log.info("Ethernet is ready!");

  } else {

    Log.info("Ethernet is not ready");

    Ethernet.off();

    WiFi.on();

    // WiFi.connect();
    // waitFor(WiFi.ready, 30000);
  }
}
*/