#include "MysqlManager.h"
#include <sstream>
#include "../base/Logging.h"
#include "../base/Singleton.h"


CMysqlManager::CMysqlManager(void)
{
	//TODO: m_strCharactSet可以放在初始化列表中初始化
    m_strCharactSet = "utf8";
}

CMysqlManager::~CMysqlManager(void)
{
    m_poConn.reset();
}

bool CMysqlManager::Init(const char* host, const char* user, const char* pwd, const char* dbname)
{
	m_strHost = host;
	m_strUser = user;
    //数据库密码可能为空
    if (pwd != NULL)
	    m_strPassword = pwd;
	m_strDataBase = dbname;

	//注意：检查数据库是否存在时，需要将数据库名称设置为空
	m_poConn.reset(new CDatabaseMysql());
    if (!m_poConn->Initialize(m_strHost, m_strUser, m_strPassword, ""))
	{
		LOG_FATAL << "CMysqlManager::Init failed, please check params(" << m_strHost << ", " << m_strUser << ", " << m_strPassword << ")";
		return false;
	}

	////////////////////// 1. 检查库是否存在 /////////////////////////
	if (!_IsDBExist())
	{
		if (!_CreateDB())
		{
			return false;
		}
	}

	//再次确定是否可以连接上数据库
    m_poConn.reset(new CDatabaseMysql());
	if (!m_poConn->Initialize(m_strHost, m_strUser, m_strPassword, m_strDataBase))
	{
		LOG_FATAL << "CMysqlManager::Init failed, please check params(" << m_strHost << ", " << m_strUser
			<< ", " << m_strPassword << ", " << m_strDataBase << ")";
		return false;
	}
    return true;
}

bool CMysqlManager::_IsDBExist()
{
	if (NULL == m_poConn)
	{
		return false;
	}

	QueryResult* pResult = m_poConn->Query("show databases");
	if (NULL == pResult)
	{
		LOG_INFO << "CMysqlManager::_IsDBExist, no database(" << m_strDataBase << ")";
		return false;
	}

	Field* pRow = pResult->Fetch();
	while (pRow != NULL)
	{
		string name = pRow[0].GetString();
		if (name == m_strDataBase)
		{
			LOG_INFO << "CMysqlManager::_IsDBExist, find database(" << m_strDataBase << ")";
			pResult->EndQuery();
			return true;
		}
		
		if (pResult->NextRow() == false)
		{
			break;
		}
		pRow = pResult->Fetch();
	}

	LOG_INFO << "CMysqlManager::_IsDBExist, no database(" << m_strDataBase << ")";
	pResult->EndQuery();
	return false;
}

bool CMysqlManager::_CreateDB()
{
	if (NULL == m_poConn)
	{
		return false;
	}

	uint32_t uAffectedCount = 0;
	int nErrno = 0;

	stringstream ss;
	ss << "create database " << m_strDataBase;
	if (m_poConn->Execute(ss.str().c_str(), uAffectedCount, nErrno))
	{
		if (uAffectedCount == 1)
		{
			LOG_INFO << "CMysqlManager::_CreateDB, create database " <<
				m_strDataBase << " success";
			return true;
		}
	}
	else
	{
		LOG_ERROR << "CMysqlManager::_CreateDB, create database " << m_strDataBase << " failed("
			<< nErrno << ")";
		return false;
	}
	return false;
}

void CMysqlManager::InitSTableInfo(const STableInfo& info)
{
	m_vecTableInfo.push_back(info);
}

bool CMysqlManager::CreateTable(void)
{
	if(m_vecTableInfo.size() == 0)
	{
		LOG_WARN << "The CreateTable's Infomation is empty, it will not be created!";

		return true;
	}
	for (size_t i = 0; i < m_vecTableInfo.size(); i++)
	{
		STableInfo table = m_vecTableInfo[i];
		if (!_CheckTable(table))
		{
	  		LOG_FATAL << "CMysqlManager::Init, table check failed : " << table.m_strName;
		 	return false;
	  	}
	 }

	m_vecTableInfo.clear();
	//	LOG_INFO << "m_vecTableInfo.size() = " << m_vecTableInfo.size() ;
	return true;
}

