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

bool entradaTerminada();

// Menus
void menuPrincipal(SistemaTaller* s);
void menuVehiculos(SistemaTaller* s);
void menuMecanicos(SistemaTaller* s);
void menuInventario(SistemaTaller* s);
void menuOrdenes(SistemaTaller* s);

// Vehiculos
void menuRegistrarVehiculo(SistemaTaller* s);
void menuBuscarVehiculo(SistemaTaller* s);
void menuActualizarVehiculo(SistemaTaller* s);
void menuEliminarVehiculo(SistemaTaller* s);
void menuListarVehiculos(SistemaTaller* s);
void mostrarVehiculo(Vehiculo* vehiculo);
void mostrarListaVehiculos(Vehiculo** vehiculos, int cantidad);

// Mecanicos
void menuRegistrarMecanico(SistemaTaller* s);
void menuBuscarMecanico(SistemaTaller* s);
void menuListarMecanicos(SistemaTaller* s);
void mostrarMecanico(Mecanico* mecanico, SistemaTaller* s);
void mostrarListaMecanicos(Mecanico** mecanicos, int cantidad, SistemaTaller* s);

// Repuestos
void menuRegistrarRepuesto(SistemaTaller* s);
void menuBuscarRepuesto(SistemaTaller* s);
void menuEntradaMercancia(SistemaTaller* s);
void menuCambiarPrecio(SistemaTaller* s);
void menuEliminarRepuesto(SistemaTaller* s);
void menuListarRepuestos(SistemaTaller* s);
void mostrarRepuesto(Repuesto* repuesto);
void mostrarListaRepuestos(Repuesto** repuestos, int cantidad);

// ordenes
void menuAbrirOrden(SistemaTaller* s);
void menuAgregarRepuestoOrden(SistemaTaller* s);
void menuQuitarRepuestoOrden(SistemaTaller* s);
void menuCerrarOrden(SistemaTaller* s);
void menuAnularOrden(SistemaTaller* s);
void menuVerDetalleOrden(SistemaTaller* s);
void menuListarOrdenes(SistemaTaller* s);
void mostrarOrden(OrdenServicio* orden, SistemaTaller* s);
void mostrarListaOrdenes(OrdenServicio** ordenes, int cantidad, SistemaTaller* s);

static void setRes(Resultado* r Resultado v) { if (r) *r = v; }

static void copiar(char* dest, const char* src int n) {
    strncpy(dest src n - 1);
    dest[n - 1] = '\0';
}

static bool vacio(const char* s) {
    for (; *s; ++s) if (!isspace((unsigned char)*s)) return false;
    return true;
}

static void aMayusculas(char* s) {
    for (; *s; ++s) *s = (char)toupper((unsigned char)*s);
}

static bool esEstado(const OrdenServicio* o const char* estado) {
    return strcmp(o->estado estado) == 0;
}

void fechaActual(char* out) {
    time_t t = time(nullptr);
    struct tm* l = localtime(&t);
    strftime(out 11 "%Y-%m-%d" l);
}

bool fechaValida(const char* f) {
    if (strlen(f) != 10) return false;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) { if (f[i] != '-') return false; }
        else if (!isdigit((unsigned char)f[i])) return false;
    }
    int a = (f[0]-'0')*1000 + (f[1]-'0')*100 + (f[2]-'0')*10 + (f[3]-'0');
    int m = (f[5]-'0')*10 + (f[6]-'0');
    int d = (f[8]-'0')*10 + (f[9]-'0');
    if (a < 1900 || m < 1 || m > 12 || d < 1) return false;
    int dias[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    bool bisiesto = (a % 4 == 0 && a % 100 != 0) || a % 400 == 0;
    int max = dias[m - 1];
    if (m == 2 && bisiesto) max = 29;
    return d <= max;
}

bool cedulaValida(const char* c) {
    size_t n = strlen(c), i = 0;
    if (n >= 2 && c[1] == '-') {
        char p = (char)toupper((unsigned char)c[0]);
        if (p == 'V' || p == 'E' || p == 'J') i = 2; else return false;
    }
    if (i >= n) return false;
    for (; i < n; i++) if (!isdigit((unsigned char)c[i])) return false;
    return true;
}

bool placaValida(const char* p) {
    size_t n = strlen(p);
    if (n < 6 || n > 7) return false;
    for (size_t i = 0; i < n; i++) if (!isalnum((unsigned char)p[i])) return false;
    return true;
}

