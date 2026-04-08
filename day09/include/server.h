#ifndef SERVER_H
#define SERVER_H

class Server {
public:
    Server(int port);
    void start();

private:
    int port_;
    int listen_fd_;

    void initSocket();

    void handleClient(int client_fd);
};

#endif