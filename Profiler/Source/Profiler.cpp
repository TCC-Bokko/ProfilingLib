// Profiler.cpp : Define las funciones exportadas de la aplicación DLL.
// https://www.cprogramming.com/snippets/source-code/find-the-number-of-cpu-cores-for-windows-mac-or-linux
// This snippet submitted by Dirk-Jan Kroon on 2010-06-09. It has been viewed 23370 times.

#include "Profiler.h"

// OTHER
#include <Psapi.h>
#include <codecvt>
#include <typeinfo>
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
		allInfo.cpuSpeed = checkCPU::getCPUSpeed();

		// GPU Info Win32_VideoController
		WMI = checkOS::queryWMI(WMI, "Win32_VideoController", "Name", "string");
		allInfo.gpuModel = WMI.stringResult;
		WMI = checkOS::queryWMI(WMI, "Win32_VideoController", "AdapterRAM", "uint32");
		float vRamMb = WMI.intResult / MB;
		allInfo.vRAM = (int)vRamMb;
		
		// Memory Info 
		allInfo.ramSpeed = checkMemory::getRAMSpeed(WMI);
		allInfo.ramSize = checkMemory::getRAMSizeMB();
		//allInfo.ramLoad

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
			//std::cout << "CPU load: "
				//<< allInfo.cpuLoad << " %\n";
			// RAM
			std::cout << "RAM: " << allInfo.ramSize << " MB @ ";
			std::cout << allInfo.ramSpeed << " Mhz.\n";
			// GPU
			std::cout << "GPU: " << allInfo.gpuModel << "\n";
			std::cout << "Free VRAM: " << allInfo.vRAM << " MB\n";
			//std::cout << "GPU load: " << allInfo.gpuLoad << " %\n";
		}

		// LLAMADA AL SERIALIZADOR
		// Profiler::serialize::CSVserialize(allInfo);

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

	WMIqueryServer checkOS::queryWMI(WMIqueryServer WMI, string wmiclass, string varname, string vartype) {
		
		// Class string to BSTR
		BSTR classQuery = bstr_t("SELECT * FROM ");
		const std::string w32class = wmiclass;
		_bstr_t classname = w32class.c_str();
		classQuery = classQuery + classname;

		// Variable string to wstring to LPCWSTR
		wstring var(varname.begin(), varname.end());
		LPCWSTR variable = var.c_str();
		WMI.pEnumerator = NULL;

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
		_tprintf(TEXT("There is  %*ld percent of memory in use.\n"),
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


	/////////////////////////////////////////////
	//
	// Serialize Methods
	//
	/////////////////////////////////////////////

	void serialize::CSVserialize(GamingData gi) {

		// IMPLEMENTAR SERIALIZACION
		std::cout << "CSVSerialize\n";
	};

}

