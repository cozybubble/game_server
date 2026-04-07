#ifndef CONNECTION_H
#define CONNECTION_H
#include <string>
class Connection{
	public:
	Connection(int fd);
	void process();

private:
	int fd_;
	std::string buffer_;

	void handleRead();
	void handleMessage(const std::string& msg);
};

#endif