// mysql_conn_pool.cpp : ���ļ����� "main" ����������ִ�н��ڴ˴���ʼ��������
//

#include <iostream>
#include <time.h>
using namespace std;
#include "Connection.h"
#include "CommonConnectionPool.h"
int main()
{
    /*
    Connection conn;
    char sql[1024] = { 0 };
    sprintf_s(sql, "insert into user(name, age, sex) values('%s', '%d', '%s')", "zhang san", 20, "male");
    conn.connect("127.0.0.1", 3306, "root", "root", "chat");
    conn.update(sql);
    */
    // ConnectionPool* cp = ConnectionPool::getConnectionPool();
    // cp->loadConfigFile();
    clock_t begin, end;
    /*
    begin = clock();
    for (int i = 0; i < 10000; ++i) {
        Connection conn;
        char sql[1024] = { 0 };
        sprintf_s(sql, "insert into user(name, age, sex) values('%s', '%d', '%s')", "zhang san", 20, "male");
        conn.connect("127.0.0.1", 3306, "root", "root", "chat");
        conn.update(sql);
    }
    end = clock();
    cout << "��ʹ�����ӳ�10000��:" << end - begin << "ms" << endl;
    
    
    begin = clock();
    ConnectionPool* cp = ConnectionPool::getConnectionPool();
    for (int i = 0; i < 10000; ++i) {
        
        shared_ptr<Connection> sp = cp->getConnection();
        char sql[1024] = { 0 };
        sprintf_s(sql, "insert into user(name, age, sex) values('%s', '%d', '%s')", "zhang san", 20, "male");
        sp->update(sql);
    }
    end = clock();
    cout << "ʹ�����ӳ�10000��:" << end - begin << "ms" << endl;
    */

    const int UP = 10000;
    Connection conn;
    conn.connect("127.0.0.1", 3306, "root", "root", "chat");
    /*
    begin = clock();
    for (int i = 0; i < 10000; ++i) {
        Connection conn;
        char sql[1024] = { 0 };
        sprintf_s(sql, "insert into user(name, age, sex) values('%s', '%d', '%s')", "zhang san", 20, "male");
        conn.connect("127.0.0.1", 3306, "root", "root", "chat");
        conn.update(sql);
    }
    end = clock();
    cout << "��ʹ�����ӳ�10000��:" << end - begin << "ms" << endl;
    */

    
    auto testFunc = [&]() {
        //ConnectionPool* cp = ConnectionPool::getConnectionPool();
        for (int i = 0; i < UP / 4; ++i) {
            /*
            shared_ptr<Connection> sp = cp->getConnection();
            char sql[1024] = { 0 };
            sprintf_s(sql, "insert into user(name, age, sex) values('%s', '%d', '%s')", "zhang san", 20, "male");
            sp->update(sql);
            */

            Connection conn;
            char sql[1024] = { 0 };
            sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
                "zhang san", 20, "male");
            conn.connect("127.0.0.1", 3306, "root", "root", "chat");
            conn.update(sql);

        }
    };
    begin = clock();
    thread t1(testFunc);
    thread t2(testFunc);
    thread t3(testFunc);
    thread t4(testFunc);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    end = clock();
    cout << "��ʹ�����ӳ�" << UP << "��:" << end - begin << "ms" << endl;

    return 0;
}
