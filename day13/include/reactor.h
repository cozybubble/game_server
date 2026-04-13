#ifndef REACTOR_H
#define REACTOR_H

#include <sys/epoll.h>
#include <unordered_map>
#include <functional>
#include <memory>
#include <iostream>

// 前向声明
class Connection;

// 事件类型
enum class EventType {
    READ  = EPOLLIN,     // 可读事件
    WRITE = EPOLLOUT,    // 可写事件
    ERROR = EPOLLERR,    // 错误事件
};

// 事件处理器接口（回调）
using EventCallback = std::function<void(uint32_t events)>;

/**
 * Reactor 类 — 事件驱动核心
 * 
 * 职责：
 * 1. 管理 epoll 实例
 * 2. 注册/注销文件描述符
 * 3. 运行事件循环 (Event Loop)
 */
 class Reactor{
public:
	Reactor();
    ~Reactor();

    // 禁止拷贝
    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;

    /**
     * 注册 fd 到 epoll
     * @param fd      文件描述符
     * @param events  关注的事件类型 (EPOLLIN | EPOLLOUT)
     * @param cb      事件触发时的回调函数
     */
    void addFd(int fd, uint32_t events, EventCallback cb);

    /**
     * 修改 fd 已注册的事件
     */
    void modFd(int fd, uint32_t events);

    /**
     * 从 epoll 移除 fd
     */
    void removeFd(int fd);

    /**
     * 启动事件循环（阻塞）
     * @param timeout_ms  超时时间(毫秒)，-1 表示无限等待
     */
    void loop(int timeout_ms = -1);

    /**
     * 停止事件循环
     */
    void stop();

    /**
     * 获取 epoll 文件描述符（用于调试）
     */
    int getEpollFd() const { return epoll_fd_; }
    
private:
    int epoll_fd_;                                    // epoll 实例的 fd
    std::unordered_map<int, EventCallback> callbacks_; // fd → 回调函数 映射
    bool running_;                                    // 是否在运行
 };

 #endif