#include "Particle.h"
#include "Connectivity.h"
#include "Led.h"
#include "ResetReason.h"
#include "SmartMeter.h"
#include "RemoteLogger.h"

STARTUP(BLE.selectAntenna(BleAntennaType::EXTERNAL));

// does not startup connectivity only after Particle.connect() is called
SYSTEM_MODE(SEMI_AUTOMATIC); 
SYSTEM_THREAD(ENABLED);

STARTUP(System.on(network_status, led_handle_network_events));
STARTUP(System.on(cloud_status, led_handle_cloud_events));

STARTUP(System.enableFeature(FEATURE_DISABLE_LISTENING_MODE));


// serial logger on serial2
Serial2LogHandler logHandler(115200, LOG_LEVEL_ALL);
RemoteLogHandler remoteLogHandler("monitor.edgetech.nl", 8888, "network-test");
// walter: 0a10aced202194944a004b38

unsigned long next_time;

void setup() {

  Watchdog.init(WatchdogConfiguration().timeout(60s));
  Watchdog.start();

  // just to test:
  WiFi.clearCredentials();
  // WiFi.setCredentials("qq","***REMOVED***");
  // WiFi.setCredentials("H369A59AFB1","79DDEE7963A3");
  // WiFi.setCredentials("external-edge-fs","123EVbest!");

  led_init();

  reset_reason_log();

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

  smart_meter_init();

  smart_meter_identify();

  if(!smart_meter_is_usable()) {

    Log.error("No meter!");
  }

  next_time = millis() + 1000;
}


void loop() {

  unsigned long now = millis();

  Watchdog.refresh();

  if(smart_meter_is_usable()) {
    smart_meter_process_serial();
  }

  connectivity_connect();

  if(next_time < now) {

    while(next_time < now) {
      next_time += 1000;
    }

    if(Particle.connected()) {
        next_time += 30000 - 1000;
    }
    
    Log.info("State: %s: Ethernet: %s, WiFi: %s, Cloud: %s",
      connectivity_state_name(connectivity_get_state()).c_str(),
      (Ethernet.ready()?"ready":"not ready"),
      (WiFi.ready()?"ready":"not ready"),
      (Particle.connected()?"connected":"not connected"));
  }
}