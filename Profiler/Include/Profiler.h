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

	// Struct para devolver con un único metodo
	// Recopilando la información que solo es importante de cara
	// a un juego.
__declspec(dllexport) struct GamingData {
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
	int minGpuLoad = 1000;
	int maxGpuLoad = 0;
	//
	int minRamLoad = 1000;
	int maxRamLoad = 0;
	//
	int minTemp = 1000;
	int maxTemp = 0;
};

namespace Profiler {
	extern __declspec(dllexport) int i = 0;  // Okay--export defi
	extern __declspec(dllexport) int firstTime = 0;
	extern __declspec(dllexport) int indx = 0;

	class gameInfo {
	private:
	
	public:
		// Shows a test message
		static __declspec(dllexport) GamingData getGameInfo(WMIqueryServer WMI);
	};

	// Clase para obtener informacion del Sistema Operativo
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

	// Clase para obtener informacion de la CPU
	class checkCPU {
		private:
		public:
			// Informacion general de la CPU
			static __declspec(dllexport) std::string getCPU();
			// Carga de la CPU
			static __declspec(dllexport) float GetCPULoad();
			// Obtener los cores de la CPU
			static __declspec(dllexport) int getCPUCores();
			// Velocidad de la CPU
			static __declspec(dllexport) int getCPUSpeed();
			// Carga de la CPU en funcion del tiempo
			static __declspec(dllexport) float CalculateCPULoad(unsigned long idleTicks, unsigned long totalTicks);
			// Metodos auxiliares para calcular la carga de la CPU
			static __declspec(dllexport) unsigned long FileTimeToInt64(const FILETIME& ft);
			static __declspec(dllexport) void getCPUcoresLoad(WMIqueryServer WMI, GamingData& allInfo);
	};

	// Clase para obtener informacion de la memoria
	class checkMemory {
	private:
	public:
		// Informacion general de la memoria
		static __declspec(dllexport) void getMemInfo();
		// Procesos en la memoria
		static __declspec(dllexport) GamingData getProcessMemInfo(GamingData gd);
		// Memoria Fisica
		static __declspec(dllexport) void showPhysicalMemoryInfo(WMIqueryServer WMI);
		// Velocidad de la RAM
		static __declspec(dllexport) int getRAMSpeed(WMIqueryServer WMI);
		// Tamaños de la RAM
		// En MegaBytes
		static __declspec(dllexport) int getRAMSizeMB();
		// En GigaBytes
		static __declspec(dllexport) int getRAMSizeGB();
		// Carga de la RAM
		static __declspec(dllexport) int getRAMLoad();
	};
	
	// Clase para obtener la informacion de la GPU
	class  checkGPU {
	private:	
	public:
		// Tarjeta Grafica
		static __declspec(dllexport) void showVideoControllerInfo(WMIqueryServer WMI);
		// Modelo de la GPU
		static __declspec(dllexport) std::string GetGPUModel(WMIqueryServer WMI);
		// No funciona, esto depende de la aplicacion
		static __declspec(dllexport) void GetFps();
		static __declspec(dllexport) void countFrames();
		// Carga de la GPU
		static __declspec(dllexport) int getGPULoad();
		// Temperatura de la GPU
		static __declspec(dllexport) int getGPUTemp();
	};


	class serialize {
	private:
	public:
		// Metodo para guardar la informacion en el CSV
		static __declspec(dllexport) void CSVserialize(GamingData gd);
		// Como el anterior pero organizado
		static __declspec(dllexport) void CSVserialize2(GamingData gd);
		// Metodo para escribir la informacion de los cores
		static __declspec(dllexport) void CSVCores(GamingData gd, std::ofstream& file);
		//---
		static __declspec(dllexport) GamingData CSVDeserialize();
		//---
		static __declspec(dllexport) void CSVSingleItemDeserialize(int& field, std::ifstream& file, char delimitator);
		// Metodo para escribir el dia en el CSV
		static __declspec(dllexport) void CSVDayStamp(GamingData gd, std::ofstream& file);
		// Metodo para escribir la hora en el CSV
		static __declspec(dllexport) void CSVHourStamp(GamingData gd, std::ofstream& file);
		// Metodo para añadir un entero al CSV
		static __declspec(dllexport) void CSVIntSerialize(int value, std::string info, std::ofstream& file, GamingData gi);
		// Como el anterior pero simplificado
		static __declspec(dllexport) void CSVIntSerialize2(int value, std::ofstream& file);

		//Añadir una coma al CSV
		static __declspec(dllexport) void addComeToCSV(std::ofstream& file);
		static __declspec(dllexport) void CSVPermanentInfo(GamingData gd, std::ofstream& file);
	};


#define DllExport   __declspec( dllexport )

	class DllExport testVariables {
	private:
		static int x;
	public:
		static void pruebita();
	};
}