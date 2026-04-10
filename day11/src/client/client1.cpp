#include <arpa/inet.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <limits>
#include <mutex>
#include <string>
#include <thread>

#include "message.pb.h"
#include "packet.pb.h"

class InteractiveClient {
public:
    explicit InteractiveClient(int fd) : fd_(fd) {}

    ~InteractiveClient() {
        running_ = false;
        ::shutdown(fd_, SHUT_RDWR);
        if (recv_thread_.joinable()) {
            recv_thread_.join();
        }
        ::close(fd_);
    }

    void run() {
        startReceive();

        while (running_) {
            showMenu();
            std::cout << "Select: ";
            int choice = getIntInput();

            switch (choice) {
                case 1:
                    login();
                    break;
                case 2:
                    startMatch();
                    break;
                case 3:
                    sendRoomMessage();
                    break;
                case 4:
                    leaveRoom();
                    break;
                case 5:
                    showStatus();
                    break;
                case 6:
                    sendCustomChat();
                    break;
                case 7:
                    runSpecialTests();
                    break;
                case 0:
                    running_ = false;
                    break;
                default:
                    std::cout << "Invalid choice" << std::endl;
                    waitForEnter();
                    break;
            }
        }
    }

private:
    int fd_;
    std::atomic<bool> running_{true};
    std::thread recv_thread_;
    std::mutex io_mutex_;
    int player_id_ = -1;
    int room_id_ = -1;

    bool sendAll(const void* data, size_t len) {
        const char* ptr = static_cast<const char*>(data);
        size_t sent = 0;
        while (sent < len) {
            int n = ::send(fd_, ptr + sent, len - sent, 0);
            if (n <= 0) {
                return false;
            }
            sent += static_cast<size_t>(n);
        }
        return true;
    }

    bool recvAll(void* data, size_t len) {
        char* ptr = static_cast<char*>(data);
        size_t received = 0;
        while (received < len) {
            int n = ::recv(fd_, ptr + received, len - received, 0);
            if (n <= 0) {
                return false;
            }
            received += static_cast<size_t>(n);
        }
        return true;
    }

    bool sendPacket(const game::Packet& packet) {
        std::string body;
        if (!packet.SerializeToString(&body)) {
            std::lock_guard<std::mutex> lock(io_mutex_);
            std::cout << "serialize packet failed" << std::endl;
            return false;
        }

        int len = static_cast<int>(body.size());
        if (!sendAll(&len, sizeof(len)) || !sendAll(body.data(), body.size())) {
            std::lock_guard<std::mutex> lock(io_mutex_);
            std::cout << "send failed" << std::endl;
            return false;
        }
        return true;
    }

    template <typename MessageT>
    bool sendMessage(game::Command cmd, const MessageT& msg, const std::string& name) {
        game::Packet packet;
        packet.set_cmd(cmd);
        if (!msg.SerializeToString(packet.mutable_payload())) {
            std::lock_guard<std::mutex> lock(io_mutex_);
            std::cout << "serialize payload failed" << std::endl;
            return false;
        }

        if (!sendPacket(packet)) {
            return false;
        }

        std::lock_guard<std::mutex> lock(io_mutex_);
        std::cout << "Sent " << name << std::endl;
        return true;
    }

    bool recvPacket(game::Packet& packet) {
        int len = 0;
        if (!recvAll(&len, sizeof(len))) {
            return false;
        }
        if (len <= 0) {
            return false;
        }

        std::string body(len, '\0');
        if (!recvAll(body.data(), body.size())) {
            return false;
        }
        return packet.ParseFromString(body);
    }

    void startReceive() {
        recv_thread_ = std::thread([this]() {
            while (running_) {
                game::Packet packet;
                if (!recvPacket(packet)) {
                    if (running_) {
                        std::lock_guard<std::mutex> lock(io_mutex_);
                        std::cout << "\nConnection closed by server" << std::endl;
                    }
                    running_ = false;
                    break;
                }
                handlePacket(packet);
            }
        });
    }

