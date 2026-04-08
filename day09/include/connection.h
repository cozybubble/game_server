#ifndef CONNECTION_H
#define CONNECTION_H
#include <string>
#include <memory>
#include<mutex>
class Connection : public std::enable_shared_from_this<Connection> {
	public:
	Connection(int fd);
	void process();
	void sendMessage(const std::string& msg);

private:
	int fd_;
	std::string buffer_;
	int player_id_ = -1;
	std::mutex send_mutex_;

	void handleRead();
	void handleMessage(const std::string& msg);
};

#endif