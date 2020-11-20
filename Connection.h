#pragma once
#include "mysql.h"
#include  <time.h>
#include <string>
using namespace std;

class Connection {
public:
	Connection();
	~Connection();

	bool connect(string ip, unsigned short port,
			string username, string password, string dbname);

	bool update(string sql);

	MYSQL_RES* query(string sql);
	// 刷新起始的空闲时间点
	void refreshAliveTime() { _alivetime = clock(); }
	// 返回存活的时间
	clock_t getAliveTime() const { return clock() - _alivetime; }
private:
	MYSQL* _conn;
	clock_t _alivetime; //记录空闲状态后的存活
};