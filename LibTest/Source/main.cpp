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

	// INITIALIZE WMI SERVICE
	WMIqueryServer WMI;
	WMI = Profiler::checkOS::initializeWMI();
	if (WMI.failStatus == 1) std::cout << "ERROR: WMI initialize FAILED\n";
	else if (WMI.failStatus == 0) std::cout << "WMI initialized with success.\n";

	// Game Info
	std::cout << "\n/// GET GAME INFO ///\n";
	Profiler::gameInfo::getGameInfo(WMI);

	system("PAUSE");


	// CLOSE WMI SERVICE
	Profiler::checkOS::closeWMI(WMI);
}