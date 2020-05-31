// Profiler.cpp : Define las funciones exportadas de la aplicación DLL.
//
#include <iostream>
//#include "stdafx.h"
#include "Profiler.h"
#include <Windows.h>
#include <time.h>

namespace Profiler {
	void Testing::testMSG() {
		std::cout << "Esto funciona.\n";
	}

	void checkOS::getTime() {
		SYSTEMTIME st, lt;
		GetSystemTime(&st);
		GetLocalTime(&lt);
		std::cout << "System Time is: " << st.wHour << ":" << st.wMinute << "\n";
		std::cout << "Local Time is: " << lt.wHour << ":" << lt.wMinute << "\n";
	}
}

