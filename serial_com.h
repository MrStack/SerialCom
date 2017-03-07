#pragma once
#include<Windows.h>
#include<stdio.h>
#include<queue>
#include<iomanip>
#include<iostream>
#include"thread_safe_queue.h"

using namespace std;

class CCom
{
private:
	HANDLE m_hcom;
	DCB m_dcb;
	HKEY m_hkey;
	LPWSTR m_MainSubKey;
	int m_RET;
	LPWSTR m_pKeyValueName;
	unsigned long m_KeyValueType;
	unsigned long m_KeyValueDataSize;
	LPWSTR m_pKeyValueDataSize;

	threadsafe_queue<unsigned char> m_DataChar;

public:
	CCom(void) :m_hcom{}, m_dcb{}, m_hkey{ HKEY_LOCAL_MACHINE }, m_MainSubKey{ TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM") }, m_RET{}, m_pKeyValueName{ TEXT("\\Device\\Serial2") }, m_KeyValueDataSize{}, m_KeyValueType{KEY_READ}, m_pKeyValueDataSize{}
	{
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, m_MainSubKey, NULL, m_KeyValueType, &m_hkey);
		RegQueryValueEx(m_hkey, m_pKeyValueName, NULL, &m_KeyValueType, NULL, &m_KeyValueDataSize);
		m_pKeyValueDataSize = new TCHAR[m_KeyValueDataSize];

		RegQueryValueEx(m_hkey, m_pKeyValueName, NULL, &m_KeyValueType, (LPBYTE)m_pKeyValueDataSize, &m_KeyValueDataSize);
		RegCloseKey(m_hkey);
		m_hcom = CreateFile(m_pKeyValueDataSize, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (m_hcom == INVALID_HANDLE_VALUE)
		{
			MessageBox(nullptr, TEXT("Serial com error."), TEXT("Error"), MB_OK);
			exit(1);
		}
		SetupComm(m_hcom, 1024, 1024);         //输入缓冲区和输出缓冲区都是1024
		GetCommState(m_hcom, &m_dcb);            //读取串口配置信息
		m_dcb.BaudRate = 115200;
		m_dcb.ByteSize = 8;
		m_dcb.Parity = NOPARITY;
		m_dcb.StopBits = ONESTOPBIT;
		SetCommState(m_hcom, &m_dcb);            //修改串口配置信息
		PurgeComm(m_hcom, PURGE_TXCLEAR | PURGE_RXCLEAR);      //将接受缓冲区清空
	}
	~CCom()
	{
		CloseHandle(m_hcom);
		delete[] m_pKeyValueDataSize;
	}

	HANDLE ComHandle(void)
	{
		return m_hcom;
	}
	void __cdecl ReceiveData(void)
	{
		unsigned char TEMP[2]{};
		BOOL ReadSta = TRUE;
		unsigned long RealCnt = 0;

		while (1)
		{
			ReadSta = ReadFile(m_hcom, TEMP, 1, &RealCnt, NULL);
			m_DataChar.push(TEMP[0]);
		//	std::cout << std::hex << static_cast<int>(TEMP[0]) << std::setw(5);
			if (!ReadSta)
			{
				MessageBox(nullptr, TEXT("Can't read com."), TEXT("Error"), MB_OK);
				exit(1);
			}
		}
	}
	threadsafe_queue<unsigned char>& GetQueue(void)
	{
		return m_DataChar;
	}
};