    void handlePacket(const game::Packet& packet) {
        std::lock_guard<std::mutex> lock(io_mutex_);
        std::cout << "\n[Server] ";

        switch (packet.cmd()) {
            case game::CMD_LOGIN_RSP: {
                game::LoginRsp rsp;
                if (!rsp.ParseFromString(packet.payload())) {
                    std::cout << "bad LoginRsp";
                    break;
                }
                if (rsp.success()) {
                    player_id_ = rsp.player_id();
                }
                std::cout << "login success=" << rsp.success()
                          << ", player_id=" << rsp.player_id()
                          << ", message=" << rsp.message();
                break;
            }
            case game::CMD_MATCH_RSP: {
                game::MatchRsp rsp;
                if (!rsp.ParseFromString(packet.payload())) {
                    std::cout << "bad MatchRsp";
                    break;
                }
                if (rsp.success() && rsp.room_id() != 0) {
                    room_id_ = rsp.room_id();
                }
                std::cout << "match success=" << rsp.success()
                          << ", room_id=" << rsp.room_id()
                          << ", message=" << rsp.message();
                break;
            }
            case game::CMD_ROOM_ENTER_NTF: {
                game::RoomEnterNtf ntf;
                if (!ntf.ParseFromString(packet.payload())) {
                    std::cout << "bad RoomEnterNtf";
                    break;
                }
                room_id_ = ntf.room_id();
                std::cout << "entered room " << ntf.room_id() << ", players=";
                for (int i = 0; i < ntf.player_ids_size(); ++i) {
                    if (i > 0) {
                        std::cout << ",";
                    }
                    std::cout << ntf.player_ids(i);
                }
                break;
            }
            case game::CMD_ROOM_CHAT_NTF: {
                game::RoomChatNtf ntf;
                if (!ntf.ParseFromString(packet.payload())) {
                    std::cout << "bad RoomChatNtf";
                    break;
                }
                std::cout << "chat from " << ntf.player_id() << ": " << ntf.content();
                break;
            }
            case game::CMD_LEAVE_ROOM_RSP: {
                game::LeaveRoomRsp rsp;
                if (!rsp.ParseFromString(packet.payload())) {
                    std::cout << "bad LeaveRoomRsp";
                    break;
                }
                if (rsp.success()) {
                    room_id_ = -1;
                }
                std::cout << "leave success=" << rsp.success()
                          << ", message=" << rsp.message();
                break;
            }
            case game::CMD_ERROR_RSP: {
                game::ErrorRsp rsp;
                if (!rsp.ParseFromString(packet.payload())) {
                    std::cout << "bad ErrorRsp";
                    break;
                }
                std::cout << "error code=" << rsp.code()
                          << ", message=" << rsp.message();
                break;
            }
            default:
                std::cout << "cmd=" << static_cast<int>(packet.cmd())
                          << ", payload_size=" << packet.payload().size();
                break;
        }

        std::cout << std::endl;
        if (running_) {
            std::cout << "Select: ";
            std::cout.flush();
        }
    }

    void showMenu() {
        std::lock_guard<std::mutex> lock(io_mutex_);
        std::cout << "\n========== Client Menu ==========" << std::endl;
        std::cout << "1. Login" << std::endl;
        std::cout << "2. Start Match" << std::endl;
        std::cout << "3. Send Room Message" << std::endl;
        std::cout << "4. Leave Room" << std::endl;
        std::cout << "5. Show Status" << std::endl;
        std::cout << "6. Send Custom Chat Content" << std::endl;
        std::cout << "7. Run Special Tests" << std::endl;
        std::cout << "0. Exit" << std::endl;
    }

    void showSpecialMenu() {
        std::lock_guard<std::mutex> lock(io_mutex_);
        std::cout << "\n------ Special Tests ------" << std::endl;
        std::cout << "a. Send empty room chat" << std::endl;
        std::cout << "b. Send long room chat (500 chars)" << std::endl;
        std::cout << "c. Send special characters" << std::endl;
        std::cout << "d. Send malformed packet" << std::endl;
        std::cout << "e. Send 10 rapid room chats" << std::endl;
        std::cout << "f. Duplicate login" << std::endl;
        std::cout << "g. Back" << std::endl;
        std::cout << "Select: ";
    }

