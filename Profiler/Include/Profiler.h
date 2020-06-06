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
	std::string queryResult;
};

__declspec(dllexport) struct GamingData {
	// Struct para devolver con un único metodo
	// Recopilando la información que solo es importante de cara
	// a un juego.
	SYSTEMTIME st;
	std::string gpuModel;
	std::string cpuModel;
	int cpuLoad; // 0-100
	int gpuLoad; // 0-100
	int ramLoad; // 0-100
	int ramSize;
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
			static __declspec(dllexport) WMIqueryServer queryWMI(WMIqueryServer WMI, std::string wmiclass, std::string varname);
			static __declspec(dllexport) void closeWMI(WMIqueryServer WMI);
	};

	class checkCPU {
		private:
			static __declspec(dllexport) float CalculateCPULoad(unsigned long idleTicks, unsigned long totalTicks);
			static __declspec(dllexport) unsigned long FileTimeToInt64(const FILETIME& ft);
		public:
			static __declspec(dllexport) float GetCPULoad();
			//
			static __declspec(dllexport) std::string getCPU();
			// Cores
			static __declspec(dllexport) int getCPUCores();
			// Velocidad
			static __declspec(dllexport) void getCPUSpeed();
	};

	class checkMemory {
	private:
	public:
		static __declspec(dllexport) void getMemInfo();
		static __declspec(dllexport) void getProcessMemInfo();
	};
	
	
	extern __declspec(dllexport) int i = 0;  // Okay--export defi
	class  checkGPU {
	private:
		
	public:
		//int a;
		static __declspec(dllexport) std::string GetGPUModel(WMIqueryServer WMI);
		static __declspec(dllexport) void GetFps();
		static __declspec(dllexport) void countFrames();
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