#include <cstring>
#include <cctype>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <iostream>
#include <string>
#ifdef _WIN32
#include <io.h>
#define ES_TTY() _isatty(_fileno(stdin))
#else
#include <unistd.h>
#define ES_TTY() isatty(fileno(stdin))
#endif


// estructuras
struct Taller {
    char nombre[100];
    char rif[20];
    char direccion[200];
    char telefono[20];
};

struct Vehiculo {
    int id;
    char placa[10];
    char marca[50];
    char modelo[50];
    int anio;
    int kilometraje;
    char nombrePropietario[100];
    char cedulaPropietario[20];
    char telefonoPropietario[20];
    char fechaRegistro[11];
};

struct Mecanico {
    int id;
    char nombre[100];
    char cedula[20];
    char especialidad[20];
    char fechaIngreso[11];
};

struct Repuesto {
    int id;
    char codigo[20];
    char nombre[100];
    char marca[50];
    float precio;
    int stock;
    int stockMinimo;
    char fechaRegistro[11];
};

struct DetalleOrden {
    int idRepuesto;
    int cantidad;
    float precioUnitario;   // precio al momento de agregarlo
};

struct OrdenServicio {
    int id;
    int idVehiculo;
    int idMecanico;
    char fechaApertura[11];
    char fechaCierre[11];
    char estado[12];        // "ABIERTA", "CERRADA", "ANULADA"
    char descripcionFalla[200];
    int kilometrajeIngreso;
    float manoObra;
    // Arreglo dinamico propio de la orden
    DetalleOrden* detalles;
    int numDetalles;
    int capacidadDetalles;
};

struct SistemaTaller {
    Taller taller;
    Vehiculo* vehiculos;      int numVehiculos;  int capacidadVehiculos;
    Mecanico* mecanicos;      int numMecanicos;  int capacidadMecanicos;
    Repuesto* repuestos;      int numRepuestos;  int capacidadRepuestos;
    OrdenServicio* ordenes;   int numOrdenes;    int capacidadOrdenes;
    int siguienteIdVehiculo;
    int siguienteIdMecanico;
    int siguienteIdRepuesto;
    int siguienteIdOrden;
};

// Codigo de resultado para las operaciones de mecanicos, repuestos y ordenes
enum Resultado {
    RES_OK = 0,
    RES_VEHICULO_NO_EXISTE,
    RES_MECANICO_NO_EXISTE,
    RES_REPUESTO_NO_EXISTE,
    RES_ORDEN_NO_EXISTE,
    RES_DUPLICADO,
    RES_DATO_INVALIDO,
    RES_VEHICULO_CON_ORDEN_ABIERTA,
    RES_MECANICO_SATURADO,
    RES_KM_MENOR,
    RES_STOCK_INSUFICIENTE,
    RES_ESTADO_INVALIDO,
    RES_REPUESTO_NO_EN_ORDEN,
    RES_ORDEN_VACIA,
    RES_REPUESTO_EN_USO
};