    void clearInputBuffer() {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    int getIntInput() {
        int value = 0;
        while (!(std::cin >> value)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "Please input a number: ";
        }
        clearInputBuffer();
        return value;
    }

    std::string getStringInput() {
        std::string value;
        std::getline(std::cin, value);
        return value;
    }

    void waitForEnter() {
        std::cout << "Press Enter to continue...";
        std::cin.get();
    }

    void login() {
        if (player_id_ != -1) {
            std::lock_guard<std::mutex> lock(io_mutex_);
            std::cout << "Already logged in as player " << player_id_ << std::endl;
            return;
        }

        std::cout << "Player ID: ";
        int id = getIntInput();

        game::LoginReq req;
        req.set_player_id(id);
        sendMessage(game::CMD_LOGIN_REQ, req, "LoginReq");
    }

    void startMatch() {
        if (player_id_ == -1) {
            std::lock_guard<std::mutex> lock(io_mutex_);
            std::cout << "Please login first" << std::endl;
            return;
        }

        game::MatchReq req;
        sendMessage(game::CMD_MATCH_REQ, req, "MatchReq");
    }

    void sendRoomMessage() {
        if (player_id_ == -1) {
            std::lock_guard<std::mutex> lock(io_mutex_);
            std::cout << "Please login first" << std::endl;
            return;
        }

        std::cout << "Room message: ";
        std::string content = getStringInput();
        if (content.empty()) {
            std::lock_guard<std::mutex> lock(io_mutex_);
            std::cout << "Message cannot be empty" << std::endl;
            return;
        }

        game::RoomChatReq req;
        req.set_content(content);
        sendMessage(game::CMD_ROOM_CHAT_REQ, req, "RoomChatReq");
    }

    void leaveRoom() {
        if (player_id_ == -1) {
            std::lock_guard<std::mutex> lock(io_mutex_);
            std::cout << "Please login first" << std::endl;
            return;
        }

        game::LeaveRoomReq req;
        sendMessage(game::CMD_LEAVE_ROOM_REQ, req, "LeaveRoomReq");
    }

    void showStatus() {
        std::lock_guard<std::mutex> lock(io_mutex_);
        std::cout << "\n========== Status ==========" << std::endl;
        std::cout << "running: " << (running_ ? "yes" : "no") << std::endl;
        std::cout << "player_id: " << player_id_ << std::endl;
        std::cout << "room_id: " << room_id_ << std::endl;
        std::cout << "============================" << std::endl;
    }

    void sendCustomChat() {
        if (player_id_ == -1) {
            std::lock_guard<std::mutex> lock(io_mutex_);
            std::cout << "Please login first" << std::endl;
            return;
        }

        std::cout << "Custom chat content: ";
        std::string content = getStringInput();
        game::RoomChatReq req;
        req.set_content(content);
        sendMessage(game::CMD_ROOM_CHAT_REQ, req, "RoomChatReq(custom)");
    }

    void testEmptyMessage() {
        game::RoomChatReq req;
        req.set_content("");
        sendMessage(game::CMD_ROOM_CHAT_REQ, req, "Empty RoomChatReq");
    }

    void testLongMessage() {
        game::RoomChatReq req;
        req.set_content(std::string(500, 'A'));
        sendMessage(game::CMD_ROOM_CHAT_REQ, req, "Long RoomChatReq");
    }

    void testSpecialChars() {
        game::RoomChatReq req;
        req.set_content("special chars: !@#$%^&*()_+{}|:<>?~`");
        sendMessage(game::CMD_ROOM_CHAT_REQ, req, "SpecialChars RoomChatReq");
    }

    void testMalformedPacket() {
        std::string bad_body = "not-a-protobuf-packet";
        int len = static_cast<int>(bad_body.size());
        if (sendAll(&len, sizeof(len)) && sendAll(bad_body.data(), bad_body.size())) {
            std::lock_guard<std::mutex> lock(io_mutex_);
            std::cout << "Sent malformed packet" << std::endl;
        }
    }

    void testRapidMessages() {
        for (int i = 1; i <= 10; ++i) {
            game::RoomChatReq req;
            req.set_content("Rapid message " + std::to_string(i));
            sendMessage(game::CMD_ROOM_CHAT_REQ, req, "Rapid RoomChatReq");
            ::usleep(100000);
        }
    }

    void testDuplicateLogin() {
        if (player_id_ == -1) {
            std::lock_guard<std::mutex> lock(io_mutex_);
            std::cout << "Please login first" << std::endl;
            return;
        }

        game::LoginReq req;
        req.set_player_id(player_id_);
        sendMessage(game::CMD_LOGIN_REQ, req, "Duplicate LoginReq");
    }

    void runSpecialTests() {
        bool back = false;
        while (!back && running_) {
            showSpecialMenu();
            char choice = '\0';
            std::cin >> choice;
            clearInputBuffer();

            switch (choice) {
                case 'a':
                    testEmptyMessage();
                    break;
                case 'b':
                    testLongMessage();
                    break;
                case 'c':
                    testSpecialChars();
                    break;
                case 'd':
                    testMalformedPacket();
                    break;
                case 'e':
                    testRapidMessages();
                    break;
                case 'f':
                    testDuplicateLogin();
                    break;
                case 'g':
                    back = true;
                    break;
                default:
                    std::lock_guard<std::mutex> lock(io_mutex_);
                    std::cout << "Invalid choice" << std::endl;
                    break;
            }

            if (!back) {
                waitForEnter();
            }
        }
    }
};

int main() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket creation failed" << std::endl;
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "connect failed, make sure server is running" << std::endl;
        ::close(fd);
        return 1;
    }

    std::cout << "Connected to server" << std::endl;
    InteractiveClient client(fd);
    client.run();
    return 0;
}
