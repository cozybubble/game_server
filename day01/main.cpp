#include "server.h"

int main() {
    Server server(8888);
    server.start();
    return 0;
}