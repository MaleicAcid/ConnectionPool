#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <iostream>
#include <atomic>
#include <memory>
#include <thread>
#include <condition_variable>
#include <functional>
#include "Connection.h"
using namespace std;

class ConnectionPool {
public:
	static ConnectionPool* getConnectionPool();// #1 提供静态接口以供获取实例
	
	// 给外部提供的获取conn接口
	// 智能指针析构时只归还不销毁
	shared_ptr<Connection> getConnection();
private:
	ConnectionPool(); // #1 构造函数私有化

	bool loadConfigFile(); // 加载配置

	// 运行在独立的线程中
	// 线程函数写成类的成员方法好处是方便访问对象的成员变量
	// 但this指针所需要的对象用绑定器绑定
	void produceConnectionTask();

	// 扫描连接的线程函数
	void scannerConnectionTask();
	string _ip;
	unsigned short _port;
	string _username;
	string _password;
	string _dbname;
	int _initSize; // 初始连接数
	int _maxSize; // 最大连接数
	int _maxIdleTime; // 连接最大空间时间
	int _connectionTimeout; // 阻塞超时时间

	queue<Connection*> _connectionQue; // 存储连接的队列, 需加锁
	mutex _queueMutex;
	atomic_int _connectionCnt; //创建的连接的总数量

	// 条件变量, 用于消费者线程和生产者线程的通信
	condition_variable cv;
};