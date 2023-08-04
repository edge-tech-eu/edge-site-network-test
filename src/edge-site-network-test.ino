#include "Particle.h"
#include "Connectivity.h"

STARTUP(BLE.selectAntenna(BleAntennaType::EXTERNAL));

// does not startup connectivity only after Particle.connect() is called
SYSTEM_MODE(SEMI_AUTOMATIC); 
SYSTEM_THREAD(ENABLED);

STARTUP(System.enableFeature(FEATURE_DISABLE_LISTENING_MODE));


// serial logger on serial2
Serial2LogHandler logHandler(115200, LOG_LEVEL_ALL);


unsigned long next_time;

void setup() {

  // just to test:
  WiFi.clearCredentials();
  // WiFi.setCredentials("qq","***REMOVED***");
  // WiFi.setCredentials("external-edge-fs","123EVbest!");

  // publish communication information to the particle cloude every 30s

  Particle.publishVitals(30); 

  if (!System.featureEnabled(FEATURE_ETHERNET_DETECTION)) {

    Log.info("Enabling Ethernet...");
    System.enableFeature(FEATURE_ETHERNET_DETECTION);

    if_wiznet_pin_remap remap = {};
    remap.base.type = IF_WIZNET_DRIVER_SPECIFIC_PIN_REMAP;

    remap.cs_pin = D1;
    remap.reset_pin = D3;
    remap.int_pin = D0;

    auto ret = if_request(nullptr, IF_REQ_DRIVER_SPECIFIC, &remap, sizeof(remap), nullptr);

    if (ret != SYSTEM_ERROR_NONE) {

        Log.error("Ethernet GPIO config error: %d", ret);

    } else {

        delay(500);
        System.reset();
    }
  }

  connectivity_init();

  next_time = millis() + 1000;
}


void loop() {

  unsigned long now = millis();

  connectivity_connect();

  if(next_time < now) {

    while(next_time < now) {
      next_time += 1000;
    }

    Log.info("State: %s: Ethernet: %s, WiFi: %s, Cloud: %s",
      connectivity_state_name(connectivity_get_state()).c_str(),
      (Ethernet.ready()?"ready":"not ready"),
      (WiFi.ready()?"ready":"not ready"),
      (Particle.connected()?"connected":"not connected"));
  }

  delay(100);
}