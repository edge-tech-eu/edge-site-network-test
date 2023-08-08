#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#define TIMEOUT_WIFI_CONNECT         60000
#define TIMEOUT_ETHERNET_CONNECT     60000
#define TIMEOUT_CLOUD_CONNECT       300000

    bool connectivity_connect(void);
    void connectivity_init(void);
    int connectivity_get_state(void);
    String connectivity_state_name(int state);

#endif