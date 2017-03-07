#pragma once
#include<Windows.h>
#include<cmath>
#include"serial_com.h"
#include<thread>
#include<iostream>
#include<iomanip>
#include"thread_safe_queue.h"
#include<fstream>

struct SPoint
{
	unsigned int Num;
	double x;
	double y;
};

class CLocAna
{
private:
	CCom m_Com;

	const double m_A0x;
	const double m_A0y;
	const double m_A1x;
	const double m_A1y;
	const double m_A2x;
	const double m_A2y;
	const double m_Height;

	double m_Reality_x;
	double m_Reality_y;
	double m_Reality_A0;
	double m_Reality_A1;
	double m_Reality_A2;

	double m_Distance[3][4];
	threadsafe_queue<SPoint> m_Queue;

	unsigned char m_MovingPoint;

	std::thread m_ComReceive;
	std::thread m_Analyzer;

	/*Test code*/
	std::fstream m_FileStream;

	void Compute(unsigned char MovingPoint)
	{
		m_Reality_A0 = sqrt(pow(m_Distance[MovingPoint][0], 2.0) - pow(m_Height, 2.0));
		m_Reality_A1 = sqrt(pow(m_Distance[MovingPoint][1], 2.0) - pow(m_Height, 2.0));
		m_Reality_A2 = sqrt(pow(m_Distance[MovingPoint][2], 2.0) - pow(m_Height, 2.0));
	//	std::cout << m_Distance[MovingPoint][0] << std::setw(10 )<< m_Distance[MovingPoint][1] << std::setw(10)<< m_Distance[MovingPoint][2] << std::endl;
	}

public:
	CLocAna(void) :m_Com{}, m_A0x{}, m_A0y{}, m_A1x{ 180.0 }, m_A1y{}, m_A2x{ 120.0 }, m_A2y{ 120.0 }, m_Height{75.0},
		m_Reality_x{}, m_Reality_y{}, m_Reality_A0{}, m_Reality_A1{}, m_Reality_A2{}, m_Distance{}, m_MovingPoint{},
		m_ComReceive{ [&](void) {m_Com.ReceiveData(); } }, m_Analyzer{ std::bind(&CLocAna::Analyze_data,this) },
		m_FileStream{ "Record.txt",std::ios::app | std::ios::in }
	{

	}
	~CLocAna()
	{
		m_ComReceive.detach();
		m_Analyzer.detach();
	}

	void Unpack(unsigned char MovingPoint)
	{
		Compute(MovingPoint);
		m_Reality_x = (pow(m_Reality_A0, 2) - pow(m_Reality_A1, 2) + pow(m_A1x, 2)) / (2 * m_A1x);
		m_Reality_y = (pow(m_Reality_A0, 2) - pow(m_Reality_A2, 2) - pow(m_Reality_x, 2) + pow(m_Reality_x - m_A2x, 2) + pow(m_A2y, 2)) / (2 * m_A2y);
		m_MovingPoint = MovingPoint;
		SPoint Point{ MovingPoint,m_Reality_x,m_Reality_y };
		m_FileStream << m_MovingPoint << " " << m_Reality_x << " " << m_Reality_y << endl;
		m_Queue.push(Point);
	}
	void Analyze_data(void)
	{
		unsigned char PackBuffer[8]{};
		unsigned char CheckParity{};

		while (true)
		{
			while (m_Com.GetQueue().empty()) {}
			while (0xE0 != m_Com.GetQueue().front() && !m_Com.GetQueue().empty())
			{
				m_Com.GetQueue().pop();
			}
			m_Com.GetQueue().pop();
			while (m_Com.GetQueue().size() < 8) {}
			for (int i{}; i < 8; i++)
			{
				PackBuffer[i] = m_Com.GetQueue().front();
				if (i != 7)
				{
					CheckParity ^= PackBuffer[i];
					//std::cout << std::hex << std::setw(4) << static_cast<int>(PackBuffer[i]) << ":" << static_cast<int>(CheckParity);
				}
				else
				{
					if (CheckParity != PackBuffer[7])
					{
						continue;
					}
					//std::cout << std::hex << std::setw(4) << static_cast<int>(PackBuffer[i]) << ":" << static_cast<int>(CheckParity) << "  " << std::endl;
					CheckParity = 0;
					m_Distance[PackBuffer[1]][PackBuffer[0]] = ((int)PackBuffer[2] << 16 | (int)PackBuffer[3] << 8 | (int)PackBuffer[4]) * 0.117375;
					m_Distance[PackBuffer[1]][3] += 1;
					if (m_Distance[PackBuffer[1]][3] != 3)
					{
						continue;
					}
					m_Distance[PackBuffer[1]][3] = 0;
					Unpack(PackBuffer[1]);
				}
				m_Com.GetQueue().pop();				
			}
		}
	}

	SPoint& GetPoint(void)
	{
		while (true)
		{
			if (m_Queue.size() > 0)
			{
				SPoint Point = m_Queue.front();
				m_Queue.pop();
				return Point;
			}
		}
	}
};