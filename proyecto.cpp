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

bool fechaValida(const char* fecha);                 // YYYY-MM-DD valida
void fechaActual(char* out11);                       // escribe YYYY-MM-DD
bool cedulaValida(const char* cedula);               // digitos prefijo V-/E-/J- opcional
bool placaValida(const char* placa);                 // alfanumerica 6 o 7 caracteres
bool especialidadValida(const char* esp);
bool contieneIgnorandoCase(const char* texto const char* sub);
bool estaBajoStock(const Repuesto* r);               // stock <= stockMinimo

void inicializarSistema(SistemaTaller* s Taller taller);
void liberarSistema(SistemaTaller* s);

Vehiculo* agregarVehiculo(SistemaTaller* s Vehiculo datos);
Vehiculo* buscarVehiculoPorID(SistemaTaller* s int id);
Vehiculo* buscarVehiculoPorPlaca(SistemaTaller* s const char* placa);
Vehiculo** buscarVehiculosPorPropietario(SistemaTaller* s const char* subcadena int* cantidad);
Vehiculo** listarVehiculos(SistemaTaller* s int* cantidad);
bool actualizarVehiculo(SistemaTaller* s int id Vehiculo datos);
bool eliminarVehiculo(SistemaTaller* s int id);
int contarOrdenesVehiculo(SistemaTaller* s int idVehiculo);

Mecanico* agregarMecanico(SistemaTaller* s Mecanico datos Resultado* res);
Mecanico* buscarMecanicoPorID(SistemaTaller* s int id);
Mecanico** listarMecanicos(SistemaTaller* s int* cantidad);
int contarOrdenesAbiertasMecanico(SistemaTaller* s int idMecanico);

Repuesto* agregarRepuesto(SistemaTaller* s Repuesto datos Resultado* res);
Repuesto* buscarRepuestoPorID(SistemaTaller* s int id);
Repuesto* buscarRepuestoPorCodigo(SistemaTaller* s const char* codigo);
Repuesto** buscarRepuestosPorTexto(SistemaTaller* s const char* subcadena int* cantidad);
Repuesto** listarRepuestos(SistemaTaller* s int* cantidad);
Resultado registrarEntradaMercancia(SistemaTaller* s int idRepuesto int cantidad);
Resultado cambiarPrecioRepuesto(SistemaTaller* s int idRepuesto float nuevoPrecio);
bool repuestoEnUso(SistemaTaller* s int idRepuesto);
Resultado eliminarRepuesto(SistemaTaller* s int idRepuesto);

OrdenServicio* abrirOrden(SistemaTaller* s int idVehiculo int idMecanico
                          int kilometraje const char* descripcionFalla Resultado* res);
OrdenServicio* buscarOrdenPorID(SistemaTaller* s int id);
OrdenServicio* buscarOrdenAbiertaDeVehiculo(SistemaTaller* s int idVehiculo);
Resultado agregarRepuestoAOrden(SistemaTaller* s int idOrden int idRepuesto int cantidad);
Resultado quitarRepuestoDeOrden(SistemaTaller* s int idOrden int idRepuesto);
Resultado cerrarOrden(SistemaTaller* s int idOrden float manoObra);
Resultado anularOrden(SistemaTaller* s int idOrden);
float calcularSubtotalRepuestos(const OrdenServicio* o);
float calcularTotalOrden(const OrdenServicio* o);
OrdenServicio** listarOrdenes(SistemaTaller* s int* cantidad);
OrdenServicio** listarOrdenesPorEstado(SistemaTaller* s const char* estado int* cantidad);
OrdenServicio** listarOrdenesPorVehiculo(SistemaTaller* s int idVehiculo int* cantidad);
