#ifndef SERVER_H
#define SERVER_H

#include "reactor.h"


class Server {
public:
    Server(int port);
    ~Server(); 
    void start();
    void stop();

private:
    int port_;
    int listen_fd_;
    Reactor reactor_;

    void initSocket();

    void handleAccept(uint32_t events);
};

#endif