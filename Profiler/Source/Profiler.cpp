// Profiler.cpp : Define las funciones exportadas de la aplicación DLL.

// https://www.cprogramming.com/snippets/source-code/find-the-number-of-cpu-cores-for-windows-mac-or-linux
// This snippet submitted by Dirk-Jan Kroon on 2010-06-09. It has been viewed 23370 times.
#ifdef _WIN32			
#include <windows.h>
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif

#include <iostream>
//#include "stdafx.h"
#include "Profiler.h"
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <tchar.h>

// Use to convert bytes to KB
#define DIV 1024

// Specify the width of the field in which to print the numbers. 
// The asterisk in the format specifier "%*I64d" takes an integer 
// argument and uses it to pad and right justify the number.
#define WIDTH 7


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

	void checkOS::getCPUCores() {
#ifdef WIN32
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		int numCores = sysinfo.dwNumberOfProcessors;
		std::cout << "Cores: " << numCores << "\n";
#elif MACOS
		int nm[2];
		size_t len = 4;
		uint32_t count;

		nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
		sysctl(nm, 2, &count, &len, NULL, 0);

		if (count < 1) {
			nm[1] = HW_NCPU;
			sysctl(nm, 2, &count, &len, NULL, 0);
			if (count < 1) { count = 1; }
		}
		std::cout << "Cores: " << count << "\n";
#else
		int numCores = sysconf(_SC_NPROCESSORS_ONLN);
		std::cout << "Cores: " << count << "\n";
#endif
	}

	void checkOS::getCPU() {
		/*
		int CPUInfo[4] = { -1 };
		unsigned   nExIds, i = 0;
		char CPUBrandString[0x40];
	
		// Get the information associated with each extended ID.
		__cpuid(CPUInfo, 0x80000000);
	
		nExIds = CPUInfo[0];
		for (i = 0x80000000; i <= nExIds; ++i)
		{
			__cpuid(CPUInfo, i);
			// Interpret CPU brand string
			if (i == 0x80000002)
				memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
			else if (i == 0x80000003)
				memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
			else if (i == 0x80000004)
				memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
		}
		//string includes manufacturer, model and clockspeed
		std::cout << "CPU Type: " << CPUBrandString << std::endl;


		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		std::cout << "Number of Cores: " << sysInfo.dwNumberOfProcessors << std::endl;

		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx(&statex);
		std::cout << "Total System Memory: " << (statex.ullTotalPhys / 1024) / 1024 << "MB" << std::endl;
		*/
	}

	void checkOS::getCPUSpeed() {
			wchar_t Buffer[_MAX_PATH];
			DWORD BufSize = _MAX_PATH;
			DWORD dwMHz = _MAX_PATH;
			HKEY hKey;

			// open the key where the proc speed is hidden:
			long lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
				0,
				KEY_READ,
				&hKey);
			if (lError != ERROR_SUCCESS)
			{// if the key is not found, tell the user why:
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					lError,
					0,
					Buffer,
					_MAX_PATH,
					0);
				wprintf(Buffer);
				//return 0;
			}

			// query the key:
			RegQueryValueEx(hKey, L"~MHz", NULL, NULL, (LPBYTE)&dwMHz, &BufSize);

			std::cout << "CPU Speed: " << (double)dwMHz << " MHz." << std::endl;
			//return (double)dwMHz;
	}

	void checkOS::getCPUInfo() {
		// Sample output :
		//  There is       51 percent of memory in use.
		//  There are 2029968 total KB of physical memory.
		//  There are  987388 free  KB of physical memory.
		//  There are 3884620 total KB of paging file.
		//  There are 2799776 free  KB of paging file.
		//  There are 2097024 total KB of virtual memory.
		//  There are 2084876 free  KB of virtual memory.
		//  There are       0 free  KB of extended memory.

		MEMORYSTATUSEX statex;

		statex.dwLength = sizeof(statex);

		GlobalMemoryStatusEx(&statex);

		_tprintf(TEXT("There is  %*ld percent of memory in use.\n"),
			WIDTH, statex.dwMemoryLoad);
		_tprintf(TEXT("There are %*I64d total KB of physical memory.\n"),
			WIDTH, statex.ullTotalPhys / DIV);
		_tprintf(TEXT("There are %*I64d free  KB of physical memory.\n"),
			WIDTH, statex.ullAvailPhys / DIV);
		_tprintf(TEXT("There are %*I64d total KB of paging file.\n"),
			WIDTH, statex.ullTotalPageFile / DIV);
		_tprintf(TEXT("There are %*I64d free  KB of paging file.\n"),
			WIDTH, statex.ullAvailPageFile / DIV);
		_tprintf(TEXT("There are %*I64d total KB of virtual memory.\n"),
			WIDTH, statex.ullTotalVirtual / DIV);
		_tprintf(TEXT("There are %*I64d free  KB of virtual memory.\n"),
			WIDTH, statex.ullAvailVirtual / DIV);

		// Show the amount of extended memory available
		_tprintf(TEXT("There are %*I64d free  KB of extended memory.\n"),
			WIDTH, statex.ullAvailExtendedVirtual / DIV);
	}
}

