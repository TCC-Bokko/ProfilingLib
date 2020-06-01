#pragma once

#include "windows.h"

namespace Profiler {
	class Testing {
		public:
			// Shows a test message
			static __declspec(dllexport) void testMSG();
	};

	class checkOS {
		private:
			
		public:
			// Hora
			static __declspec(dllexport) void getTime();
			// Version del SO
			static __declspec(dllexport) void getOSVersion();
	};

	class checkCPU {
		private:
			static __declspec(dllexport) float CalculateCPULoad(unsigned long idleTicks, unsigned long totalTicks);
			static __declspec(dllexport) unsigned long FileTimeToInt64(const FILETIME& ft);
		public:
			static __declspec(dllexport) float GetCPULoad();
			//
			static __declspec(dllexport) void getCPU();
			// Cores
			static __declspec(dllexport) void getCPUCores();
			// Velocidad
			static __declspec(dllexport) void getCPUSpeed();
	};

	class checkMemory {
	private:
	public:
		static __declspec(dllexport) void getMemInfo();
	};

	class checkGPU {
	private:
	public:
	};

}