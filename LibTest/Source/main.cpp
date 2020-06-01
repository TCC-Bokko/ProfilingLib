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

	std::cout << "\n/// TEST MSG ///" << std::endl;
	Profiler::Testing::testMSG();

	// OS
	std::cout << "\n/// GET OS VERSION ///" << std::endl;
	Profiler::checkOS::getOSVersion();
	std::cout << "\n/// GET OS TIME ///" << std::endl;
	Profiler::checkOS::getTime();

	// CPU
	std::cout << "\n/// GET CPU CORES ///" << std::endl;
	Profiler::checkCPU::getCPUCores();

	std::cout << "\n/// GET CPU SPEED ///" << std::endl;
	Profiler::checkCPU::getCPUSpeed();

	// MEMORY
	std::cout << "\n/// GET Memory INFO ///" << std::endl;
	Profiler::checkMemory::getMemInfo();

	// GPU

	system("PAUSE");
}