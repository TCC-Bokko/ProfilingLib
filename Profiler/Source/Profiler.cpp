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
#include <codecvt>
#include <typeinfo>
#include <cstdio>
using namespace std;

// GPU (Obtained from NvAPI, Nvidia software)
// magic numbers, do not change them
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  34
#define NVAPI_MAX_THERMAL_SENSORS_PER_GPU 3

typedef enum
{
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

typedef enum
{
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

typedef struct
{
	int   version;                //!< structure version 
	int   count;                  //!< number of associated thermal sensors
	struct
	{
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

	void Testing::testMSG() {
		std::cout << "Esto funciona.\n";
	}

	/////////////////////////////////////////////
	//
	// GameInfo Methods
	//
	/////////////////////////////////////////////

	// Devuelve en un unico struct todos los datos importantes
	GamingData gameInfo::getGameInfo(WMIqueryServer WMI) {
		// La idea de este metodo es muestrear todos los datos del programa cada X segundos
		// y devolverlos en un struct. A ser posible cada vez que se llame a este metodo
		// estaría bien serializar los datos.

		bool debug = true;
		GamingData allInfo;

		// Date, Time
		GetLocalTime(&allInfo.st);

		// CPU Info Win32_Processor
		//WMI = checkOS::queryWMI(WMI, "Win32_Processor", "Manufacturer", "string");
		//allInfo.cpuBuilder = WMI.stringResult;
		allInfo.cpuCores = checkCPU::getCPUCores();
		std::cout << "getCPUCores(): " << allInfo.cpuCores << "\n";
		std::cout << "Tamaño del vector cpuCoresLoad (Antes): " << allInfo.cpuCoresLoad.size() << "\n";
		/////////////////////////////////////////////////////////////////
		allInfo.cpuCoresLoad = checkCPU::getCPUcoresLoad(WMI);
		std::cout << "Tamaño del vector cpuCoresLoad (Despues): " << allInfo.cpuCoresLoad.size() << "\n";
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
		
		// Memory Info 
		allInfo.ramSpeed = checkMemory::getRAMSpeed(WMI);
		allInfo.ramSize = checkMemory::getRAMSizeMB();
		allInfo.ramLoad = checkMemory::getRAMLoad();

		std::cout << "GETGAMEINFO Inicializado\n";

		// LLAMADA AL SERIALIZADOR
	    Profiler::serialize::CSVserialize(allInfo);

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
			// CPU
			//std::cout << "CPU Model: " << allInfo.cpuModel << "\n";
			//std::cout << "CPU Builder: " << allInfo.cpuBuilder << "\n";
			std::cout << "CPU Cores: " << allInfo.cpuCores << " Cores @ ";
			std::cout << allInfo.cpuSpeed << " Mhz\n";

			std::cout << "Tamaño del vector cpuCoresLoad: " << allInfo.cpuCoresLoad.size() << "\n";
			int size = allInfo.cpuCoresLoad.size();
			if (size > 0){
				std::cout << "CPU Load: " << allInfo.cpuCoresLoad.at(allInfo.cpuCoresLoad.size() - 1) << " % (Average)\n";
				for (int i = 0; i < allInfo.cpuCoresLoad.size()-1; i++) {
					std::cout << "Core " << i << ": " << allInfo.cpuCoresLoad.at(i) << " %\n";
				}
			}
			 
			//std::cout << "CPU load: "
				//<< allInfo.cpuLoad << " %\n";
			// RAM
			std::cout << "RAM: " << allInfo.ramSize << " MB @ ";
			std::cout << allInfo.ramSpeed << " Mhz.\n";
			std::cout << "RAM Load: " << allInfo.ramLoad << " %\n";
			// GPU
			std::cout << "GPU: " << allInfo.gpuModel << "\n";
			std::cout << "Free VRAM: " << allInfo.vRAM << " MB\n";
			std::cout << "GPU Load: " << allInfo.gpuLoad << " %\n";
			std::cout << "GPU Temp: " << allInfo.gpuTemp << " C\n";
			//std::cout << "GPU load: " << allInfo.gpuLoad << " %\n";
		}

		

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

	std::vector<int> checkCPU::getCPUcoresLoad(WMIqueryServer WMI)
	{
		HRESULT hres;
		IEnumWbemClassObject* pEnumerator = NULL;
		IWbemClassObject* pclsObj = NULL;
		std::vector<int> loads;
		int i = 1;
		
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

		ULONG uReturn = 0;

		while (pEnumerator) {
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
				&pclsObj, &uReturn);

			if (0 == uReturn) {
				break;
			}

			VARIANT vtProp;

			// Get the value of the Name property
			//hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
			hr = pclsObj->Get(L"PercentProcessorTime", 0, &vtProp, 0, 0);
			//wcout << "CPU " << i << " : " << vtProp.bstrVal << " %" << endl;

			//Transformar bstr a string y de string a int para guardarlo en el vector
			//convertir bstr a wstring
			std::wstring ws(vtProp.bstrVal, SysStringLen(vtProp.bstrVal));
			//convertir wstring (UTF16) a string (UTF8)
			std::string resultado = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(ws);
			//std::cout << "String convertido: " << resultado << "\n";
			// convertir string a int
			int res = std::stoi(resultado);
			//Guardar en vector
			loads.emplace_back(res);
			/*
			std::cout << "int convertido: " << res << "\n";
			std::cout << "Loads: {";
			for (int j = 0; j < loads.size(); j++) {
				std::cout << loads.at(j) << ",";
			}
			std::cout << "}\n";
			*/

			VariantClear(&vtProp);

			//IMPORTANT!!
			pclsObj->Release();

			i++;
		}
		return loads;
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

	void checkMemory::getProcessMemInfo()
	{
		HANDLE hProcess;
		PROCESS_MEMORY_COUNTERS pmc;
		
		BOOL result = GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
		SIZE_T usedPhy = pmc.WorkingSetSize/ MB;
		SIZE_T peakusedPhy = pmc.PeakWorkingSetSize / MB;

		if (result) {
			printf("-- Current Process Info --\n");
			_tprintf(TEXT("There are %*I64d MB used of physical memory by this process.\n"),
				WIDTH, usedPhy);
			_tprintf(TEXT("The Peak was %*I64d MB used of physical memory by this process.\n"),
				WIDTH, peakusedPhy);
			//Add more info if wanted
			printf("--------------------------\n");
		}
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
		(*NvAPI_GPU_GetUsages)(gpuHandles[0], gpuUsages);
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

		// IMPLEMENTAR SERIALIZACION
		std::cout << "CSVSerialize\n";
		ofstream file;
		file.open("example.csv",ofstream::app);
		//DATE AND MOTHERBOARD
		file <<gi.st.wDay<<"/"<<gi.st.wMonth<<"/"<<gi.st.wYear<<" -> "<<gi.st.wHour<<":"<<gi.st.wMinute<<":"<<gi.st.wSecond<<":"<<gi.st.wMilliseconds <<"\n";
		//file << "MOTHERBOARD;" << gi.motherboardModel<<";\n";

		//CPU
		file << "CPU INFO" << ";\n"; //Header
		file << "CPU Cores;" << gi.cpuCores <<";;" << "CPU SPEED;" << gi.cpuSpeed << "\n";
		file << "CPU LOAD;" << gi.cpuLoad << ";\n\n";
		CSVCores(gi, file);
		//file << "CPU MODEL;" << gi.cpuModel + ";;" << "CPU BUILDER;" << gi.cpuBuilder << "\n";
		//GPU
		//Header
		file << "GPU INFO" << ";\n"; 
		file << "GPU MODEL;" << gi.gpuModel << ";;" << "GPU TEMP;" << gi.gpuTemp << "\n";
		file << "GPU LOAD;" << gi.gpuLoad <<";;" << "GPU VRAM;" << gi.vRAM << "\n\n";
		//RAM
		//Header
		file << "RAM INFO" << ";\n"; 
		file << "RAM SIZE;"<< gi.ramSize << ";; RAM SPEED;" << gi.ramSpeed << "\n";
		file << "RAM LOAD;" << gi.ramLoad << ";\n\n\n\n";
		file.close();
	}
	void serialize::CSVCores(GamingData gd, ofstream& file)
	{
		int _fullsize = gd.cpuCoresLoad.size();
		int _size = _fullsize / 2;
		for (int i = 0; i < _size; i++) {
			for (int j = 0; j < 2; j++) {
				int aux = i + j;
				file << gd.cpuCoresLoad[aux] << ";";
				if (j == 1) {
					file << ";\n";
				}
			}
		}
	}
	;

}

