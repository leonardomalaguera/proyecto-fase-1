// Experimento de la pregunta 1 del README_V2: escribir con write() una OrdenServicio del
// Proyecto 1 TAL CUAL (con su DetalleOrden* detalles) y leerla en OTRA ejecucion.
// Compilar: g++ experimento.cpp -o experimento
// Ejecutar:  ./experimento escribir     (y luego, en una ejecucion nueva)   ./experimento leer
#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

struct DetalleOrden { int idRepuesto; int cantidad; float precioUnitario; };
struct OrdenServicioP1 {                 // estructura del Proyecto 1
    int id; int idVehiculo; int idMecanico;
    char fechaApertura[11]; char fechaCierre[11]; char estado[12]; char descripcionFalla[200];
    int kilometrajeIngreso; float manoObra;
    DetalleOrden* detalles; int numDetalles; int capacidadDetalles;   // <- puntero a memoria dinamica
};

int main(int argc, char** argv) {
    if (argc < 2) { cout << "uso: experimento escribir|leer\n"; return 1; }
    if (strcmp(argv[1], "escribir") == 0) {
        OrdenServicioP1 o; memset(&o, 0, sizeof o);
        o.id = 1; strcpy(o.estado, "ABIERTA");
        o.capacidadDetalles = 4; o.numDetalles = 3;
        o.detalles = new DetalleOrden[4];
        for (int i = 0; i < 3; i++) { o.detalles[i].idRepuesto = i + 1; o.detalles[i].cantidad = 2; o.detalles[i].precioUnitario = 10.5f * (i + 1); }
        cout << "[escribir] sizeof(OrdenServicioP1) = " << sizeof o << " bytes\n";
        cout << "[escribir] direccion guardada en 'detalles' = " << (void*)o.detalles << "\n";
        cout << "[escribir] detalles[0].idRepuesto = " << o.detalles[0].idRepuesto << " (aqui si funciona)\n";
        ofstream f("orden_p1.bin", ios::binary);
        f.write(reinterpret_cast<const char*>(&o), sizeof o);
        return 0;                          // el programa termina: esa memoria dinamica desaparece
    }
    OrdenServicioP1 o;
    ifstream f("orden_p1.bin", ios::binary);
    f.read(reinterpret_cast<char*>(&o), sizeof o);
    cout << "[leer] id = " << o.id << ", estado = " << o.estado << ", numDetalles = " << o.numDetalles << " (los datos simples si volvieron)\n";
    cout << "[leer] direccion leida en 'detalles' = " << (void*)o.detalles << "\n";
    cout << "[leer] intentando leer detalles[0].idRepuesto ...\n" << flush;
    cout << "[leer] detalles[0].idRepuesto = " << o.detalles[0].idRepuesto << "\n";
    return 0;
}
