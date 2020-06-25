// Profiler.cpp : Define las funciones exportadas de la aplicación DLL.
// https://www.cprogramming.com/snippets/source-code/find-the-number-of-cpu-cores-for-windows-mac-or-linux
// This snippet submitted by Dirk-Jan Kroon on 2010-06-09. It has been viewed 23370 times.


// This software uses Nvidia NVAPI to check for GPU information.
#if defined(_M_X64) || defined(__amd64__)
#define NVAPI_DLL "nvapi64.dll"
#else
#define NVAPI_DLL "nvapi.dll"
#endif

#include "Profiler.h"

// OTHER
#include <Psapi.h>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <typeinfo>
#include <cstdio>
using namespace std;

// GPU (Obtained from NvAPI, Nvidia software)
// magic numbers, do not change them
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  34
#define NVAPI_MAX_THERMAL_SENSORS_PER_GPU 3

typedef enum {
	NVAPI_THERMAL_CONTROLLER_NONE = 0,
	NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL,
	NVAPI_THERMAL_CONTROLLER_ADM1032,
	NVAPI_THERMAL_CONTROLLER_MAX6649,
	NVAPI_THERMAL_CONTROLLER_MAX1617,
	NVAPI_THERMAL_CONTROLLER_LM99,
	NVAPI_THERMAL_CONTROLLER_LM89,
	NVAPI_THERMAL_CONTROLLER_LM64,
	NVAPI_THERMAL_CONTROLLER_ADT7473,
	NVAPI_THERMAL_CONTROLLER_SBMAX6649,
	NVAPI_THERMAL_CONTROLLER_VBIOSEVT,
	NVAPI_THERMAL_CONTROLLER_OS,
	NVAPI_THERMAL_CONTROLLER_UNKNOWN = -1,
} NV_THERMAL_CONTROLLER;

typedef enum {
	NVAPI_THERMAL_TARGET_NONE = 0,
	NVAPI_THERMAL_TARGET_GPU = 1,     //!< GPU core temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_MEMORY = 2,     //!< GPU memory temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_POWER_SUPPLY = 4,     //!< GPU power supply temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_BOARD = 8,     //!< GPU board ambient temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_VCD_BOARD = 9,     //!< Visual Computing Device Board temperature requires NvVisualComputingDeviceHandle
	NVAPI_THERMAL_TARGET_VCD_INLET = 10,    //!< Visual Computing Device Inlet temperature requires NvVisualComputingDeviceHandle
	NVAPI_THERMAL_TARGET_VCD_OUTLET = 11,    //!< Visual Computing Device Outlet temperature requires NvVisualComputingDeviceHandle

	NVAPI_THERMAL_TARGET_ALL = 15,
	NVAPI_THERMAL_TARGET_UNKNOWN = -1,
} NV_THERMAL_TARGET;

typedef struct {
	int   version;                //!< structure version 
	int   count;                  //!< number of associated thermal sensors
	struct {
		NV_THERMAL_CONTROLLER       controller;        //!< internal, ADM1032, MAX6649...
		int       defaultMinTemp;    //!< The min default temperature value of the thermal sensor in degree Celsius 
		int       defaultMaxTemp;    //!< The max default temperature value of the thermal sensor in degree Celsius 
		int       currentTemp;       //!< The current temperature value of the thermal sensor in degree Celsius 
		NV_THERMAL_TARGET           target;            //!< Thermal sensor targeted @ GPU, memory, chipset, powersupply, Visual Computing Device, etc.
	} sensor[NVAPI_MAX_THERMAL_SENSORS_PER_GPU];

} NV_GPU_THERMAL_SETTINGS;

