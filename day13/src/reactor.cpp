#include "reactor.h"
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include "logger.h" 

Reactor::Reactor() : running_(false) {
    // 创建 epoll 实例
    // epoll_create1(0): 使用最新的 epoll 实现
    epoll_fd_ = ::epoll_create1(0);
    if (epoll_fd_ == -1) {
        LOG_ERROR << "epoll_create1 failed: " << strerror(errno);
        exit(1);  // epoll 创建失败是致命错误
    }
    LOG_DEBUG << "initialized, epoll_fd=" << epoll_fd_;
}

Reactor::~Reactor() {
    if (epoll_fd_ != -1) {
        ::close(epoll_fd_);
    }
}

void Reactor::addFd(int fd, uint32_t events, EventCallback cb){
	callbacks_[fd] = std::move(cb);

	// 注册到 epoll
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;  // 通过 fd 关联回调

    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        LOG_ERROR << "epoll_ctl ADD failed for fd=" << fd 
                  << ": " << strerror(errno); 
        return;
    }

    // 设置非阻塞模式！⭐ 关键步骤
    int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    LOG_DEBUG << "added fd=" << fd << ", events=" << events; 
}

void Reactor::modFd(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;

    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
        std::cerr << "[Reactor] epoll_ctl MOD failed for fd=" << fd 
                  << ": " << strerror(errno) << std::endl;
    }
}

void Reactor::removeFd(int fd) {
    // 从 epoll 移除
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    
    // 删除回调
    callbacks_.erase(fd);
    
    std::cout << "[Reactor] removed fd=" << fd << std::endl;
}

void Reactor::loop(int timeout_ms){
	running_ = true;
    std::cout << "[Reactor] event loop started..." << std::endl;

    // 就绪事件数组
    struct epoll_event ready_events[64];

    while(running_){
    		// 🔑 核心调用：等待事件
        int nready = ::epoll_wait(
            epoll_fd_,           // epoll 实例
            ready_events,        // 输出参数：就绪事件数组
            64,                  // 数组大小 (maxevents)
            timeout_ms           // 超时时间
        );

        if (nready == -1) {
            if (errno == EINTR) {
                // 被信号中断，继续循环
                continue;
            }
            std::cerr << "[Reactor] epoll_wait error: " 
                      << strerror(errno) << std::endl;
            break;
        }

        for(int i = 0; i < nready; ++i){
        	int fd = ready_events[i].data.fd;
        	uint32_t events = ready_events[i].events;

        	// 查找对应的回调
            auto it = callbacks_.find(fd);
            if (it != callbacks_.end()) {
                it->second(events);  // 执行回调
            } else {
                std::cerr << "[Reactor] no callback for fd=" << fd << std::endl;
            }
        }
    }
	    std::cout << "[Reactor] event loop stopped." << std::endl;
}

void Reactor::stop() {
    running_ = false;
}