bool especialidadValida(const char* e) {
    return strcmp(e, "MOTOR") == 0 || strcmp(e, "FRENOS") == 0 ||
           strcmp(e, "ELECTRICIDAD") == 0 || strcmp(e, "SUSPENSION") == 0 ||
           strcmp(e, "GENERAL") == 0;
}

bool contieneIgnorandoCase(const char* t, const char* sub) {
    size_t n = strlen(t), m = strlen(sub);
    if (m == 0) return true;
    if (m > n) return false;
    for (size_t i = 0; i + m <= n; i++) {
        size_t j = 0;
        while (j < m && tolower((unsigned char)t[i + j]) == tolower((unsigned char)sub[j])) j++;
        if (j == m) return true;
    }
    return false;
}

bool estaBajoStock(const Repuesto* r) { return r->stock <= r->stockMinimo; }

// redimensionamiento 
// cuando num == capacidad se reserva un arreglo del doble se copia y se libera el viejo.
static void crecerVehiculos(SistemaTaller* s) {
    if (s->numVehiculos < s->capacidadVehiculos) return;
    int nueva = s->capacidadVehiculos * 2;
    Vehiculo* nuevo = new Vehiculo[nueva]();
    for (int i = 0; i < s->numVehiculos; i++) nuevo[i] = s->vehiculos[i];
    delete[] s->vehiculos;
    s->vehiculos = nuevo;
    s->capacidadVehiculos = nueva;
}

static void crecerMecanicos(SistemaTaller* s) {
    if (s->numMecanicos < s->capacidadMecanicos) return;
    int nueva = s->capacidadMecanicos * 2;
    Mecanico* nuevo = new Mecanico[nueva]();
    for (int i = 0; i < s->numMecanicos; i++) nuevo[i] = s->mecanicos[i];
    delete[] s->mecanicos;
    s->mecanicos = nuevo;
    s->capacidadMecanicos = nueva;
}

static void crecerRepuestos(SistemaTaller* s) {
    if (s->numRepuestos < s->capacidadRepuestos) return;
    int nueva = s->capacidadRepuestos * 2;
    Repuesto* nuevo = new Repuesto[nueva]();
    for (int i = 0; i < s->numRepuestos; i++) nuevo[i] = s->repuestos[i];
    delete[] s->repuestos;
    s->repuestos = nuevo;
    s->capacidadRepuestos = nueva;
}

static void crecerOrdenes(SistemaTaller* s) {
    if (s->numOrdenes < s->capacidadOrdenes) return;
    int nueva = s->capacidadOrdenes * 2;
    OrdenServicio* nuevo = new OrdenServicio[nueva]();
    // Copia superficial cada orden traslada su puntero detalles al nuevo arreglo.
    // El arreglo de detalles no se copia ni se pierde solo cambia de dueño.
    // delete[] del arreglo viejo no libera los detalles porque OrdenServicio no tiene destructor.
    for (int i = 0; i < s->numOrdenes; i++) nuevo[i] = s->ordenes[i];
    delete[] s->ordenes;
    s->ordenes = nuevo;
    s->capacidadOrdenes = nueva;
}

static void crecerDetalles(OrdenServicio* o) {
    if (o->numDetalles < o->capacidadDetalles) return;
    int nueva = o->capacidadDetalles * 2;
    DetalleOrden* nuevo = new DetalleOrden[nueva]();
    for (int i = 0; i < o->numDetalles; i++) nuevo[i] = o->detalles[i];
    delete[] o->detalles;
    o->detalles = nuevo;
    o->capacidadDetalles = nueva;
}

//sistema 
void inicializarSistema(SistemaTaller* s, Taller taller) {
    s->taller = taller;
    s->capacidadVehiculos = 4; s->numVehiculos = 0; s->vehiculos = new Vehiculo[4]();
    s->capacidadMecanicos = 4; s->numMecanicos = 0; s->mecanicos = new Mecanico[4]();
    s->capacidadRepuestos = 4; s->numRepuestos = 0; s->repuestos = new Repuesto[4]();
    s->capacidadOrdenes   = 4; s->numOrdenes   = 0; s->ordenes   = new OrdenServicio[4]();
    s->siguienteIdVehiculo = 1;
    s->siguienteIdMecanico = 1;
    s->siguienteIdRepuesto = 1;
    s->siguienteIdOrden = 1;
}

