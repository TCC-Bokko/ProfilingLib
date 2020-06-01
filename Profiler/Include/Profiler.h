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
			static __declspec(dllexport) void getTime();
	};

	class checkCPU {
		private:
			static __declspec(dllexport) float CalculateCPULoad(unsigned long idleTicks, unsigned long totalTicks);
			static __declspec(dllexport) unsigned long FileTimeToInt64(const FILETIME& ft);
		public:
			static __declspec(dllexport) float GetCPULoad();
	};
}