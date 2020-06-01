#pragma once

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
			static __declspec(dllexport) void getCPU();
			static __declspec(dllexport) void getCPUCores();
			static __declspec(dllexport) void getCPUSpeed();
			static __declspec(dllexport) void getCPUInfo();
	};
}