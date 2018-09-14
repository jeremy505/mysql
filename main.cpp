/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/13/2018 11:59:13 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "base/Logging.h"
#include "base/Singleton.h"
#include "mysql/MysqlManager.h"
int main(int argc, char* argv[])
{
    Logger::setLogLevel(Logger::INFO);
    // Logger::setLogLevel(Logger::FATAL);

	const char* dbserver = "localhost";
    const char* dbuser = "root";
    const char* dbpassword = "jeremy";
    const char* dbname = "People";
    CMysqlManager pmysql = Singleton<CMysqlManager>::Instance();
    if (!pmysql.Init(dbserver, dbuser, dbpassword, dbname))
	{
		 LOG_FATAL << "Init mysql failed, please check your database config..............";
	}

  	// 初始化表
	// 1. t_user
	STableInfo info;
    info.m_strName = "friend";
    info.m_mapField["id"] = { "id", "bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID'", "bigint(20)" };
    info.m_mapField["name"] = { "name", "varchar(64) NOT NULL COMMENT '名字'", "varchar(64)" };
    info.m_mapField["age"] = { "age", "int(10) NOT NULL COMMENT '年龄'", "int(10)" };
    info.m_mapField["phonenumber"] = { "phonenumber", "varchar(64) DEFAULT NULL COMMENT '电话'", "varchar(64)" };


    info.m_strKeyString = "PRIMARY KEY (id)";
    pmysql.DeleteData("DELETE FROM " + info.m_strName);
    pmysql.InitSTableInfo(info);

    if(!pmysql.CreateTable())
    {
        LOG_FATAL << "mysql CreateTable failed, please check your table config..............";
    }

    STableData data;

    data.m_strName = "friend";
    data.m_mapData["id"] =  "1";
    data.m_mapData["name"] = "\"jeremy\"";
    data.m_mapData["age"] = "22";
    data.m_mapData["phonenumber"] = "12221221";

    if(!pmysql.InsertData(data))
    {
        LOG_WARN << "CMysqlManager::InsertData, data insert failed";
    }

    std::shared_ptr<CDatabaseMysql> poConn = pmysql.GetCurpoConn();

    char sql[256] = { 0 };
    snprintf(sql, 256, "SELECT name, age, phonenumber FROM friend");
    QueryResult* pResult = poConn->Query(sql);
    if (NULL == pResult)
    {
        LOG_INFO << "UserManager::Query error, db=" << dbname;
        return false;
    }

    struct People_atrribute{
        string name;
        int age;
        string phonenumber;
    };

    People_atrribute someone;
    while (true)
    {
        Field* pRow = pResult->Fetch();
        if (pRow == NULL)
            break;

        someone.name = pRow[0].GetString();
        someone.age = pRow[1].GetInt32();
        someone.phonenumber = pRow[2].GetString();
        LOG_INFO << pRow[0].GetString() << "-" << pRow[1].GetInt32() << "-" << pRow[2].GetString();
        LOG_INFO << "name= " << someone.name
                 << "  age= " << someone.age
                 << "  phonenumber= " << someone.phonenumber;
        if (!pResult->NextRow())
        {
            break;
        }
    }

    pResult->EndQuery();

	return 0;
}
