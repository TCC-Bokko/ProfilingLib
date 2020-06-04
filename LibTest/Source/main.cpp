#include <iostream>
#include <string>
#include "../Profiler/Include/Profiler.h"
#include <windows.h>

// PARA USAR EL DLL EN CUALQUIER SOFTWARE
// Include del .lib en Preferencias Proyecto -> Vinculador -> Entrada
// El .lib debe estar en la carpeta Lib
// Enlazar el encabezado de la librería (Profiler.h) debe estar en el directorio Include
// El dll tiene que estar en la carpeta del ejecutable.

// ESPECIFICO PARA EL TEST
// Si se hacen cambios en el Profiler, acordarse de compilarlo antes de ejecutar LibTest
// LibTest genera su ejecutable dentro del Build del Profiler para evitar mover archivos.
// LibTest enlaza el .lib desde la carpeta Build del Profiler


void main() {

	std::cout << "Grupo 7: Profiler.\n";

	std::cout << "\n/// TEST MSG ///\n";
	Profiler::Testing::testMSG();

	// OS
	std::cout << "\n/// GET OS VERSION ///\n";
	Profiler::checkOS::getOSVersion();
	std::cout << "\n/// GET OS TIME ///\n";
	Profiler::checkOS::getTime();

	// CPU
	std::cout << "\n/// GET CPU INFO ///\n";
	Profiler::checkCPU::getCPU();
	std::cout << "\n";
	Profiler::checkCPU::getCPUCores();

	std::cout << "\n/// GET CPU SPEED ///\n";
	Profiler::checkCPU::getCPUSpeed();

	std::cout << "\n/// GET CPU PINNING ///\n";
	int counter = 0;
	int laps = 20;
	int wait = 100;
	float media = 0;
	std::cout << "CPU pinning in " << laps << " laps with " << wait << "ms in between\n";
	while (counter < laps) {
		float CPULoad = Profiler::checkCPU::GetCPULoad();
		media += CPULoad;

		// Sleep para dejar la CPU idle un tiempo y que se note el cambio
		// entre una llamada y otra, no sera necesario en el juego
		// ya que habra mas cosas entre una llamada y otra de GetCPULoad().
		Sleep(wait);

		counter++;
	}

	media = media / laps;
	std::cout << "Media CPU pinning: " << media << "\n";


	// MEMORY
	std::cout << "\n/// GET Memory INFO ///\n";
	Profiler::checkMemory::getMemInfo();

	// GPU
	//Profiler::checkGPU::GetGPUInfo();

	system("PAUSE");
}