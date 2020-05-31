#include <iostream>
#include <string>
#include "../Profiler/Include/Profiler.h"
#include <windows.h>

// Include del .lib en Preferencias Proyecto -> Vinculador -> Entrada
// El .lib debe estar en la carpeta Lib
// Enlazar el encabezado de la librería (Profiler.h) debe estar en el directorio Include
// El dll tiene que estar en la carpeta del ejecutable.

// OJO: Cada vez que se haga un cambio en la libreria Profiler y se compile
// hay que mover el Profiler.dll a la carpeta de ejecutables
// y el Profiler.lib a la carpeta Lib

void main() {

	std::cout << "Hello World.\n";

	Profiler::Testing::testMSG();

	system("PAUSE");
}