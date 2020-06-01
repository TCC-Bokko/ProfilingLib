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
#include <intrin.h>

using namespace std;

// Use to convert bytes to KB
#define DIV 1024

//creates a static variable to convert Bytes to Megabytes
#define MB 1048576

//creates a static variable to convert Bytes to GygaBytes
#define GB 1073741824

// Specify the width of the field in which to print the numbers. 
// The asterisk in the format specifier "%*I64d" takes an integer 
// argument and uses it to pad and right justify the number.
#define WIDTH 7

// OS Version
typedef LONG NTSTATUS, *PNTSTATUS;
#define STATUS_SUCCESS (0x00000000)
typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

namespace Profiler {
	void Testing::testMSG() {
		std::cout << "Esto funciona.\n";
	}


	/////////////////////////////////////////////
	//
	// Operating System Methods
	//
	/////////////////////////////////////////////

	void checkOS::getTime() {
		SYSTEMTIME st, lt;
		GetSystemTime(&st);
		GetLocalTime(&lt);
		std::cout << "System Time is: " << st.wHour << ":" << st.wMinute << "\n";
		std::cout << "Local Time is: " << lt.wHour << ":" << lt.wMinute << "\n";
	}
	
	void checkOS::getOSVersion() {
		HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
		if (hMod) {
			RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
			if (fxPtr != nullptr) {
				RTL_OSVERSIONINFOW rovi = { 0 };
				rovi.dwOSVersionInfoSize = sizeof(rovi);
				if (STATUS_SUCCESS == fxPtr(&rovi)) {
					//return rovi;
					std::cout << "Windows Major Version: " << rovi.dwMajorVersion << std::endl;
					std::cout << "Windows Minor Version: " << rovi.dwMinorVersion << std::endl;
					std::cout << "Build Number: " << rovi.dwBuildNumber << std::endl;
					std::cout << "OSVersionInfoSize: " << rovi.dwOSVersionInfoSize << std::endl;
					std::cout << "PlatformID: " << rovi.dwPlatformId << std::endl;
				}
			}
		}
	}

	/////////////////////////////////////////////
	//
	// CPU Methods
	//
	/////////////////////////////////////////////

	//functions to calculate and retrieve CPU Load information
	float checkCPU::CalculateCPULoad(unsigned long idleTicks, unsigned long totalTicks) {
		static unsigned long long _previousTotalTicks = 0;
		static unsigned long long _previousIdleTicks = 0;

		unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
		unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;

		float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

		_previousTotalTicks = totalTicks;
		_previousIdleTicks = idleTicks;
		return ret;
	}

	unsigned long checkCPU::FileTimeToInt64(const FILETIME& ft) {
		return (((unsigned long)(ft.dwHighDateTime)) << 32) | ((unsigned long)ft.dwLowDateTime);
	}

	// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
	// Call this at regular intervals, since it measures the load between
	// the previous call and the current one.  Returns -1.0 on error.
	float checkCPU::GetCPULoad() {
		FILETIME idleTime, kernelTime, userTime;
		float CPULoad = GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime)) : -1.0f;
		std::cout << "CPU Load: " << CPULoad << "\n";
		return CPULoad;
	}

	void checkCPU::getCPUCores() {
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

	void checkCPU::getCPU() {
		int CPUInfo[4] = { -1 };
		__cpuid(CPUInfo, 0x80000000);
		unsigned int nExIds = CPUInfo[0];

		// Get the information associated with each extended ID.
		char CPUBrandString[0x40] = { 0 };
		for (unsigned int i = 0x80000000; i <= nExIds; ++i)
		{
			__cpuid(CPUInfo, i);

			// Interpret CPU brand string and cache information.
			if (i == 0x80000002)
			{
				memcpy(CPUBrandString,
					CPUInfo,
					sizeof(CPUInfo));
			}
			else if (i == 0x80000003)
			{
				memcpy(CPUBrandString + 16,
					CPUInfo,
					sizeof(CPUInfo));
			}
			else if (i == 0x80000004)
			{
				memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
			}
		}

		std::cout << "Cpu Model: " << CPUBrandString;
	}

	void checkCPU::getCPUSpeed() {
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

	/////////////////////////////////////////////
	//
	// Memory Methods
	//
	/////////////////////////////////////////////

	void checkMemory::getMemInfo() {
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

		// KiloBytes
		/*
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
		*/

		/*
		// MegaBytes
		_tprintf(TEXT("There is  %*ld percent of memory in use.\n"),
			WIDTH, statex.dwMemoryLoad);
		_tprintf(TEXT("There are %*I64d total MB of physical memory.\n"),
			WIDTH, statex.ullTotalPhys / MB);
		_tprintf(TEXT("There are %*I64d free  MB of physical memory.\n"),
			WIDTH, statex.ullAvailPhys / MB);
		_tprintf(TEXT("There are %*I64d total MB of paging file.\n"),
			WIDTH, statex.ullTotalPageFile / MB);
		_tprintf(TEXT("There are %*I64d free  MB of paging file.\n"),
			WIDTH, statex.ullAvailPageFile / MB);
		_tprintf(TEXT("There are %*I64d total MB of virtual memory.\n"),
			WIDTH, statex.ullTotalVirtual / MB);
		_tprintf(TEXT("There are %*I64d free  MB of virtual memory.\n"),
			WIDTH, statex.ullAvailVirtual / MB);

		// Show the amount of extended memory available
		_tprintf(TEXT("There are %*I64d free  MB of extended memory.\n"),
			WIDTH, (statex.ullAvailExtendedVirtual) / MB);
		*/

		// GygaBytes
		_tprintf(TEXT("There is  %*ld percent of memory in use.\n"),
			WIDTH, statex.dwMemoryLoad);
		_tprintf(TEXT("There are %*I64d total GB of physical memory.\n"),
			WIDTH, statex.ullTotalPhys / GB);
		_tprintf(TEXT("There are %*I64d free GB of physical memory.\n"),
			WIDTH, statex.ullAvailPhys / GB);
		_tprintf(TEXT("There are %*I64d total GB of paging file.\n"),
			WIDTH, statex.ullTotalPageFile / GB);
		_tprintf(TEXT("There are %*I64d free GB of paging file.\n"),
			WIDTH, statex.ullAvailPageFile / GB);
		_tprintf(TEXT("There are %*I64d total GB of virtual memory.\n"),
			WIDTH, statex.ullTotalVirtual / GB);
		_tprintf(TEXT("There are %*I64d free GB of virtual memory.\n"),
			WIDTH, statex.ullAvailVirtual / GB);

		// Show the amount of extended memory available
		_tprintf(TEXT("There are %*I64d free GB of extended memory.\n"),
			WIDTH, (statex.ullAvailExtendedVirtual) / GB);
	}

	/////////////////////////////////////////////
	//
	// GPU Methods
	//
	/////////////////////////////////////////////

}

