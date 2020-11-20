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
	// ���е��ų�ʼ��, ����������
	static ConnectionPool pool; // ��̬����ĳ�ʼ���������Զ�lock��unlock
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
			cv.wait(lock); // ���в���, �����߳̽���ȴ�״̬
		}

		// ��������û�е�������, ��������������
		if (_connectionCnt < _maxSize) {
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAliveTime();
			_connectionQue.push(p);
			_connectionCnt++;
		}

		// ֪ͨ�������߳̿�������������
		cv.notify_all();
	}

}

// �ͷŶ������������
void ConnectionPool::scannerConnectionTask()
{
	for (;;) {
		// sleepģ�ⶨʱЧ��
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

		//ɨ����������
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize) {
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() >= (1000 * _maxIdleTime)) {
				_connectionQue.pop();
				_connectionCnt--;
				delete p; // �ͷ����ӵ���~Connection()
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
		// �������߳����������ӻ���ʱ����notify

		// �ǿյĻ��ͽ���ȴ�
		if(cv_status::timeout == cv.wait_for(lock, chrono::microseconds(_connectionTimeout))) {
			if (_connectionQue.empty()) {
				LOG("��ȡ��������ʧ��");
				return nullptr;
			}
		}
		// �����Ѷ�û�г�ʱ, �ټ��һ��
	}


	// ��дshared_ptrɾ����
	auto deleteFunctor = [&](Connection* pconn) {
		// ���뻹��Ҫ����
		unique_lock<mutex> lock(_queueMutex);
		pconn->refreshAliveTime(); // ˢ�´��ʱ��
		_connectionQue.push(pconn);
	};
	shared_ptr<Connection> sp(_connectionQue.front(), deleteFunctor);
	_connectionQue.pop();

	// ��������˶����е����һ��conn, ����֪ͨ������
	if (_connectionQue.empty()) {
		cv.notify_all(); // �Ͻ��������Ӱ�
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

	// �����������߳����߳�
	thread produce(std::bind(
		&ConnectionPool::produceConnectionTask, this
	));
	produce.detach(); //�ػ��߳�

	// ������ʱ�߳�, ɨ���������, ����maxIdleTime�ľͻ���

	thread scanner(std::bind(
		&ConnectionPool::scannerConnectionTask, this
	));
	scanner.detach(); //�ػ��߳�
}
