#pragma once

#ifdef _WIN32			
#include <windows.h>
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif

#include <time.h>
#include <stdio.h>
#include <tchar.h>
#include <intrin.h>
#include <string>
#include <vector>

// WMI 
#define _WIN32_DCOM
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

__declspec(dllexport) struct WMIqueryServer {
	HRESULT hres = NULL;
	IWbemServices *pSvc = NULL;
	IWbemLocator *pLoc = NULL;
	IEnumWbemClassObject* pEnumerator;
	bool initialized = false;
	int failStatus; //  1 = error, 0 = success
	//Query Result needs conversion to printable.
	std::string stringResult;
	unsigned long long intResult;
	bool boolResult;
	BSTR bstrResult;
};

__declspec(dllexport) struct GamingData {
	// Struct para devolver con un único metodo
	// Recopilando la información que solo es importante de cara
	// a un juego.
	// OS
	SYSTEMTIME st;
	std::string motherboardModel;
	// CPU
	std::string cpuModel;
	std::string cpuBuilder;
	int cpuCores;
	int cpuSpeed;
	int cpuLoad; // 0-100
	std::vector<int> cpuCoresLoad;
	// GPU
	std::string gpuModel;
	int gpuLoad; // 0-100
	int vRAM;
	// Memory
	int ramLoad; // 0-100
	int ramSize; // 0-100
	int ramSpeed; // Mhz
	
};

namespace Profiler {
	class Testing {
		public:
			// Shows a test message
			static __declspec(dllexport) void testMSG();
	};

	class gameInfo {
	private:
	
	public:
		// Shows a test message
		static __declspec(dllexport) GamingData getGameInfo(WMIqueryServer WMI);

	};

	class checkOS {
		private:
			
		public:
			// Hora
			static __declspec(dllexport) void getTime();
			// Version del SO
			static __declspec(dllexport) void getOSVersion();
			// Initialize WMI Query
			static __declspec(dllexport) WMIqueryServer initializeWMI();
			static __declspec(dllexport) WMIqueryServer queryWMI(WMIqueryServer WMI, std::string wmiclass, std::string varname, std::string vartype);
			static __declspec(dllexport) void closeWMI(WMIqueryServer WMI);
	};

	class checkCPU {
		private:
		public:
			static __declspec(dllexport) float GetCPULoad();
			static __declspec(dllexport) std::string getCPU();
			static __declspec(dllexport) int getCPUCores();
			static __declspec(dllexport) int getCPUSpeed();
			static __declspec(dllexport) float CalculateCPULoad(unsigned long idleTicks, unsigned long totalTicks);
			static __declspec(dllexport) unsigned long FileTimeToInt64(const FILETIME& ft);
			static __declspec(dllexport) std::vector<int> getCPUcoresLoad(WMIqueryServer WMI);
	};

	class checkMemory {
	private:
	public:
		static __declspec(dllexport) void showPhysicalMemoryInfo(WMIqueryServer WMI);
		static __declspec(dllexport) void getMemInfo();
		static __declspec(dllexport) void getProcessMemInfo();
		static __declspec(dllexport) int getRAMSpeed(WMIqueryServer WMI);
		static __declspec(dllexport) int getRAMSizeMB();
		static __declspec(dllexport) int getRAMSizeGB();
	};
	
	
	extern __declspec(dllexport) int i = 0;  // Okay--export defi
	class  checkGPU {
	private:
		
	public:
		//int a;
		static __declspec(dllexport) void showVideoControllerInfo(WMIqueryServer WMI);
		static __declspec(dllexport) std::string GetGPUModel(WMIqueryServer WMI);
		static __declspec(dllexport) void GetFps();
		static __declspec(dllexport) void countFrames();
		static __declspec(dllexport) int getGPULoad();
	};


	class serialize {
	private:
	public:
		static __declspec(dllexport) void CSVserialize(GamingData gd);
	};


#define DllExport   __declspec( dllexport )

	class DllExport testVariables {
	private:
		static int x;
	public:
		static void pruebita();
	};
}