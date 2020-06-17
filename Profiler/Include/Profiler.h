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
#include <fstream>
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

__declspec(dllexport) struct InfoStruct {
	std::vector<SYSTEMTIME> times;
	std::vector<double> values;
};
__declspec(dllexport) struct GamingData {
	// Struct para devolver con un �nico metodo
	// Recopilando la informaci�n que solo es importante de cara
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
	int gpuTemp;
	int vRAM;
	// RAM
	int ramLoad; // 0-100
	int ramSize; // 0-100
	int ramSpeed; // Mhz
	// Memory
	int usedMemoryMB;
	int peakMemoryUsedMB;
	//Max & Mins..
	//
	int minGpuLoad = 0;//Podriamos poner la inicial el minimo no nos interesa mucho..
	int maxGpuLoad = 0;
	//
	int minRamLoad = 0;
	int maxRamLoad = 0;
	//
	int minTemp = 0;
	int maxTemp = 0;
};

namespace Profiler {



	extern __declspec(dllexport) int i = 0;  // Okay--export defi
	extern __declspec(dllexport) int firstTime = 0;
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
			static __declspec(dllexport) WMIqueryServer getWMIService(WMIqueryServer WMI);
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
			static __declspec(dllexport) void getCPUcoresLoad(WMIqueryServer WMI, GamingData& allInfo);
	};

	class checkMemory {
	private:
	public:
		static __declspec(dllexport) void showPhysicalMemoryInfo(WMIqueryServer WMI);
		static __declspec(dllexport) void getMemInfo();
		static __declspec(dllexport) GamingData getProcessMemInfo(GamingData gd);
		static __declspec(dllexport) int getRAMSpeed(WMIqueryServer WMI);
		static __declspec(dllexport) int getRAMSizeMB();
		static __declspec(dllexport) int getRAMSizeGB();
		static __declspec(dllexport) int getRAMLoad();
	};
	

	class  checkGPU {
	private:	
	public:
		//int a;
		static __declspec(dllexport) void showVideoControllerInfo(WMIqueryServer WMI);
		static __declspec(dllexport) std::string GetGPUModel(WMIqueryServer WMI);
		static __declspec(dllexport) void GetFps();
		static __declspec(dllexport) void countFrames();
		static __declspec(dllexport) int getGPULoad();
		static __declspec(dllexport) int getGPUTemp();
	};


	class serialize {
	private:
	public:
		static __declspec(dllexport) void CSVserialize(GamingData gd);
		static __declspec(dllexport) void CSVCores(GamingData gd, std::ofstream& file);
		static __declspec(dllexport) GamingData CSVDeserialize();
		static __declspec(dllexport) void CSVSingleItemDeserialize(int& field, std::ifstream& file, char delimitator);
		static __declspec(dllexport) void CSVTimeStamp(GamingData gd, std::ofstream& file);
		static __declspec(dllexport) void CSVIntSerialize(int value, std::string info, std::ofstream& file, GamingData gi);
		static __declspec(dllexport) void CSVPermanentInfo(GamingData gd, std::ofstream& file);
		static __declspec(dllexport) InfoStruct CSVGetInfoFromFIle(std::string info, std::ifstream& file);
	};


#define DllExport   __declspec( dllexport )

	class DllExport testVariables {
	private:
		static int x;
	public:
		static void pruebita();
	};
}