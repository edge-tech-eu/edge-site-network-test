#pragma once

#include "Particle.h"


class RemoteLogHandler : public LogHandler {
    String m_host;
    uint16_t m_port;
    String m_app;
    String m_system;
    UDP m_udp;
    bool m_inited;
    IPAddress m_address;
    

public:
    explicit RemoteLogHandler(String host, uint16_t port, String app, LogLevel level = LOG_LEVEL_INFO, 
                                    const LogCategoryFilters &filters = {}, String system = System.deviceID());
    virtual ~RemoteLogHandler();

private:

    bool lazyInit();
    const char* extractFileName(const char *s);
    const char* extractFuncName(const char *s, size_t *size);
    void log(String message);
    static IPAddress resolve(const char *host);
    static const uint16_t kLocalPort;

protected:
    virtual void logMessage(const char *msg, LogLevel level, const char *category, const LogAttributes &attr) override;
};
