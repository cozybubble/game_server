#ifndef CONNECTION_H
#define CONNECTION_H

class Connection{
	public:
	Connection(int fd);
	void process();

private:
	int fd_;

	void handleRead();
	void handleWrite(const char* data, int len);
};

#endif