#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <limits>  // 添加这个头文件

class InteractiveClient {
private:
    int fd_;
    std::atomic<bool> running_{true};
    std::thread recv_thread_;
    int player_id_ = -1;
    
    // 辅助函数：清空输入缓冲区
    void clearInputBuffer() {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    
    // 辅助函数：安全地读取整数
    int getIntInput() {
        int value;
        while (!(std::cin >> value)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "请输入有效的数字: ";
        }
        clearInputBuffer();  // 清除换行符
        return value;
    }
    
    // 辅助函数：安全地读取字符串
    std::string getStringInput() {
        std::string value;
        std::getline(std::cin, value);
        return value;
    }
    
    // 辅助函数：等待用户按回车
    void waitForEnter() {
        std::cout << "\n按回车继续...";
        std::cin.get();  // 这会消耗掉一个换行符
    }
    
public:
    InteractiveClient(int fd) : fd_(fd) {}
    
    ~InteractiveClient() {
        running_ = false;
        if (recv_thread_.joinable()) {
            recv_thread_.join();
        }
        close(fd_);
    }
    
    void sendMessage(const std::string& msg) {
        int len = msg.size();
        send(fd_, &len, 4, 0);
        send(fd_, msg.c_str(), len, 0);
        std::cout << "✓ 已发送: " << msg << std::endl;
    }
    
    void startReceive() {
        recv_thread_ = std::thread([this]() {
            while (running_) {
                int len = 0;
                int ret = recv(fd_, &len, 4, MSG_DONTWAIT);
                if (ret <= 0) {
                    if (ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                
                char buf[4096] = {0};
                int received = 0;
                while (received < len) {
                    int n = recv(fd_, buf + received, len - received, 0);
                    if (n <= 0) break;
                    received += n;
                }
                buf[received] = '\0';
                std::cout << "\n📨 [服务器消息] " << buf << std::endl;
                std::cout << "请选择操作: ";
                std::cout.flush();
            }
        });
    }
    
    void showMenu() {
        std::cout << "\n╔════════════════════════════════════╗\n";
        std::cout << "║       游戏客户端 - 主菜单          ║\n";
        std::cout << "╠════════════════════════════════════╣\n";
        std::cout << "║  1. 登录                          ║\n";
        std::cout << "║  2. 开始匹配                      ║\n";
        std::cout << "║  3. 发送房间消息                  ║\n";
        std::cout << "║  4. 离开房间                      ║\n";
        std::cout << "║  5. 查看状态                      ║\n";
        std::cout << "║  6. 发送自定义消息                ║\n";
        std::cout << "║  7. 测试特殊消息                  ║\n";
        std::cout << "║  0. 退出                         ║\n";
        std::cout << "╚════════════════════════════════════╝\n";
    }
    
    void showSpecialMenu() {
        std::cout << "\n┌─────────── 特殊消息测试 ───────────┐\n";
        std::cout << "│  a. 发送空消息                      │\n";
        std::cout << "│  b. 发送超长消息 (500字符)          │\n";
        std::cout << "│  c. 发送特殊字符消息                │\n";
        std::cout << "│  d. 发送格式错误的消息              │\n";
        std::cout << "│  e. 连续快速发送10条消息            │\n";
        std::cout << "│  f. 重复登录测试                    │\n";
        std::cout << "│  g. 返回主菜单                      │\n";
        std::cout << "└───────────────────────────────────┘\n";
        std::cout << "请选择: ";
    }
    
    void login() {
        if (player_id_ != -1) {
            std::cout << "⚠️  你已经登录了! (ID: " << player_id_ << ")" << std::endl;
            return;
        }
        
        std::cout << "请输入玩家ID (数字): ";
        int id = getIntInput();
        
        std::string msg = "login:" + std::to_string(id);
        sendMessage(msg);
        player_id_ = id;
    }
    
    void startMatch() {
        if (player_id_ == -1) {
            std::cout << "❌ 请先登录!" << std::endl;
            return;
        }
        sendMessage("match");
        std::cout << "⏳ 正在匹配中..." << std::endl;
    }
    
    void sendRoomMessage() {
        if (player_id_ == -1) {
            std::cout << "❌ 请先登录!" << std::endl;
            return;
        }
        
        std::cout << "请输入消息内容: ";
        std::string content = getStringInput();
        
        if (content.empty()) {
            std::cout << "⚠️  消息不能为空!" << std::endl;
            return;
        }
        
        std::string msg = "room_msg:" + content;
        sendMessage(msg);
    }
    
    void leaveRoom() {
        if (player_id_ == -1) {
            std::cout << "❌ 请先登录!" << std::endl;
            return;
        }
        sendMessage("leave_room");
    }
    
    void showStatus() {
        std::cout << "\n========== 当前状态 ==========" << std::endl;
        if (player_id_ == -1) {
            std::cout << "登录状态: ❌ 未登录" << std::endl;
        } else {
            std::cout << "登录状态: ✓ 已登录 (ID: " << player_id_ << ")" << std::endl;
        }
        std::cout << "连接状态: ✓ 已连接服务器" << std::endl;
        std::cout << "=============================\n" << std::endl;
    }
    
    void sendCustomMessage() {
        if (player_id_ == -1) {
            std::cout << "❌ 请先登录!" << std::endl;
            return;
        }
        
        std::cout << "请输入完整的消息内容 (例如: room_msg:hello): ";
        std::string msg = getStringInput();
        
        if (msg.empty()) {
            std::cout << "⚠️  消息不能为空!" << std::endl;
            return;
        }
        
        sendMessage(msg);
    }
    
    void testEmptyMessage() {
        if (player_id_ == -1) {
            std::cout << "❌ 请先登录!" << std::endl;
            return;
        }
        sendMessage("room_msg:");
        std::cout << "🧪 测试: 发送空消息" << std::endl;
    }
    
    void testLongMessage() {
        if (player_id_ == -1) {
            std::cout << "❌ 请先登录!" << std::endl;
            return;
        }
        std::string long_msg(500, 'A');
        sendMessage("room_msg:" + long_msg);
        std::cout << "🧪 测试: 发送500字符的超长消息" << std::endl;
    }
    
    void testSpecialChars() {
        if (player_id_ == -1) {
            std::cout << "❌ 请先登录!" << std::endl;
            return;
        }
        std::string special = "room_msg:特殊字符测试: !@#$%^&*()_+{}|:<>?~`";
        sendMessage(special);
        std::cout << "🧪 测试: 发送特殊字符消息" << std::endl;
    }
    
    void testMalformedMessage() {
        if (player_id_ == -1) {
            std::cout << "❌ 请先登录!" << std::endl;
            return;
        }
        std::cout << "选择错误格式:\n";
        std::cout << "  1. 缺少冒号 (room_msg hello)\n";
        std::cout << "  2. 错误前缀 (chat:hello)\n";
        std::cout << "  3. 空命令 (空字符串)\n";
        std::cout << "请选择: ";
        int choice = getIntInput();
        
        switch(choice) {
            case 1:
                sendMessage("room_msg hello");
                break;
            case 2:
                sendMessage("chat:hello");
                break;
            case 3:
                sendMessage("");
                break;
            default:
                std::cout << "无效选择" << std::endl;
        }
        std::cout << "🧪 测试: 发送格式错误的消息" << std::endl;
    }
    
    void testRapidMessages() {
        if (player_id_ == -1) {
            std::cout << "❌ 请先登录!" << std::endl;
            return;
        }
        std::cout << "开始连续发送10条消息..." << std::endl;
        for (int i = 1; i <= 10; i++) {
            std::string msg = "room_msg:Rapid message " + std::to_string(i);
            sendMessage(msg);
            usleep(100000); // 间隔0.1秒
        }
        std::cout << "🧪 测试: 已发送10条连续消息" << std::endl;
    }
    
    void testDuplicateLogin() {
        if (player_id_ == -1) {
            std::cout << "❌ 请先登录!" << std::endl;
            return;
        }
        std::string msg = "login:" + std::to_string(player_id_);
        sendMessage(msg);
        std::cout << "🧪 测试: 重复登录 (应该返回 already_login)" << std::endl;
    }
    
    void run() {
        startReceive();
        
        while (running_) {
            showMenu();
            std::cout << "请选择操作: ";
            int choice = getIntInput();
            
            switch(choice) {
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
                    sendCustomMessage();
                    break;
                case 7:
                    {
                        bool back = false;
                        while (!back) {
                            showSpecialMenu();
                            char special_choice;
                            std::cin >> special_choice;
                            clearInputBuffer();  // 清除换行符
                            
                            switch(special_choice) {
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
                                    testMalformedMessage();
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
                                    std::cout << "无效选择!" << std::endl;
                            }
                            
                            if (!back) {
                                waitForEnter();
                            }
                        }
                    }
                    break;
                case 0:
                    std::cout << "退出客户端..." << std::endl;
                    running_ = false;
                    break;
                default:
                    std::cout << "无效选择，请重新输入!" << std::endl;
                    waitForEnter();
            }
        }
    }
};

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "❌ 创建socket失败!" << std::endl;
        return 1;
    }
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "❌ 连接服务器失败! 请确保服务器已启动" << std::endl;
        return 1;
    }
    
    std::cout << "\n🎮 已连接到游戏服务器!" << std::endl;
    
    InteractiveClient client(fd);
    client.run();
    
    return 0;
}