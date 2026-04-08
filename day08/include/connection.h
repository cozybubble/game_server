#ifndef CONNECTION_H
#define CONNECTION_H
#include <string>
class Connection{
	public:
	Connection(int fd);
	void process();
	void sendMessage(const std::string& msg);

private:
	int fd_;
	std::string buffer_;
	int player_id_ = -1;

	void handleRead();
	void handleMessage(const std::string& msg);
};

#endif