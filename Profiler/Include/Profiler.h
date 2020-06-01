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
			// Hora
			static __declspec(dllexport) void getTime();
			// Version del SO
			static __declspec(dllexport) void getOSVersion();
	};

	class checkCPU {
	private:
	public:
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