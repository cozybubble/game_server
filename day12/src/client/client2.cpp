#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>

void sendMessage(int fd, const std::string& msg) {
    int len = msg.size();
    send(fd, &len, 4, 0);
    send(fd, msg.c_str(), len, 0);
    std::cout << "Sent: " << msg << std::endl;
}

std::string receiveMessage(int fd) {
    int len = 0;
    if (recv(fd, &len, 4, 0) <= 0) return "";
    
    char buf[4096] = {0};
    int received = 0;
    while (received < len) {
        int n = recv(fd, buf + received, len - received, 0);
        if (n <= 0) return "";
        received += n;
    }
    buf[received] = '\0';
    return std::string(buf);
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return 1;
    }

    std::cout << "=== Client 2 Started ===" << std::endl;
    std::cout << "Testing multi-player interactions...\n" << std::endl;

    // 测试1: Login with different ID
    std::cout << "Test 1: Login" << std::endl;
    sendMessage(fd, "login:1002");
    std::string reply = receiveMessage(fd);
    std::cout << "Reply: " << reply << std::endl << std::endl;

    // 测试2: Match making (should match with client1)
    std::cout << "Test 2: Start Match (should match with client1)" << std::endl;
    sendMessage(fd, "match");
    reply = receiveMessage(fd);
    std::cout << "Reply: " << reply << std::endl << std::endl;

    // 等待匹配完成
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 测试3: Send multiple room messages
    for (int i = 1; i <= 3; i++) {
        std::cout << "Test 3." << i << ": Send Room Message" << std::endl;
        std::string msg = "room_msg:Message " + std::to_string(i) + " from client2";
        sendMessage(fd, msg);
        // 等待广播消息
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << std::endl;
    }

    // 测试4: Try to leave and re-join
    std::cout << "Test 4: Leave Room" << std::endl;
    sendMessage(fd, "leave_room");
    reply = receiveMessage(fd);
    std::cout << "Reply: " << reply << std::endl << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 测试5: Try to match again
    std::cout << "Test 5: Re-match after leaving" << std::endl;
    sendMessage(fd, "match");
    reply = receiveMessage(fd);
    std::cout << "Reply: " << reply << std::endl << std::endl;

    std::cout << "=== Client 2 Test Complete ===" << std::endl;
    
    close(fd);
    return 0;
}