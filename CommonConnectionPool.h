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
	static ConnectionPool* getConnectionPool();// #1 �ṩ��̬�ӿ��Թ���ȡʵ��
	
	// ���ⲿ�ṩ�Ļ�ȡconn�ӿ�
	// ����ָ������ʱֻ�黹������
	shared_ptr<Connection> getConnection();
private:
	ConnectionPool(); // #1 ���캯��˽�л�

	bool loadConfigFile(); // ��������

	// �����ڶ������߳���
	// �̺߳���д����ĳ�Ա�����ô��Ƿ�����ʶ���ĳ�Ա����
	// ��thisָ������Ҫ�Ķ����ð�����
	void produceConnectionTask();

	// ɨ�����ӵ��̺߳���
	void scannerConnectionTask();
	string _ip;
	unsigned short _port;
	string _username;
	string _password;
	string _dbname;
	int _initSize; // ��ʼ������
	int _maxSize; // ���������
	int _maxIdleTime; // �������ռ�ʱ��
	int _connectionTimeout; // ������ʱʱ��

	queue<Connection*> _connectionQue; // �洢���ӵĶ���, �����
	mutex _queueMutex;
	atomic_int _connectionCnt; //���������ӵ�������

	// ��������, �����������̺߳��������̵߳�ͨ��
	condition_variable cv;
};