void liberarSistema(SistemaTaller* s) {
    // Primero la memoria anidada el arreglo de detalles de cada orden
    for (int i = 0; i < s->numOrdenes; i++) {
        delete[] s->ordenes[i].detalles;
        s->ordenes[i].detalles = nullptr;
    }
    delete[] s->ordenes;   s->ordenes = nullptr;
    delete[] s->repuestos; s->repuestos = nullptr;
    delete[] s->mecanicos; s->mecanicos = nullptr;
    delete[] s->vehiculos; s->vehiculos = nullptr;
    s->numVehiculos = s->numMecanicos = s->numRepuestos = s->numOrdenes = 0;
    s->capacidadVehiculos = s->capacidadMecanicos = s->capacidadRepuestos = s->capacidadOrdenes = 0;
}

//vehiculos 
static bool propietarioYModeloValidos(const Vehiculo& v) {
    if (vacio(v.marca) || vacio(v.modelo) || vacio(v.nombrePropietario) || vacio(v.telefonoPropietario)) return false;
    if (v.anio < 1950 || v.anio > 2027) return false;
    if (!cedulaValida(v.cedulaPropietario)) return false;
    return true;
}

Vehiculo* agregarVehiculo(SistemaTaller* s, Vehiculo datos) {
    aMayusculas(datos.placa);
    if (!placaValida(datos.placa)) return nullptr;
    if (buscarVehiculoPorPlaca(s, datos.placa) != nullptr) return nullptr;
    if (!propietarioYModeloValidos(datos)) return nullptr;
    if (datos.kilometraje < 0) return nullptr;

    crecerVehiculos(s);
    datos.id = s->siguienteIdVehiculo++;
    fechaActual(datos.fechaRegistro);
    s->vehiculos[s->numVehiculos] = datos;
    s->numVehiculos++;
    return &s->vehiculos[s->numVehiculos - 1];
}

Vehiculo* buscarVehiculoPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numVehiculos; i++)
        if (s->vehiculos[i].id == id) return &s->vehiculos[i];
    return nullptr;
}

Vehiculo* buscarVehiculoPorPlaca(SistemaTaller* s, const char* placa) {
    for (int i = 0; i < s->numVehiculos; i++)
        if (strcmp(s->vehiculos[i].placa, placa) == 0) return &s->vehiculos[i];
    return nullptr;
}

Vehiculo** buscarVehiculosPorPropietario(SistemaTaller* s, const char* sub, int* cantidad) {
    int n = 0;
    for (int i = 0; i < s->numVehiculos; i++)
        if (contieneIgnorandoCase(s->vehiculos[i].nombrePropietario, sub)) n++;
    *cantidad = n;
    if (n == 0) return nullptr;
    Vehiculo** r = new Vehiculo*[n];
    int k = 0;
    for (int i = 0; i < s->numVehiculos; i++)
        if (contieneIgnorandoCase(s->vehiculos[i].nombrePropietario, sub)) r[k++] = &s->vehiculos[i];
    return r;
}

Vehiculo** listarVehiculos(SistemaTaller* s, int* cantidad) {
    *cantidad = s->numVehiculos;
    if (s->numVehiculos == 0) return nullptr;
    Vehiculo** r = new Vehiculo*[s->numVehiculos];
    for (int i = 0; i < s->numVehiculos; i++) r[i] = &s->vehiculos[i];
    return r;
}

bool actualizarVehiculo(SistemaTaller* s, int id, Vehiculo datos) {
    Vehiculo* v = buscarVehiculoPorID(s, id);
    if (!v) return false;
    if (!propietarioYModeloValidos(datos)) return false;
    copiar(v->marca, datos.marca, sizeof v->marca);
    copiar(v->modelo, datos.modelo, sizeof v->modelo);
    v->anio = datos.anio;
    copiar(v->nombrePropietario, datos.nombrePropietario, sizeof v->nombrePropietario);
    copiar(v->cedulaPropietario, datos.cedulaPropietario, sizeof v->cedulaPropietario);
    copiar(v->telefonoPropietario, datos.telefonoPropietario, sizeof v->telefonoPropietario);
    return true;
}

int contarOrdenesVehiculo(SistemaTaller* s, int idVehiculo) {
    int n = 0;
    for (int i = 0; i < s->numOrdenes; i++)
        if (s->ordenes[i].idVehiculo == idVehiculo) n++;
    return n;
}

bool eliminarVehiculo(SistemaTaller* s, int id) {
    int idx = -1;
    for (int i = 0; i < s->numVehiculos; i++)
        if (s->vehiculos[i].id == id) { idx = i; break; }
    if (idx < 0) return false;
    if (contarOrdenesVehiculo(s, id) > 0) return false;
    for (int i = idx; i < s->numVehiculos - 1; i++) s->vehiculos[i] = s->vehiculos[i + 1];
    s->numVehiculos--;
    return true;
}