// function pointer types
typedef int *(*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int(*NvAPI_Initialize_t)();
typedef int(*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);
typedef int(*NvAPI_GPU_GetUsages_t)(int *handle, unsigned int *usages);
typedef int(*NvAPI_GPU_GetThermalSettings_t)(int *handle, int sensorIndex, NV_GPU_THERMAL_SETTINGS *temp);


// MEMORY
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
	/////////////////////////////////////////////
	//
	// GameInfo Methods
	//
	/////////////////////////////////////////////

	// Este metodo muestrea todos los datos del programa cada X segundos y los
	// devuelve en un struct. Cada vez que se llame a este metodo se serializaran los
	// datos obtenidos.
	GamingData gameInfo::getGameInfo(WMIqueryServer WMI) {
		bool debug = true;
		GamingData allInfo;

		// Date, Time
		GetLocalTime(&allInfo.st);

		// CPU Info Win32_Processor
		//WMI = checkOS::queryWMI(WMI, "Win32_Processor", "Manufacturer", "string");
		//allInfo.cpuBuilder = WMI.stringResult;
		allInfo.cpuCores = checkCPU::getCPUCores();
		std::cout << "getCPUCores(): " << allInfo.cpuCores << "\n";
		//std::cout << "Tamaño del vector cpuCoresLoad (Antes): " << allInfo.cpuCoresLoad.size() << "\n";
		/////////////////////////////////////////////////////////////////
		//checkCPU::getCPUcoresLoad(WMI, allInfo);
		//std::cout << "Tamaño del vector cpuCoresLoad (Despues): " << allInfo.cpuCoresLoad.size() << "\n";
		///////////////////////////////////////////////////////////
		allInfo.cpuSpeed = checkCPU::getCPUSpeed();

		// GPU Info Win32_VideoController
		WMI = checkOS::queryWMI(WMI, "Win32_VideoController", "Name", "string");
		allInfo.gpuModel = WMI.stringResult;
		WMI = checkOS::queryWMI(WMI, "Win32_VideoController", "AdapterRAM", "uint32");
		float vRamMb = WMI.intResult / MB;
		allInfo.vRAM = (int)vRamMb;
		allInfo.gpuLoad = checkGPU::getGPULoad();
		allInfo.gpuTemp = checkGPU::getGPUTemp();
		
		// RAM Info 
		allInfo.ramSpeed = checkMemory::getRAMSpeed(WMI);
		allInfo.ramSize = checkMemory::getRAMSizeMB();
		allInfo.ramLoad = checkMemory::getRAMLoad();

		// Memoria
		allInfo = checkMemory::getProcessMemInfo(allInfo);
		//std::cout << "GETGAMEINFO Inicializado\n";

		allInfo.cpuModel = checkCPU::getCPU();
		


		//MAX & MIN
		if (allInfo.gpuLoad > allInfo.maxGpuLoad) {
			allInfo.maxGpuLoad = allInfo.gpuLoad;
		}
		if (allInfo.gpuLoad < allInfo.minGpuLoad || allInfo.minGpuLoad == 0) {
			allInfo.minGpuLoad = allInfo.gpuLoad;
		}

		if (allInfo.gpuTemp > allInfo.maxTemp) {
			allInfo.maxTemp = allInfo.gpuTemp;
		}

		if (allInfo.ramLoad > allInfo.maxRamLoad) {
			allInfo.maxRamLoad = allInfo.ramLoad;
		}

		// LLAMADA AL SERIALIZADOR
		
		// Data debug
		if (debug) {
			// OS
			std::cout << "Date: " 
				<< allInfo.st.wDay << "/" 
				<< allInfo.st.wMonth << "/"
				<< allInfo.st.wYear << ". ";
			std::cout << "Time: "
				<< allInfo.st.wHour << ":"
				<< allInfo.st.wMinute << ":"
				<< allInfo.st.wSecond << ":"
				<< allInfo.st.wMilliseconds
				<< "\n";
			//CPU
			//Info solo para la primera vez:
			if (firstTime == 0) {
				std::cout << "CPU: " << allInfo.cpuModel << "\n";
				std::cout << "RAM: " << allInfo.ramSize << " MB @ ";
				std::cout << allInfo.ramSpeed << " Mhz.\n";
				std::cout << "GPU: " << allInfo.gpuModel << "\n";
				std::cout << "Free VRAM: " << allInfo.vRAM << " MB\n";
			}
			checkCPU::getCPUcoresLoad(WMI, allInfo);

			// RAM			
			std::cout << "RAM Load: " << allInfo.ramLoad << " %\n";
			std::cout << "Max Ram Load: " << allInfo.maxRamLoad << " %\n";

			// Memoria
			std::cout << "Used physical memory: " << allInfo.usedMemoryMB << " MB \n";
			std::cout << "Peak memory used: " << allInfo.peakMemoryUsedMB << " MB \n";
			// GPU	
			std::cout << "GPU Load: " << allInfo.gpuLoad << " %\n";
			std::cout << "Max GPU Load: " << allInfo.maxGpuLoad << "\n";
			std::cout << "Min GPU Load: " << allInfo.minGpuLoad << "\n";
			std::cout << "GPU Temp: " << allInfo.gpuTemp << " C\n";
			std::cout << "Max GPU Temp " << allInfo.maxTemp << " C\n";
			std::cout << "\n";
		}

		Profiler::serialize::CSVserialize2(allInfo);

		return allInfo;
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

	WMIqueryServer checkOS::initializeWMI() {
		WMIqueryServer WMI;

		// Step 1: --------------------------------------------------
		// Initialize COM. ------------------------------------------
		//std::cout << "[GetGPUInfo]Initialize COM.\n";
		WMI.hres = CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(WMI.hres))
		{
			cout << "Failed to initialize COM library. Error code = 0x"
				<< hex << WMI.hres << endl;
			std::cout << "[GetGPUInfo] Failed.\n";

			WMI.failStatus = 1; //Mark as failed
			return WMI;                  // Program has failed.
		}

		// Step 2: --------------------------------------------------
		// Set general COM security levels --------------------------
		//std::cout << "[GetGPUInfo]Set COM security levels.\n";
		WMI.hres = CoInitializeSecurity(
			NULL,
			-1,                          // COM authentication
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities 
			NULL                         // Reserved
		);


		if (FAILED(WMI.hres))
		{
			cout << "Failed to initialize security. Error code = 0x"
				<< hex << WMI.hres << endl;
			CoUninitialize();
			std::cout << "[GetGPUInfo] Failed.\n";
			WMI.failStatus = 1; //Mark as failed
			return WMI;
			//return "Failed to retrieve GPUInfo [Security Init ERROR]";  //return 1;                    // Program has failed.
		}

		// Step 3: ---------------------------------------------------
		// Obtain the initial locator to WMI -------------------------
		//std::cout << "[GetGPUInfo]Obtain the initial locator to WMI.\n";
		//IWbemLocator *pLoc = NULL;
		WMI.pLoc = NULL;


		WMI.hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&WMI.pLoc);

		if (FAILED(WMI.hres))
		{
			cout << "Failed to create IWbemLocator object."
				<< " Err code = 0x"
				<< hex << WMI.hres << endl;
			CoUninitialize();
			WMI.failStatus = 1;
			return WMI;
			//return "Failed to retrieve GPUInfo [Security Init ERROR]";  //return 1;                 // Program has failed.
		}

		// Step 4: -----------------------------------------------------
		// Connect to WMI through the IWbemLocator::ConnectServer method
		//std::cout << "[GetGPUInfo] Connect to WMI through the IWbemLocator::Connectserver Method.\n";
		//IWbemServices *pSvc = NULL;
		WMI.pSvc = NULL;

		// Connect to the root\cimv2 namespace with
		// the current user and obtain pointer pSvc
		// to make IWbemServices calls.
		WMI.hres = WMI.pLoc->ConnectServer(
			_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
			NULL,                    // User name. NULL = current user
			NULL,                    // User password. NULL = current
			0,                       // Locale. NULL indicates current
			NULL,                    // Security flags.
			0,                       // Authority (for example, Kerberos)
			0,                       // Context object 
			&WMI.pSvc                    // pointer to IWbemServices proxy
		);

		if (FAILED(WMI.hres))
		{
			cout << "Could not connect. Error code = 0x"
				<< hex << WMI.hres << endl;
			WMI.pLoc->Release();
			CoUninitialize();
			std::cout << "[GetGPUInfo] Failed.\n";
			WMI.failStatus = 1;
			return WMI;
			//return 1;                // Program has failed.
		}

		cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


		// Step 5: --------------------------------------------------
		// Set security levels on the proxy -------------------------
		//std::cout << "[GetGPUInfo]Set security levels on the proxy.\n";
		WMI.hres = CoSetProxyBlanket(
			WMI.pSvc,                        // Indicates the proxy to set
			RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
			RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
			NULL,                        // Server principal name 
			RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
			RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
			NULL,                        // client identity
			EOAC_NONE                    // proxy capabilities 
		);

		if (FAILED(WMI.hres))
		{
			cout << "Could not set proxy blanket. Error code = 0x"
				<< hex << WMI.hres << endl;
			WMI.pSvc->Release();
			WMI.pLoc->Release();
			CoUninitialize();
			std::cout << "[GetGPUInfo] Failed.\n";
			WMI.failStatus = 1; // Mark Fail.
			return WMI;               // Program has failed.
		}

		WMI.failStatus = 0; // Mark Success
		return WMI;
	}

	WMIqueryServer checkOS::getWMIService(WMIqueryServer WMI) {
		HRESULT Hres;
		
		// pLoc
		IWbemLocator* pLocator;
		Hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLocator);
		std::cout << "pLoc initialized.\n";

		// pSvc
		IWbemServices* pService;
		Hres = pLocator->ConnectServer(
			_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
			NULL,                    // User name. NULL = current user
			NULL,                    // User password. NULL = current
			0,                       // Locale. NULL indicates current
			NULL,                    // Security flags.
			0,                       // Authority (for example, Kerberos)
			0,                       // Context object 
			&pService                // pointer to IWbemServices proxy
		);
		std::cout << "pSvc initialized.\n";

		WMI.pLoc = pLocator;
		WMI.pSvc = pService;
		WMI.failStatus = 0;

		return WMI;
	}

	WMIqueryServer checkOS::queryWMI(WMIqueryServer WMI, string wmiclass, string varname, string vartype) {
		
		// Class string to BSTR
		BSTR classQuery = bstr_t("SELECT * FROM ");
		const std::string w32class = wmiclass;
		_bstr_t classname = w32class.c_str();
		classQuery = classQuery + classname;

		// Variable string to wstring to LPCWSTR
		wstring var(varname.begin(), varname.end());
		LPCWSTR variable = var.c_str();

		IEnumWbemClassObject* pEnum = NULL;
		//IWbemServices pService;
		//WMI.pEnumerator = NULL;

		// Class Retrieve
		WMI.hres = WMI.pSvc->ExecQuery(
			bstr_t("WQL"),
			bstr_t(classQuery),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			NULL,
			&WMI.pEnumerator);

		// FailCheck
		if (FAILED(WMI.hres))
		{
			cout << "Query for operating system name failed."
				<< " Error code = 0x"
				<< hex << WMI.hres << endl;
			WMI.pSvc->Release();
			WMI.pLoc->Release();
			CoUninitialize();
			std::cout << "[GetGPUInfo] Failed.\n";
			WMI.failStatus = 1;
			return WMI;               // Program has failed.
		}

		// GET Request variable
		IWbemClassObject *pclsObj = NULL;
		ULONG uReturn = 0;

		while (WMI.pEnumerator)
		{
			HRESULT hr = WMI.pEnumerator->Next(WBEM_INFINITE, 1,
				&pclsObj, &uReturn);

			if (0 == uReturn)
			{
				break;
			}

			VARIANT vtProp;

			// Get the value of the Name property
			//hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
			hr = pclsObj->Get(variable, 0, &vtProp, 0, 0);
			
			// El valor obtenido se guarda en vtProp, que es un tipo variable de datos llamado VARIANT
			// Los tipos de datos utilizados en las librerias Win32 son 
			// uint16, uint32, uint64, string, boolean y datetime
			const char* tipo = typeid(vtProp).name();
			WMI.bstrResult = vtProp.bstrVal;
			WMI.intResult = vtProp.intVal;
			WMI.boolResult = vtProp.boolVal;
			//std::cout << "WMI results for " << wmiclass << ". Variable: " << varname << "\n";

			if (vartype == "string" || vartype == "datetime") {
				///////////////////////
				// Si es un string
				//////////////////////
				//convertir bstr a wstring
				std::wstring ws(vtProp.bstrVal, SysStringLen(vtProp.bstrVal));
				//convertir wstring (UTF16) a string (UTF8)
				std::string resultado = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(ws);
				//Guardar string en el resultado del WMI
				WMI.stringResult = resultado;		
				//std::cout << "WMI stringResult = " << WMI.stringResult << "\n";
			}
			else if (vartype == "uint16" || vartype == "uint32" || vartype == "uint64") {
				WMI.intResult = vtProp.uintVal;
				//std::cout << "WMI intResult = " << WMI.intResult << "\n";
			}
			else if (vartype == "bool") {
				WMI.boolResult = vtProp.boolVal;
				//std::cout << "WMI boolResult = " << WMI.boolResult << "\n";
			}

			VariantClear(&vtProp);

			pclsObj->Release();
		}

		WMI.pEnumerator->Release();
		return WMI;
	}

	void checkOS::closeWMI(WMIqueryServer WMI) {
		// Cleanup
		// ========
		if (WMI.pSvc != NULL) WMI.pSvc->Release();
		if (WMI.pLoc != NULL) WMI.pLoc->Release();
		CoUninitialize();
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
		//std::cout << "CPU Load: " << CPULoad << "\n";
		return CPULoad;
	}

	int checkCPU::getCPUCores() {
#ifdef WIN32
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		int numCores = sysinfo.dwNumberOfProcessors;
		//std::cout << "Cores: " << numCores << "\n";
		return numCores;
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
		//std::cout << "Cores: " << count << "\n";
		return count;
#else
		int numCores = sysconf(_SC_NPROCESSORS_ONLN);
		//std::cout << "Cores: " << count << "\n";
		return numCores;
#endif
	}

	std::string checkCPU::getCPU() {
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

		//std::cout << "Cpu Model: " << CPUBrandString;
		return CPUBrandString;
	}

	int checkCPU::getCPUSpeed() {
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

			//std::cout << "CPU Speed: " << (double)dwMHz << " MHz." << std::endl;
			return (int)dwMHz;
	}

	void checkCPU::getCPUcoresLoad(WMIqueryServer WMI, GamingData& allInfo)
	{
		//std::cout << "INICIALIZACION\n";
		HRESULT hres;
		IEnumWbemClassObject* pEnumerator = NULL;
		IWbemClassObject* pclsObj = NULL;
		std::vector<int> loads;
		int i = 0;
		//std::cout << "QUERY\n";
		hres = WMI.pSvc->ExecQuery(bstr_t("WQL"),
			bstr_t("SELECT * FROM Win32_PerfFormattedData_PerfOS_Processor"),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			NULL,
			&pEnumerator);


		if (FAILED(WMI.hres)) {
			cout << "Query for operating system name failed."
				<< " Error code = 0x"
				<< hex << WMI.hres << endl;
			WMI.pSvc->Release();
			WMI.pLoc->Release();
			CoUninitialize();
			// Program has failed.
		}
		int vueltasBucle = 0;
		ULONG uReturn = 0;
		//std::cout << "WHILE{\n";
		while (pEnumerator) {
			//std::cout << "     pENUMERATOR NEXT\n";
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
				&pclsObj, &uReturn);

			if (0 == uReturn) {
				break;
			}

			VARIANT vtProp;

			// Get the value of the Name property
			//hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
			//std::cout << "     GetValue\n";
			hr = pclsObj->Get(L"PercentProcessorTime", 0, &vtProp, 0, 0);
			//wcout << "CPU " << i << " : " << vtProp.bstrVal << " %" << endl;

			//Transformar bstr a string y de string a int para guardarlo en el vector
			//convertir bstr a wstring
			//std::cout << "     Conversion\n";
			std::wstring ws(vtProp.bstrVal, SysStringLen(vtProp.bstrVal));
			//convertir wstring (UTF16) a string (UTF8)
			std::string resultado = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(ws);
			//std::cout << "String convertido: " << resultado << "\n";
			// convertir string a int
			int res = std::stoi(resultado);
			//std::cout << "    Query Result CPU " << i << ": " << res << "\n";
			std::cout << "CPU " << i << ": " << res << "\n";
			//Guardar en vector
			//std::cout << "     EmplaceBackLoads\n";
			loads.emplace_back(res);
			/*
			std::cout << "int convertido: " << res << "\n";
			*/		

			VariantClear(&vtProp);

			//IMPORTANT!!
			pclsObj->Release();

			vueltasBucle++;
			i++;
		}
		allInfo.cpuCoresLoad = loads;

		/*
		std::cout << "} While End: Vueltas Bucle: " << vueltasBucle << "\n";

		std::cout << "GI struct equal\n";
		std::cout << "COUT\n";
		for (int j = 0; j < loads.size(); j++) {
			std::cout << "CPU " << j << ": " << loads.at(j) << "%\n";
		}
		*/
		//if (loads.size() > 0){
		//	std::cout << "Overall CPU Load: " << loads.at(loads.size()-1) << "% \n";
		//}
		//std::cout << "}\n";
	}

	/////////////////////////////////////////////
	//
	// Memory Methods
	//
	/////////////////////////////////////////////
	void checkMemory::showPhysicalMemoryInfo(WMIqueryServer WMI) {
		/*
		WMI = checkOS::queryWMI(WMI, "Win32_PhysicalMemory", "Name", "string");
		std::cout << "[PhysicalMemory] Name: " << WMI.stringResult << "\n";

		WMI = checkOS::queryWMI(WMI, "Win32_PhysicalMemory", "Model", "string");
		std::cout << "[PhysicalMemory] Model: " << WMI.stringResult << "\n";

		WMI = checkOS::queryWMI(WMI, "Win32_PhysicalMemory", "Manufacturer", "string");
		std::cout << "[PhysicalMemory] Manufacturer: " << WMI.stringResult << "\n";
		*/

		WMI = checkOS::queryWMI(WMI, "Win32_PhysicalMemory", "Capacity", "uint64");
		float totalRam = WMI.intResult / MB;
		std::cout << "[PhysicalMemory] Capacity: " << totalRam << "\n";

		WMI = checkOS::queryWMI(WMI, "Win32_PhysicalMemory", "Speed", "uint32");
		std::cout << "[PhysicalMemory] Speed: " << WMI.intResult << " Mhz\n";
	}

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

		//Used Mem
		DWORDLONG phyMemUsed = statex.ullTotalPhys - statex.ullAvailPhys;

		// GygaBytes
		_tprintf(TEXT("There is  %*ld  % of memory in use.\n"),
			WIDTH, statex.dwMemoryLoad);
		_tprintf(TEXT("There are %*I64d total GB of physical memory.\n"),
			WIDTH, statex.ullTotalPhys / GB);
		_tprintf(TEXT("There are %*I64d free GB of physical memory.\n"),
			WIDTH, statex.ullAvailPhys / GB);
		_tprintf(TEXT("There are %*I64d used GB of physical memory.\n"),
			WIDTH, phyMemUsed / GB);
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

	GamingData checkMemory::getProcessMemInfo(GamingData gd)
	{
		HANDLE hProcess;
		PROCESS_MEMORY_COUNTERS pmc;
		
		BOOL result = GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
		SIZE_T usedPhy = pmc.WorkingSetSize/ MB;
		SIZE_T peakusedPhy = pmc.PeakWorkingSetSize / MB;

		gd.usedMemoryMB = usedPhy;
		gd.peakMemoryUsedMB = peakusedPhy;

		return gd;

	}

	int checkMemory::getRAMSpeed(WMIqueryServer WMI) {
		WMI = checkOS::queryWMI(WMI, "Win32_PhysicalMemory", "Speed", "uint32");
		//std::cout << "[PhysicalMemory] Speed: " << WMI.intResult << " Mhz\n";
		return WMI.intResult;
	}

	int checkMemory::getRAMSizeMB() {
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx(&statex);
		DWORDLONG phyMemUsed = statex.ullTotalPhys - statex.ullAvailPhys;
		int size = statex.ullTotalPhys / MB;
		return size;
	}

	int checkMemory::getRAMSizeGB() {
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx(&statex);
		DWORDLONG phyMemUsed = statex.ullTotalPhys - statex.ullAvailPhys;
		int size = statex.ullTotalPhys / GB;
		return size;
	}

	int checkMemory::getRAMLoad() {
		int load = 0;

		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx(&statex);

		load = statex.dwMemoryLoad;
		//std::cout << "Load: " << load << " %\n";

		return load;
	}

	/////////////////////////////////////////////
	//
	// GPU Methods
	//
	/////////////////////////////////////////////
	void checkGPU::showVideoControllerInfo(WMIqueryServer WMI) {
		WMI = checkOS::queryWMI(WMI, "Win32_VideoController", "AdapterCompatibility", "string");
		std::cout << "[VideoController] Adapter Compability: " << WMI.stringResult << "\n";

		WMI = checkOS::queryWMI(WMI, "Win32_VideoController", "Description", "string");
		std::cout << "[VideoController] Description: " << WMI.stringResult << "\n";

		WMI = checkOS::queryWMI(WMI, "Win32_VideoController", "DriverVersion", "string");
		std::cout << "[VideoController] DriverVersion: " << WMI.stringResult << "\n";

		WMI = checkOS::queryWMI(WMI, "Win32_VideoController", "AdapterRAM", "uint32");
		float vRamMb = WMI.intResult / MB;
		std::cout << "[VideoController] Adapter RAM: " << vRamMb << "\n";

		WMI = checkOS::queryWMI(WMI, "Win32_VideoController", "Status", "string");
		std::cout << "[VideoController] Status: " << WMI.stringResult << "\n";

		WMI = checkOS::queryWMI(WMI, "Win32_VideoController", "MaxMemorySupported", "uint32");
		std::cout << "[VideoController] Max Memory Supported: " << WMI.intResult << "\n";
	}

	string checkGPU::GetGPUModel(WMIqueryServer WMI) {
		
		WMI = checkOS::queryWMI(WMI, "Win32_VideoController", "Name", "string");
		
		//if (WMI.failStatus == 0) std::cout << "GPU Name: " << WMI.queryResult << "\n";
		//else if (WMI.failStatus == 1) std::cout << "ERROR retrieving GPU Name info.\n";

		return WMI.stringResult;
	}

	//Este metodo se llamara 1 vez cada sec obteniendo la informacin de los frames y 
	//actualizando su valor.
	void checkGPU::GetFps()
	{
		
		std::cout << "\n/// FPS: ///" << i << "\n";
		i = 0;
		
	}
	
	//Este metodo se llamara en el render
	void checkGPU::countFrames()
	{
		i++;
	}

	void testVariables::pruebita()
	{
		//x = 3;
		//x++;
		//std::cout << "\n/// prueba variables: ///" << x << "\n";

	}

	int checkGPU::getGPULoad() {
		HMODULE hmod = LoadLibraryA(NVAPI_DLL);
		
		if (hmod == NULL)
		{
			std::cerr << "Couldn't find nvapi.dll" << std::endl;
			return -1;
		}

		// nvapi.dll internal function pointers
		NvAPI_QueryInterface_t      NvAPI_QueryInterface = NULL;
		NvAPI_Initialize_t          NvAPI_Initialize = NULL;
		NvAPI_EnumPhysicalGPUs_t    NvAPI_EnumPhysicalGPUs = NULL;
		NvAPI_GPU_GetUsages_t       NvAPI_GPU_GetUsages = NULL;

		// nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
		NvAPI_QueryInterface = (NvAPI_QueryInterface_t)GetProcAddress(hmod, "nvapi_QueryInterface");

		// some useful internal functions that aren't exported by nvapi.dll
		NvAPI_Initialize = (NvAPI_Initialize_t)(*NvAPI_QueryInterface)(0x0150E828);
		NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t)(*NvAPI_QueryInterface)(0xE5AC921F);
		NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t)(*NvAPI_QueryInterface)(0x189A1FDF);

		if (NvAPI_Initialize == NULL || NvAPI_EnumPhysicalGPUs == NULL ||
			NvAPI_EnumPhysicalGPUs == NULL || NvAPI_GPU_GetUsages == NULL)
		{
			std::cerr << "Couldn't get functions in nvapi.dll" << std::endl;
			return -2;
		}

		// initialize NvAPI library, call it once before calling any other NvAPI functions
		(*NvAPI_Initialize)();

		// Defines come variables
		int          gpuCount = 0;
		int         *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
		unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };

		// gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
		gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;

		(*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);

		// print GPU usage every second
		/*
		for (int i = 0; i < 100; i++)
		{
			(*NvAPI_GPU_GetUsages)(gpuHandles[0], gpuUsages);
			int usage = gpuUsages[3];
			std::cout << "GPU Usage: " << usage <<  "%" << std::endl;
			Sleep(1000);
		}
		*/
		int u = (*NvAPI_GPU_GetUsages)(gpuHandles[0], gpuUsages);
		int usage = gpuUsages[3];

		return usage;
	};

	int checkGPU::getGPUTemp() {
		HMODULE hmod = LoadLibraryA(NVAPI_DLL);

		if (hmod == NULL)
		{
			std::cerr << "Couldn't find nvapi.dll" << std::endl;
			return -1;
		}

		// nvapi.dll internal function pointers
		NvAPI_QueryInterface_t      NvAPI_QueryInterface = NULL;
		NvAPI_Initialize_t          NvAPI_Initialize = NULL;
		NvAPI_EnumPhysicalGPUs_t    NvAPI_EnumPhysicalGPUs = NULL;
		NvAPI_GPU_GetUsages_t       NvAPI_GPU_GetUsages = NULL;
		NvAPI_GPU_GetThermalSettings_t NvAPI_GPU_GetThermalSettings = NULL;

		// nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
		NvAPI_QueryInterface = (NvAPI_QueryInterface_t)GetProcAddress(hmod, "nvapi_QueryInterface");

		// some useful internal functions that aren't exported by nvapi.dll
		NvAPI_Initialize = (NvAPI_Initialize_t)(*NvAPI_QueryInterface)(0x0150E828);
		NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t)(*NvAPI_QueryInterface)(0xE5AC921F);
		NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t)(*NvAPI_QueryInterface)(0x189A1FDF);
		NvAPI_GPU_GetThermalSettings = (NvAPI_GPU_GetThermalSettings_t)(*NvAPI_QueryInterface)(0xE3640A56);

		if (NvAPI_Initialize == NULL || NvAPI_EnumPhysicalGPUs == NULL ||
			NvAPI_EnumPhysicalGPUs == NULL || NvAPI_GPU_GetUsages == NULL || NvAPI_GPU_GetThermalSettings == NULL)
		{
			std::cerr << "Couldn't get functions in nvapi.dll" << std::endl;
			return -2;
		}
		
		//Initialize service
		(*NvAPI_Initialize)();
		NV_GPU_THERMAL_SETTINGS nvgts;
		int          gpuCount = 0;
		int 		 gpuTemp;
		int         *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
		unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };

		// gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
		gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;

		(*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);
		nvgts.version = sizeof(NV_GPU_THERMAL_SETTINGS) | (1 << 16);
		nvgts.count = 0;
		nvgts.sensor[0].controller = NVAPI_THERMAL_CONTROLLER_UNKNOWN;
		nvgts.sensor[0].target = NVAPI_THERMAL_TARGET_GPU;

		// DO QUERYS HERE //
		(*NvAPI_GPU_GetThermalSettings)(gpuHandles[0], 0, &nvgts);
		int temp = nvgts.sensor[0].currentTemp;
		//printf("GPU Temp: %d\n", nvgts.sensor[0].currentTemp);
		
		return temp;
	}

	/////////////////////////////////////////////
	//
	// Serialize Methods
	//
	/////////////////////////////////////////////

	void serialize::CSVserialize(GamingData gi) {
		if (!firstTime) {
			int indexFile = 0;
			std::string sesLoc = "SessionIndex.txt";
			ifstream f;
			f.open(sesLoc);
			bool exist = f.good();
			if (!exist) {
				f.close();
				ofstream sesionFile;
				sesionFile.open(sesLoc);
				sesionFile << "1";
				sesionFile.close();
				indx = 0;
			}
			else {
				std::string aux;
				getline(f, aux);
				indexFile = std::stoi(aux);
				f.close();
				////////////////
				ofstream sesionFile2;
				sesionFile2.open(sesLoc, ofstream::ate);
				int newAux = indexFile + 1;
				sesionFile2 << newAux;
				sesionFile2.close();
				indx = indexFile;
			}
		}
		std::string fileName = "session_";
		std::string extName = ".csv";
		std::string fullName = fileName + std::to_string(indx) + extName;

		// IMPLEMENTAR SERIALIZACION
		std::cout << "CSVSerialize\n";
		ofstream file;
		file.open(fullName, ofstream::app);
		//Permanent Data
		if (!firstTime) {
			CSVPermanentInfo(gi, file);
			firstTime = 1;
		}
		//CPU
		CSVIntSerialize(gi.cpuLoad, "CPU_LOAD", file, gi);
		CSVDayStamp(gi, file);
		addComeToCSV(file);
		CSVHourStamp(gi, file);
		addComeToCSV(file);
		file << ",CPU_CORES_INFO";
		for (int i = 0; i < gi.cpuCoresLoad.size(); i++) {
			std::string aux = ",CORE_";
			aux += std::to_string(i);
			file << aux << ',' << gi.cpuCoresLoad[i];
		}
		file << "\n";



		//GPU
		CSVIntSerialize(gi.gpuLoad, "GPU_LOAD", file, gi);
		CSVIntSerialize(gi.minGpuLoad, "MIN_GPU_LOAD", file, gi);
		CSVIntSerialize(gi.maxGpuLoad, "MAX_GPU_LOAD", file, gi);

		//Min/Maxes
		CSVIntSerialize(gi.usedMemoryMB, "USED_MEMORY", file, gi);
		CSVIntSerialize(gi.peakMemoryUsedMB, "PEEK_MEMORY", file, gi);


		CSVIntSerialize(gi.ramLoad, "RAM_LOAD", file, gi);
		CSVIntSerialize(gi.minRamLoad, "MIN_RAM_LOAD", file, gi);
		CSVIntSerialize(gi.maxRamLoad, "MAX_RAM_LOAD", file, gi);

		// Temperatura
		CSVIntSerialize(gi.gpuTemp, "GPU_TEMP", file, gi);
		//CSVIntSerialize(gi.minTemp, "MIN_TEMP", file, gi);
		CSVIntSerialize(gi.maxTemp, "MAX_TEMP", file, gi);

		//RAM
		file << "\n";
		file.close();
	}
	void serialize::CSVserialize2(GamingData gi) {
		if (!firstTime) {
			int indexFile = 0;
			std::string sesLoc = "SessionIndex.txt";
			ifstream f;
			f.open(sesLoc);
			bool exist = f.good();
			if (!exist) {
				f.close();
				ofstream sesionFile;
				sesionFile.open(sesLoc);
				sesionFile << "1";
				sesionFile.close();
				indx = 0;
			}
			else {
				std::string aux;
				getline(f, aux);
				indexFile = std::stoi(aux);
				f.close();
				////////////////
				ofstream sesionFile2;
				sesionFile2.open(sesLoc, ofstream::ate);
				int newAux = indexFile + 1;
				sesionFile2 << newAux;
				sesionFile2.close();
				indx = indexFile;
			}
		}
		std::string fileName = "session_";
		std::string extName = ".csv";
		std::string fullName = fileName + std::to_string(indx) + extName;

		// IMPLEMENTAR SERIALIZACION
		std::cout << "CSVSerialize\n";
		ofstream file;
		file.open(fullName, ofstream::app);
		//Permanent Data
		if (!firstTime) {
			//CSVPermanentInfo(gi, file);
			file << "DAY,TIME,CORES,CPU_LOAD,GPU_LOAD,MIN_GPU_LOAD,MAX_GPU_LOAD,USED_MEMORY,PEEK_MEMORY,RAM_LOAD,MIN_RAM_LOAD,MAX_RAM_LOAD,GPU_TEMP,MIN_TEMP,MAX_TEMP\n";
			firstTime = 1;
		}
		//CSVTimeStamp(gi, file);
		//Dia y Hora
		CSVDayStamp(gi, file);
		addComeToCSV(file);
		CSVHourStamp(gi, file);
		addComeToCSV(file);

		//CPU_LOAD
		//SVIntSerialize2(gi.cpuLoad, file);
		//addComeToCSV(file);
		//file << ",CPU_CORES_INFO";
		for (int i = 0; i < gi.cpuCoresLoad.size(); i++) {
			if (i != gi.cpuCoresLoad.size()) {
				file << gi.cpuCoresLoad[i] << ';';
			}
			else {
				addComeToCSV(file);
				file << gi.cpuCoresLoad[i];
			}

		}
		addComeToCSV(file);

		//GPU
		CSVIntSerialize2(gi.gpuLoad, file); addComeToCSV(file);
		CSVIntSerialize2(gi.minGpuLoad, file); addComeToCSV(file);
		CSVIntSerialize2(gi.maxGpuLoad, file); addComeToCSV(file);

		//Min/Maxes
		CSVIntSerialize2(gi.usedMemoryMB, file); addComeToCSV(file);
		CSVIntSerialize2(gi.peakMemoryUsedMB, file); addComeToCSV(file);


		CSVIntSerialize2(gi.ramLoad, file); addComeToCSV(file);
		//CSVIntSerialize2(gi.minRamLoad, file); addComeToCSV(file);
		CSVIntSerialize2(gi.maxRamLoad, file); addComeToCSV(file);

		// Temperatura
		CSVIntSerialize2(gi.gpuTemp, file); addComeToCSV(file);
		CSVIntSerialize2(gi.minTemp, file); addComeToCSV(file);
		CSVIntSerialize2(gi.maxTemp, file);

		//RAM
		file << "\n";
		file.close();
	}

	void serialize::CSVCores(GamingData gd, ofstream& file)
	{
		int _fullsize = gd.cpuCoresLoad.size();
		int _size = _fullsize / 2;
		for (int i = 0; i < _fullsize - 1; i += 2) {
			for (int j = 0; j < 2; j++) {
				int aux = i + j;
				file << gd.cpuCoresLoad[aux] << " ;";
				if (j == 1) {
					file << "\n;";
				}
			}
		}
	}
	GamingData serialize::CSVDeserialize()
	{
		ifstream file;
		GamingData gi;
		file.open("example.csv");
		char skip;
		std::string endLineSkip;
		std::string aux;
		std::vector<int> v;
		for (int i = 0; i < 3; i++) { //Dia, mes, año
			std::getline(file, aux, '/');
			if (i == 0)
				gi.st.wDay = std::stoi(aux);
			else if (i == 1)
				gi.st.wMonth = std::stoi(aux);
			else
				gi.st.wYear = std::stoi(aux);
		}
		getline(file, endLineSkip, ';');
		for (int i = 0; i < 4; i++) { //Hora, min, sec, milisec
			if (i < 3) {
				std::getline(file, aux, ':');
				if (i == 0)
					gi.st.wHour = std::stoi(aux);
				else if (i == 1)
					gi.st.wMinute = std::stoi(aux);
				else if (i == 2)
					gi.st.wSecond = std::stoi(aux);
			}
			else {
				std::getline(file, aux, ';');
				gi.st.wMilliseconds = std::stoi(aux);
			}
		}
		getline(file, endLineSkip, ';'); //Header
		//CPU INFO
		//std::getline(file, aux, ';'); //Primero descripcion con previo salto de linea
		//std::getline(file, aux, ';'); //Luego el valor
		//gi.cpuCores = std::stoi(aux);
		CSVSingleItemDeserialize(gi.cpuCores, file, ';');

		//std::getline(file, aux, ';');
		//std::getline(file, aux, ';');
		//gi.cpuSpeed = std::stoi(aux);
		CSVSingleItemDeserialize(gi.cpuSpeed, file, ';');

		//std::getline(file, aux, ';');
		//std::getline(file, aux, ';');
		//gi.cpuLoad = std::stoi(aux);
		CSVSingleItemDeserialize(gi.cpuLoad, file, ';');

		getline(file, aux, ';');
		for (int i = 0; i < gi.cpuCores; i += 2) {
			for (int j = 0; j < 2; j++) {
				int mix = i + j;
				std::getline(file, aux, ';');
				v.push_back(std::stoi(aux));
			}
			getline(file, aux, ';');
		}
		//GPU INFO
		getline(file, endLineSkip, ';'); //header

		getline(file, aux, ';');
		getline(file, aux, ';');
		gi.gpuModel = aux;

		//getline(file, aux, ';');
		//getline(file, aux, ';');
		//gi.gpuTemp = std::stoi(aux);
		CSVSingleItemDeserialize(gi.gpuTemp, file, ';');

		//getline(file, aux, ';');
		//getline(file, aux, ';');
		//gi.gpuLoad = std::stoi(aux);
		CSVSingleItemDeserialize(gi.gpuLoad, file, ';');

		//getline(file, aux, ';');
		//getline(file, aux, ';');
		//gi.vRAM = std::stoi(aux);
		CSVSingleItemDeserialize(gi.vRAM, file, ';');
		
		
		// MINIMOS Y MAXIMOS AQUI
		CSVSingleItemDeserialize(gi.usedMemoryMB, file, ';');
		CSVSingleItemDeserialize(gi.peakMemoryUsedMB, file, ';');
		CSVSingleItemDeserialize(gi.minGpuLoad, file, ';');
		CSVSingleItemDeserialize(gi.maxGpuLoad, file, ';');
		CSVSingleItemDeserialize(gi.minRamLoad, file, ';');
		CSVSingleItemDeserialize(gi.maxRamLoad, file, ';');
		CSVSingleItemDeserialize(gi.minTemp, file, ';');
		CSVSingleItemDeserialize(gi.maxTemp, file, ';');

		//RAM INFO
		getline(file, endLineSkip, ';'); //Header

		getline(file, aux, ';');
		getline(file, aux, ';');
		gi.ramSize = std::stoi(aux);

		getline(file, aux, ';');
		getline(file, aux, ';');
		gi.ramSpeed = std::stoi(aux);

		getline(file, aux, ';');
		getline(file, aux, ';');
		gi.ramLoad = std::stoi(aux);

		getline(file, endLineSkip, ';');

		file.close();
		return GamingData();
	}
	void serialize::CSVSingleItemDeserialize(int& field, std::ifstream& file, char delimitator)
	{
		std::string auxiliar;
		getline(file, auxiliar, delimitator);
		getline(file, auxiliar, delimitator);
		field = std::stoi(auxiliar);
	}

	void serialize::CSVDayStamp(GamingData gi, std::ofstream& file)	{
		file << gi.st.wDay << "/" << gi.st.wMonth << "/" << gi.st.wYear;
	}

	void serialize::CSVHourStamp(GamingData gi, std::ofstream& file) {
		file << gi.st.wHour << ":" << gi.st.wMinute << ":" << gi.st.wSecond << ":" << gi.st.wMilliseconds;
	}
	
	void serialize::CSVIntSerialize(int value, std::string info, std::ofstream& file, GamingData gi) {
		//CSVTimeStamp(gi, file);
		file <<',' <<info << ',' << value<<"\n";
	}
	void serialize::CSVIntSerialize2(int value, std::ofstream& file) {
		file << value;
	}
	void serialize::addComeToCSV(std::ofstream& file)
	{
		file << ',';
	}
	void serialize::CSVPermanentInfo(GamingData gi, std::ofstream& file) {
		//ram speed y size  vram nº cores y speedcpu

		file << "DIA, HORA ,HARDWARE_INFO,VALUE\n";
		CSVDayStamp(gi, file);
		addComeToCSV(file);
		CSVHourStamp(gi, file);
		addComeToCSV(file);
		file << ',' << "HARDWARE_INFO," << "CPU_MODEL," << gi.cpuModel << ',' << "CPU_CORES," << gi.cpuCores << ',' << "CPU_SPEED," 
			<< gi.cpuSpeed <<',' <<"GPU_MODEL," << gi.gpuModel << ',' << "vRAM," << gi.vRAM << ',' << "RAM_SIZE," << gi.ramSize << ',' << "RAM_SPEED," << gi.ramSpeed << "\n";
	}
}