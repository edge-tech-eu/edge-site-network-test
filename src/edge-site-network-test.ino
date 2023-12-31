#include "Particle.h"
#include "Connectivity.h"
#include "ResetReason.h"
#include "Util.h"

#define ETHERNET_BOARD_PARTICLE 0
#define ETHERNET_BOARD_EDGE     1
#define ETHERNET_BOARD_ARGON    2

// #define ETHERNET_BOARD          ETHERNET_BOARD_PARTICLE
#define ETHERNET_BOARD          ETHERNET_BOARD_EDGE
// #define ETHERNET_BOARD          ETHERNET_BOARD_ARGON


STARTUP(System.enableFeature(FEATURE_RESET_INFO));
STARTUP(BLE.selectAntenna(BleAntennaType::EXTERNAL));

// does not startup connectivity only after Particle.connect() is called
SYSTEM_MODE(SEMI_AUTOMATIC); 
SYSTEM_THREAD(ENABLED);

STARTUP(System.enableFeature(FEATURE_DISABLE_LISTENING_MODE));


// serial logger on serial2
#if ETHERNET_BOARD == ETHERNET_BOARD_PARTICLE
Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL);
#endif
#if ETHERNET_BOARD == ETHERNET_BOARD_EDGE
Serial2LogHandler logHandler(115200, LOG_LEVEL_ALL);
#endif
#if ETHERNET_BOARD == ETHERNET_BOARD_ARGON
SerialLogHandler logHandler(115200, LOG_LEVEL_ALL);
#endif

unsigned long last_time;
int print_state;

void setup() {

  Watchdog.init(WatchdogConfiguration().timeout(60s));
  Watchdog.start();

  // just to test ethernet: no credentials is not use wifi
  WiFi.clearCredentials();
  
  reset_reason_log();

  Log.info("%s-%s-%s", completeVersion, System.version().c_str(), System.deviceID().c_str());

  // publish communication information to the particle cloude every 30s
  Particle.publishVitals(30); 

  // how do we turn ethernet off?
  // System.disableFeature(FEATURE_ETHERNET_DETECTION);
  // Ethernet.off();

  if (!System.featureEnabled(FEATURE_ETHERNET_DETECTION)) {

    Log.info("Enabling Ethernet...");
    System.enableFeature(FEATURE_ETHERNET_DETECTION);


    if_wiznet_pin_remap remap = {};
    remap.base.type = IF_WIZNET_DRIVER_SPECIFIC_PIN_REMAP;

#if ETHERNET_BOARD == ETHERNET_BOARD_EDGE
    remap.cs_pin = D1;
    remap.reset_pin = D3;
    remap.int_pin = D0;
#endif
#if ETHERNET_BOARD == ETHERNET_BOARD_PARTICLE
    remap.cs_pin = PIN_INVALID;
    remap.reset_pin = PIN_INVALID;
    remap.int_pin = PIN_INVALID;
#endif

    auto ret = if_request(nullptr, IF_REQ_DRIVER_SPECIFIC, &remap, sizeof(remap), nullptr);

    if (ret != SYSTEM_ERROR_NONE) {

        Log.error("Ethernet GPIO config error: %d", ret);

    } else {

        delay(500);
        System.reset();
    }
  }

  Particle.disconnect(CloudDisconnectOptions().clearSession(true));

  connectivity_init();

  print_state = -1;

  last_time = millis();
}


void loop() {

  Watchdog.refresh();

  connectivity_connect();

  if((millis()-last_time) >  1000L) {

    Log.info("1s loop check");

    last_time = millis();
  }


  bool ethReady = Ethernet.ready();
  bool wifiReady = WiFi.ready();
  bool cloudConnected = Particle.connected();
  int connectingState = connectivity_get_state();

  int new_print_state = connectingState+(ethReady?32:0)+(wifiReady?64:0)+(cloudConnected?128:0);

  if(new_print_state != print_state) {

    last_time = millis();

    Log.info("State: %s: Ethernet: %s, WiFi: %s, Cloud: %s",
      connectivity_state_name(connectingState).c_str(),
      (ethReady?"ready":"not ready"),
      (wifiReady?"ready":"not ready"),
      (cloudConnected?"connected":"not connected"));

    print_state = new_print_state;

    // testing: once connected restart and do again
    if(cloudConnected) {

      Log.info("Connected, restart"); 
      delay(1000);
      System.reset();
    }
  }
}