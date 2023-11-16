#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#define TIMEOUT_WIFI_CONNECT         60000
#define TIMEOUT_ETHERNET_CONNECT      5000
#define TIMEOUT_CLOUD_CONNECT       180000

#define CONNECTIVITY_STATE_UNKNOWN                (-1)
#define CONNECTIVITY_ETHERNET_CONNECT               0
#define CONNECTIVITY_ETHERNET_WAIT_CONNECTED        1
#define CONNECTIVITY_ETHERNET_CLOUD_WAIT_CONNECTED  2
#define CONNECTIVITY_ETHERNET_CLOUD_CONNECTED       3
#define CONNECTIVITY_WIFI_CONNECT                   4
#define CONNECTIVITY_WIFI_WAIT_CONNECTED            5
#define CONNECTIVITY_WIFI_CLOUD_WAIT_CONNECTED      6
#define CONNECTIVITY_WIFI_CLOUD_CONNECTED           7

    bool connectivity_connect(void);
    void connectivity_init(void);
    int connectivity_get_state(void);
    String connectivity_state_name(int state);
    void connectivity_dump_stats(void);
    bool connectivity_first_connected(void);

#endif