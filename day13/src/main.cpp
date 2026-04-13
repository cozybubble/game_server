#include "server.h"
#include "logger.h"

int main() {
    initLogger(LogLevel::DEBUG, "game_server.log");

    LOG_INFO << "========== Game Server Starting ==========";

    Server server(8888);
    server.start();

    LOG_INFO << "========== Game Server Stopped ==========";
    return 0;
}