bool CMysqlManager::InsertData(const STableData& data)
{
	if (data.m_strName.find_first_not_of("\t\r\n ") == string::npos)
	{
	   	LOG_WARN << "CMysqlManager::_CheckTable, tale info not valid";
	   	return true;
	}

	string field;
	string value;
	 for (map<string, string>::const_iterator it = data.m_mapData.begin();
		 it != data.m_mapData.end(); ++it)
	 {
		 field += it->first;
		 field += ",";
		 value += it->second;
		 value += ",";
	 }

	 field.erase(field.end()-1);
	 value.erase(value.end()-1);

	 stringstream ss;
	 ss << "INSERT INTO " << data.m_strName << "("
		<< field << ") " << "VALUES(" << value << ")";

	 string sql = ss.str();

	 if (m_poConn->Execute(sql.c_str()))
	 {
			 LOG_INFO << sql;
	 }
	 else
	 {
		   LOG_ERROR << "CMysqlManager::InsertData failed : " << sql;
		  return false;
	 }

	 return true;
}


bool CMysqlManager::DeleteData(const string& cmd)
{
	string sql = cmd.c_str();

	if (m_poConn->Execute(sql.c_str()))
	{
		LOG_INFO << sql;
	}
	else
	{
		LOG_ERROR << "CMysqlManager::InsertData failed : " << sql;
		return false;
	}

	return true;
}

bool CMysqlManager::_CheckTable(const STableInfo& table)
{
	if (NULL == m_poConn)
	{
		return false;
	}

	if (table.m_strName.find_first_not_of("\t\r\n ") == string::npos)
	{
		LOG_WARN << "CMysqlManager::_CheckTable, tale info not valid";
		return true;
	}

	stringstream ss;
	ss << "desc " << table.m_strName;
	QueryResult* pResult = m_poConn->Query(ss.str());
	if (NULL == pResult)
	{
		LOG_INFO << "CMysqlManager::_CheckTable, no table" << table.m_strName << ", begin create.....";
		if (_CreateTable(table))
		{
			LOG_INFO << "CMysqlManager::_CheckTable, " << table.m_strName << ", end create.....";
			return true;
		}
		return false;
	}
	else
	{
		map<string, string> mapOldTable;
		Field* pRow = pResult->Fetch();
		while (pRow != NULL)
		{
			string name = pRow[0].GetString();
			string type = pRow[1].GetString();
			mapOldTable[name] = type;

			if (pResult->NextRow() == false)
			{
				break;
			}
			pRow = pResult->Fetch();
		}

		pResult->EndQuery();

		for (map<string, STableField>::const_iterator it = table.m_mapField.begin();
			it != table.m_mapField.end(); ++it)
		{
			STableField field = it->second;
			if (mapOldTable.find(field.m_strName) == mapOldTable.end())
			{
				stringstream ss;
				ss << "alter table " << table.m_strName << " add column "
					<< field.m_strName << " " << field.m_strType;

				string sql = ss.str();
				if (m_poConn->Execute(sql.c_str()))
				{
					LOG_INFO << sql;
					continue;
				}
				else
				{
					LOG_ERROR << "CMysqlManager::_CheckTable failed : " << sql;
					return false;
				}
			}
		}
	}

	return true;
}

bool CMysqlManager::_CreateTable(const STableInfo& table)
{
	if (table.m_mapField.size() == 0)
	{
		LOG_ERROR << "CMysqlManager::_CreateTable, table info not valid, " << table.m_strName;
		return false;
	}
	stringstream ss;
	ss << "CREATE TABLE IF NOT EXISTS " << table.m_strName << " (";
	
	for (map<string, STableField>::const_iterator it = table.m_mapField.begin();
		it != table.m_mapField.end(); ++it)
	{
		if (it != table.m_mapField.begin())
		{
			ss << ", ";
		}

		STableField field = it->second;
		ss << field.m_strName << " " << field.m_strType;		
	}

	if (table.m_strKeyString != "")
	{
		ss << ", " << table.m_strKeyString;
	}

	ss << ")default charset = utf8, ENGINE = InnoDB;";
	if (m_poConn->Execute(ss.str().c_str()))
	{
		return true;
	}

	return false;
}

