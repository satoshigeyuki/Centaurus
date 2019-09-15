#pragma once

#include <iostream>
#include "CppUnitTest.h"

template<typename T, int N = 4096>
class LoggerStreamBuf : public std::basic_streambuf<T>
{
	using std::basic_streambuf<T>::setp;
	using std::basic_streambuf<T>::pptr;

	T m_buffer[N + 2];
public:
	LoggerStreamBuf()
	{
		setp(m_buffer, m_buffer + N);
	}
	virtual ~LoggerStreamBuf() {}
	int overflow(int c = EOF)
	{
		T* p1 = pptr();
		p1[0] = c;
		p1[1] = 0;
		Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(m_buffer);

		setp(m_buffer, m_buffer + N);
		return EOF;
	}
	int underflow()
	{
		return EOF;
	}
	int sync()
	{
		T* p1 = pptr();
		p1[0] = 0;
		Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(m_buffer);

		setp(m_buffer, m_buffer + N);
		return 0;
	}
};