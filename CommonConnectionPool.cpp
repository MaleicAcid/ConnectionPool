#include "CommonConnectionPool.h"
#include "public.h"
#include <unordered_map>
#include <string>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>

using namespace std;
ConnectionPool* ConnectionPool::getConnectionPool() {
	// 运行到才初始化, 所以是懒汉
	static ConnectionPool pool; // 静态对象的初始化编译器自动lock和unlock
	return &pool;
}

bool ConnectionPool::loadConfigFile() {
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr) {
		LOG("mysql.ini open failed");
		return false;
	}
	unordered_map<string, string> map = {};
	while (!feof(pf)) {
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		if (str[0] == '#') continue;
		int idx = str.find('=', 0);

		if (idx == -1) {
			continue;
		}
		int endidx = str.find('\n', idx);
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);
		map[key] = value;
	}
	_ip = map["ip"];
	// _port = map["port"];
	_port = atoi((char*)map["port"].c_str());
	_username = map["username"];
	_password = map["password"];
	_dbname = map["dbname"];
	_initSize = atoi(map["initSize"].c_str());
	_maxSize = atoi(map["maxSize"].c_str());
	_maxIdleTime = atoi(map["maxIdleTime"].c_str());
	_connectionTimeout = atoi(map["connectionTimeout"].c_str());
	return true;

}

void ConnectionPool::produceConnectionTask()
{
	for (;;) {

		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty()) {
			cv.wait(lock); // 队列不空, 生产线程进入等待状态
		}

		// 连接数量没有到达上限, 继续创建新连接
		if (_connectionCnt < _maxSize) {
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAliveTime();
			_connectionQue.push(p);
			_connectionCnt++;
		}

		// 通知消费者线程可以消费连接了
		cv.notify_all();
	}

}

// 释放多余的闲置连接
void ConnectionPool::scannerConnectionTask()
{
	for (;;) {
		// sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

		//扫描整个队列
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize) {
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() >= (1000 * _maxIdleTime)) {
				_connectionQue.pop();
				_connectionCnt--;
				delete p; // 释放连接调用~Connection()
			}else {
				break;
			}
		}
	}
}

shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty()) {
		// 生产者线程生产或连接回收时都会notify

		// 是空的话就进入等待
		if(cv_status::timeout == cv.wait_for(lock, chrono::microseconds(_connectionTimeout))) {
			if (_connectionQue.empty()) {
				LOG("获取空闲连接失败");
				return nullptr;
			}
		}
		// 被唤醒而没有超时, 再检查一次
	}


	// 重写shared_ptr删除器
	auto deleteFunctor = [&](Connection* pconn) {
		// 插入还是要加锁
		unique_lock<mutex> lock(_queueMutex);
		pconn->refreshAliveTime(); // 刷新存活时间
		_connectionQue.push(pconn);
	};
	shared_ptr<Connection> sp(_connectionQue.front(), deleteFunctor);
	_connectionQue.pop();

	// 如果消费了队列中的最后一个conn, 负责通知生产者
	if (_connectionQue.empty()) {
		cv.notify_all(); // 赶紧生产连接吧
	}
	
	return sp;
}

ConnectionPool::ConnectionPool() {
	if (!loadConfigFile()) {
		return;
	}

	for (int i = 0; i < _initSize; ++i) {
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->refreshAliveTime();
		_connectionQue.push(p);
		_connectionCnt++;
	}

	// 启动生产者线程新线程
	thread produce(std::bind(
		&ConnectionPool::produceConnectionTask, this
	));
	produce.detach(); //守护线程

	// 启动定时线程, 扫描空闲连接, 超过maxIdleTime的就回收

	thread scanner(std::bind(
		&ConnectionPool::scannerConnectionTask, this
	));
	scanner.detach(); //守护线程
}
