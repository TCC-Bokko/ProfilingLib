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

	std::cout << "Hello World.\n";

	int counter = 0;
	int laps = 20;
	float media = 0;
	while (counter < laps) {

		//Profiler::Testing::testMSG();
		
		//Profiler::checkOS::getTime();

		media += Profiler::checkCPU::GetCPULoad();
		

		//Sleep(20);
	
		counter++;
	}

	media = media / laps;
	std::cout << "Media: " << media << "\n";

	system("PAUSE");
}