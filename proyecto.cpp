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

Mecanico* agregarMecanico(SistemaTaller* s, Mecanico datos, Resultado* res) {
    aMayusculas(datos.cedula);
    aMayusculas(datos.especialidad);
    if (vacio(datos.nombre) || !cedulaValida(datos.cedula) ||
        !especialidadValida(datos.especialidad) || !fechaValida(datos.fechaIngreso)) {
        setRes(res, RES_DATO_INVALIDO);
        return nullptr;
    }
    for (int i = 0; i < s->numMecanicos; i++)
        if (strcmp(s->mecanicos[i].cedula, datos.cedula) == 0) {
            setRes(res, RES_DUPLICADO);
            return nullptr;
        }
    crecerMecanicos(s);
    datos.id = s->siguienteIdMecanico++;
    s->mecanicos[s->numMecanicos] = datos;
    s->numMecanicos++;
    setRes(res, RES_OK);
    return &s->mecanicos[s->numMecanicos - 1];
}

Mecanico* buscarMecanicoPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numMecanicos; i++)
        if (s->mecanicos[i].id == id) return &s->mecanicos[i];
    return nullptr;
}

Mecanico** listarMecanicos(SistemaTaller* s, int* cantidad) {
    *cantidad = s->numMecanicos;
    if (s->numMecanicos == 0) return nullptr;
    Mecanico** r = new Mecanico*[s->numMecanicos];
    for (int i = 0; i < s->numMecanicos; i++) r[i] = &s->mecanicos[i];
    return r;
}

int contarOrdenesAbiertasMecanico(SistemaTaller* s, int idMecanico) {
    int n = 0;
    for (int i = 0; i < s->numOrdenes; i++)
        if (s->ordenes[i].idMecanico == idMecanico && esEstado(&s->ordenes[i], "ABIERTA")) n++;
    return n;
}

Repuesto* agregarRepuesto(SistemaTaller* s, Repuesto datos, Resultado* res) {
    aMayusculas(datos.codigo);
    if (vacio(datos.codigo) || vacio(datos.nombre) || vacio(datos.marca) ||
        datos.precio <= 0 || datos.stock < 0 || datos.stockMinimo < 0) {
        setRes(res, RES_DATO_INVALIDO);
        return nullptr;
    }
    if (buscarRepuestoPorCodigo(s, datos.codigo) != nullptr) {
        setRes(res, RES_DUPLICADO);
        return nullptr;
    }
    crecerRepuestos(s);
    datos.id = s->siguienteIdRepuesto++;
    fechaActual(datos.fechaRegistro);
    s->repuestos[s->numRepuestos] = datos;
    s->numRepuestos++;
    setRes(res, RES_OK);
    return &s->repuestos[s->numRepuestos - 1];
}

Repuesto* buscarRepuestoPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numRepuestos; i++)
        if (s->repuestos[i].id == id) return &s->repuestos[i];
    return nullptr;
}

Repuesto* buscarRepuestoPorCodigo(SistemaTaller* s, const char* codigo) {
    for (int i = 0; i < s->numRepuestos; i++)
        if (strcmp(s->repuestos[i].codigo, codigo) == 0) return &s->repuestos[i];
    return nullptr;
}

Repuesto** buscarRepuestosPorTexto(SistemaTaller* s, const char* sub, int* cantidad) {
    int n = 0;
    for (int i = 0; i < s->numRepuestos; i++)
        if (contieneIgnorandoCase(s->repuestos[i].codigo, sub) ||
            contieneIgnorandoCase(s->repuestos[i].nombre, sub)) n++;
    *cantidad = n;
    if (n == 0) return nullptr;
    Repuesto** r = new Repuesto*[n];
    int k = 0;
    for (int i = 0; i < s->numRepuestos; i++)
        if (contieneIgnorandoCase(s->repuestos[i].codigo, sub) ||
            contieneIgnorandoCase(s->repuestos[i].nombre, sub)) r[k++] = &s->repuestos[i];
    return r;
}

Repuesto** listarRepuestos(SistemaTaller* s, int* cantidad) {
    *cantidad = s->numRepuestos;
    if (s->numRepuestos == 0) return nullptr;
    Repuesto** r = new Repuesto*[s->numRepuestos];
    for (int i = 0; i < s->numRepuestos; i++) r[i] = &s->repuestos[i];
    return r;
}

Resultado registrarEntradaMercancia(SistemaTaller* s, int idRepuesto, int cantidad) {
    Repuesto* r = buscarRepuestoPorID(s, idRepuesto);
    if (!r) return RES_REPUESTO_NO_EXISTE;
    if (cantidad <= 0) return RES_DATO_INVALIDO;
    r->stock += cantidad;
    return RES_OK;
}

Resultado cambiarPrecioRepuesto(SistemaTaller* s, int idRepuesto, float nuevoPrecio) {
    Repuesto* r = buscarRepuestoPorID(s, idRepuesto);
    if (!r) return RES_REPUESTO_NO_EXISTE;
    if (nuevoPrecio <= 0) return RES_DATO_INVALIDO;
    r->precio = nuevoPrecio;   // las órdenes conservan su propio precioUnitario
    return RES_OK;
}

bool repuestoEnUso(SistemaTaller* s, int idRepuesto) {
    for (int i = 0; i < s->numOrdenes; i++)
        for (int j = 0; j < s->ordenes[i].numDetalles; j++)
            if (s->ordenes[i].detalles[j].idRepuesto == idRepuesto) return true;
    return false;
}

Resultado eliminarRepuesto(SistemaTaller* s, int idRepuesto) {
    int idx = -1;
    for (int i = 0; i < s->numRepuestos; i++)
        if (s->repuestos[i].id == idRepuesto) { idx = i; break; }
    if (idx < 0) return RES_REPUESTO_NO_EXISTE;
    if (repuestoEnUso(s, idRepuesto)) return RES_REPUESTO_EN_USO;
    for (int i = idx; i < s->numRepuestos - 1; i++) s->repuestos[i] = s->repuestos[i + 1];
    s->numRepuestos--;
    return RES_OK;
}

OrdenServicio* abrirOrden(SistemaTaller* s, int idVehiculo, int idMecanico,
                          int kilometraje, const char* descripcionFalla, Resultado* res) {
    Vehiculo* v = buscarVehiculoPorID(s, idVehiculo);
    if (!v) { setRes(res, RES_VEHICULO_NO_EXISTE); return nullptr; }
    Mecanico* m = buscarMecanicoPorID(s, idMecanico);
    if (!m) { setRes(res, RES_MECANICO_NO_EXISTE); return nullptr; }
    if (kilometraje < 0 || vacio(descripcionFalla) || strlen(descripcionFalla) >= 200) {
        setRes(res, RES_DATO_INVALIDO); return nullptr;
    }
    if (buscarOrdenAbiertaDeVehiculo(s, idVehiculo) != nullptr) {
        setRes(res, RES_VEHICULO_CON_ORDEN_ABIERTA); return nullptr;
    }
    if (contarOrdenesAbiertasMecanico(s, idMecanico) >= 3) {
        setRes(res, RES_MECANICO_SATURADO); return nullptr;
    }
    if (kilometraje < v->kilometraje) { setRes(res, RES_KM_MENOR); return nullptr; }

    crecerOrdenes(s);   // puede invalidar punteros a ordenes; 'v' es de otro arreglo y sigue valido
    OrdenServicio nueva = OrdenServicio();
    nueva.id = s->siguienteIdOrden++;
    nueva.idVehiculo = idVehiculo;
    nueva.idMecanico = idMecanico;
    fechaActual(nueva.fechaApertura);
    nueva.fechaCierre[0] = '\0';
    copiar(nueva.estado, "ABIERTA", sizeof nueva.estado);
    copiar(nueva.descripcionFalla, descripcionFalla, sizeof nueva.descripcionFalla);
    nueva.kilometrajeIngreso = kilometraje;
    nueva.manoObra = 0;
    nueva.capacidadDetalles = 2;
    nueva.numDetalles = 0;
    nueva.detalles = new DetalleOrden[2]();

    s->ordenes[s->numOrdenes] = nueva;
    s->numOrdenes++;
    v->kilometraje = kilometraje;
    setRes(res, RES_OK);
    return &s->ordenes[s->numOrdenes - 1];
}

OrdenServicio* buscarOrdenPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numOrdenes; i++)
        if (s->ordenes[i].id == id) return &s->ordenes[i];
    return nullptr;
}

OrdenServicio* buscarOrdenAbiertaDeVehiculo(SistemaTaller* s, int idVehiculo) {
    for (int i = 0; i < s->numOrdenes; i++)
        if (s->ordenes[i].idVehiculo == idVehiculo && esEstado(&s->ordenes[i], "ABIERTA"))
            return &s->ordenes[i];
    return nullptr;
}

static int indiceDetalle(const OrdenServicio* o, int idRepuesto) {
    for (int i = 0; i < o->numDetalles; i++)
        if (o->detalles[i].idRepuesto == idRepuesto) return i;
    return -1;
}

Resultado agregarRepuestoAOrden(SistemaTaller* s, int idOrden, int idRepuesto, int cantidad) {
    OrdenServicio* o = buscarOrdenPorID(s, idOrden);
    if (!o) return RES_ORDEN_NO_EXISTE;
    if (!esEstado(o, "ABIERTA")) return RES_ESTADO_INVALIDO;
    Repuesto* r = buscarRepuestoPorID(s, idRepuesto);
    if (!r) return RES_REPUESTO_NO_EXISTE;
    if (cantidad <= 0) return RES_DATO_INVALIDO;
    if (r->stock < cantidad) return RES_STOCK_INSUFICIENTE;

    int idx = indiceDetalle(o, idRepuesto);
    if (idx >= 0) {
        o->detalles[idx].cantidad += cantidad;   // acumula en la linea existente
    } else {
        crecerDetalles(o);
        DetalleOrden d;
        d.idRepuesto = idRepuesto;
        d.cantidad = cantidad;
        d.precioUnitario = r->precio;            // precio histórico congelado
        o->detalles[o->numDetalles++] = d;
    }
    r->stock -= cantidad;
    return RES_OK;
}

Resultado quitarRepuestoDeOrden(SistemaTaller* s, int idOrden, int idRepuesto) {
    OrdenServicio* o = buscarOrdenPorID(s, idOrden);
    if (!o) return RES_ORDEN_NO_EXISTE;
    if (!esEstado(o, "ABIERTA")) return RES_ESTADO_INVALIDO;
    int idx = indiceDetalle(o, idRepuesto);
    if (idx < 0) return RES_REPUESTO_NO_EN_ORDEN;
    Repuesto* r = buscarRepuestoPorID(s, idRepuesto);
    if (r) r->stock += o->detalles[idx].cantidad;   // devuelve al inventario
    for (int i = idx; i < o->numDetalles - 1; i++) o->detalles[i] = o->detalles[i + 1];
    o->numDetalles--;
    return RES_OK;
}

Resultado cerrarOrden(SistemaTaller* s, int idOrden, float manoObra) {
    OrdenServicio* o = buscarOrdenPorID(s, idOrden);
    if (!o) return RES_ORDEN_NO_EXISTE;
    if (!esEstado(o, "ABIERTA")) return RES_ESTADO_INVALIDO;
    if (manoObra < 0) return RES_DATO_INVALIDO;
    if (o->numDetalles == 0 && manoObra <= 0) return RES_ORDEN_VACIA;
    o->manoObra = manoObra;
    copiar(o->estado, "CERRADA", sizeof o->estado);
    fechaActual(o->fechaCierre);
    return RES_OK;
}

Resultado anularOrden(SistemaTaller* s, int idOrden) {
    OrdenServicio* o = buscarOrdenPorID(s, idOrden);
    if (!o) return RES_ORDEN_NO_EXISTE;
    if (!esEstado(o, "ABIERTA") && !esEstado(o, "CERRADA")) return RES_ESTADO_INVALIDO;
    for (int i = 0; i < o->numDetalles; i++) {
        Repuesto* r = buscarRepuestoPorID(s, o->detalles[i].idRepuesto);
        if (r) r->stock += o->detalles[i].cantidad;
    }
    copiar(o->estado, "ANULADA", sizeof o->estado);
    return RES_OK;
}

float calcularSubtotalRepuestos(const OrdenServicio* o) {
    float t = 0;
    for (int i = 0; i < o->numDetalles; i++)
        t += o->detalles[i].cantidad * o->detalles[i].precioUnitario;
    return t;
}

float calcularTotalOrden(const OrdenServicio* o) {
    return o->manoObra + calcularSubtotalRepuestos(o);
}

OrdenServicio** listarOrdenes(SistemaTaller* s, int* cantidad) {
    *cantidad = s->numOrdenes;
    if (s->numOrdenes == 0) return nullptr;
    OrdenServicio** r = new OrdenServicio*[s->numOrdenes];
    for (int i = 0; i < s->numOrdenes; i++) r[i] = &s->ordenes[i];
    return r;
}

OrdenServicio** listarOrdenesPorEstado(SistemaTaller* s, const char* estado, int* cantidad) {
    int n = 0;
    for (int i = 0; i < s->numOrdenes; i++) if (esEstado(&s->ordenes[i], estado)) n++;
    *cantidad = n;
    if (n == 0) return nullptr;
    OrdenServicio** r = new OrdenServicio*[n];
    int k = 0;
    for (int i = 0; i < s->numOrdenes; i++) if (esEstado(&s->ordenes[i], estado)) r[k++] = &s->ordenes[i];
    return r;
}

OrdenServicio** listarOrdenesPorVehiculo(SistemaTaller* s, int idVehiculo, int* cantidad) {
    int n = contarOrdenesVehiculo(s, idVehiculo);
    *cantidad = n;
    if (n == 0) return nullptr;
    OrdenServicio** r = new OrdenServicio*[n];
    int k = 0;
    for (int i = 0; i < s->numOrdenes; i++) if (s->ordenes[i].idVehiculo == idVehiculo) r[k++] = &s->ordenes[i];
    return r;
}

//capa de presentacion
using namespace std;

static bool gEOF = false;
static bool gEco = !ES_TTY();   

bool entradaTerminada() { return gEOF; }

//lectura de entrada
static void mayusculas(string& s) {
    for (size_t i = 0; i < s.size(); i++) s[i] = (char)toupper((unsigned char)s[i]);
}

static void recortar(string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    s = s.substr(a, b - a);
}

static bool igualCI(const string& a, const char* b) {
    if (a.size() != strlen(b)) return false;
    for (size_t i = 0; i < a.size(); i++)
        if (toupper((unsigned char)a[i]) != toupper((unsigned char)b[i])) return false;
    return true;
}

//devuelve false si el usuario escribe cancelar o si se acabo la entrada.
static bool leerLinea(const char* prompt, string& out) {
    cout << prompt << flush;
    if (!getline(cin, out)) {
        gEOF = true;
        cout << "\n";
        return false;
    }
    recortar(out);
    if (gEco) cout << out << "\n";
    if (igualCI(out, "CANCELAR")) {
        cout << "Operación cancelada. No se realizó ningún cambio.\n";
        return false;
    }
    return true;
}

static bool leerTexto(const char* prompt, char* dest, int max) {
    string t;
    while (true) {
        if (!leerLinea(prompt, t)) return false;
        if (t.empty()) { cout << "ERROR: Este campo no puede estar vacío.\n"; continue; }
        if ((int)t.size() >= max) {
            cout << "ERROR: Texto demasiado largo (máximo " << max - 1 << " caracteres).\n";
            continue;
        }
        strcpy(dest, t.c_str());
        return true;
    }
}

static bool leerEntero(const char* prompt, int* out, long minV, long maxV) {
    string t;
    while (true) {
        if (!leerLinea(prompt, t)) return false;
        char* fin = nullptr;
        long v = strtol(t.c_str(), &fin, 10);
        if (t.empty() || *fin != '\0') {
            cout << "ERROR: '" << t << "' no es un número entero válido.\n";
            continue;
        }
        if (v < minV || v > maxV) {
            if (maxV >= INT_MAX) cout << "ERROR: El valor debe ser mayor o igual a " << minV << ".\n";
            else cout << "ERROR: El valor debe estar entre " << minV << " y " << maxV << ".\n";
            continue;
        }
        *out = (int)v;
        return true;
    }
}

static bool leerFloat(const char* prompt, float* out, double minV, bool estricto) {
    string t;
    while (true) {
        if (!leerLinea(prompt, t)) return false;
        for (size_t i = 0; i < t.size(); i++) if (t[i] == ',') t[i] = '.';
        char* fin = nullptr;
        double v = strtod(t.c_str(), &fin);
        if (t.empty() || *fin != '\0') {
            cout << "ERROR: '" << t << "' no es un número válido.\n";
            continue;
        }
        if (estricto ? !(v > minV) : !(v >= minV)) {
            cout << "ERROR: El valor debe ser " << (estricto ? "mayor que " : "mayor o igual a ") << minV << ".\n";
            continue;
        }
        *out = (float)v;
        return true;
    }
}

static bool leerSN(const char* prompt, bool* resp) {
    string t;
    while (true) {
        if (!leerLinea(prompt, t)) return false;
        mayusculas(t);
        if (t == "S") { *resp = true; return true; }
        if (t == "N") { *resp = false; return true; }
        cout << "ERROR: Responda S o N.\n";
    }
}

static bool leerCedula(const char* prompt, char* dest, int max) {
    string t;
    while (true) {
        if (!leerLinea(prompt, t)) return false;
        mayusculas(t);
        if (!cedulaValida(t.c_str()) || (int)t.size() >= max) {
            cout << "ERROR: Cédula inválida. Use solo dígitos (se admite prefijo V-, E- o J-).\n";
            continue;
        }
        strcpy(dest, t.c_str());
        return true;
    }
}

static bool leerPlaca(const char* prompt, char* dest, int max) {
    string t;
    while (true) {
        if (!leerLinea(prompt, t)) return false;
        mayusculas(t);
        if (!placaValida(t.c_str()) || (int)t.size() >= max) {
            cout << "ERROR: Placa inválida. Debe ser alfanumérica de 6 o 7 caracteres.\n";
            continue;
        }
        strcpy(dest, t.c_str());
        return true;
    }
}

static bool leerFecha(const char* prompt, char* dest) {
    string t;
    while (true) {
        if (!leerLinea(prompt, t)) return false;
        if (!fechaValida(t.c_str())) {
            cout << "ERROR: Fecha inválida. Use el formato YYYY-MM-DD con valores reales.\n";
            continue;
        }
        strcpy(dest, t.c_str());
        return true;
    }
}

static bool leerEspecialidad(const char* prompt, char* dest, int max) {
    string t;
    while (true) {
        if (!leerLinea(prompt, t)) return false;
        mayusculas(t);
        if (!especialidadValida(t.c_str()) || (int)t.size() >= max) {
            cout << "ERROR: Especialidad inválida. Valores permitidos: MOTOR, FRENOS, ELECTRICIDAD, SUSPENSION, GENERAL.\n";
            continue;
        }
        strcpy(dest, t.c_str());
        return true;
    }
}

static bool leerCodigo(const char* prompt, char* dest, int max) {
    string t;
    while (true) {
        if (!leerLinea(prompt, t)) return false;
        mayusculas(t);
        if (t.empty()) { cout << "ERROR: Este campo no puede estar vacío.\n"; continue; }
        if ((int)t.size() >= max) { cout << "ERROR: Código demasiado largo.\n"; continue; }
        strcpy(dest, t.c_str());
        return true;
    }
}

static bool leerOpcion(int* op, int maxOp) {
    return leerEntero("Seleccione una opción: ", op, 0, maxOp);
}

static int largoUtf8(const char* s) {
    int n = 0;
    for (; *s; ++s) if (((unsigned char)*s & 0xC0) != 0x80) n++;
    return n;
}

// Imprime s ocupando exactamente 'ancho' caracteres (cuenta caracteres UTF-8, no bytes)
static void imprimirPad(const char* s, int ancho, bool derecha) {
    string r;
    int n = 0;
    const char* p = s;
    while (*p && n < ancho) {
        unsigned char c = (unsigned char)*p;
        size_t len = 1;
        if (c >= 0xF0) len = 4; else if (c >= 0xE0) len = 3; else if (c >= 0xC0) len = 2;
        size_t resto = strlen(p);
        if (len > resto) len = resto;
        r.append(p, len);
        p += len;
        n++;
    }
    int pad = ancho - n;
    if (pad < 0) pad = 0;
    if (derecha) cout << string(pad, ' ') << r;
    else cout << r << string(pad, ' ');
}

static void rep(const char* s, int n) { for (int i = 0; i < n; i++) cout << s; }

static void celda(const char* s, int ancho, bool derecha = false) {
    cout << ' ';
    imprimirPad(s, ancho - 2, derecha);
    cout << ' ';
}

static void lineaTabla(const char* izq, const char* med, const char* der, const int* w, int n) {
    cout << izq;
    for (int i = 0; i < n; i++) {
        rep("-", w[i]);
        if (i < n - 1) cout << med;
    }
    cout << der << "\n";
}

static void filaMenu(const char* txt, bool centrar) {
    const int ancho = 43;
    cout << "|";
    if (centrar) {
        int izq = (ancho - largoUtf8(txt)) / 2;
        if (izq < 0) izq = 0;
        cout << string(izq, ' ');
        imprimirPad(txt, ancho - izq, false);
    } else {
        cout << ' ';
        imprimirPad(txt, ancho - 1, false);
    }
    cout << "|\n";
}

static void caja(const char* titulo, const char* sub, const char* const* items, int n) {
    cout << "+"; rep("-", 43); cout << "-\n";
    filaMenu(titulo, true);
    if (sub) filaMenu(sub, true);
    cout << "+"; rep("-", 43); cout << "+\n";
    for (int i = 0; i < n; i++) filaMenu(items[i], false);
    cout << "+"; rep("-", 43); cout << "+\n";
}

static void fmtMoneda(char* buf, size_t n, float v) { snprintf(buf, n, "%.2f", v); }


static void mostrarError(Resultado r, SistemaTaller* s, int idOrden, int idVeh, int idMec,
                         int idRep, int cantidad, int km) {
    Vehiculo* v = idVeh > 0 ? buscarVehiculoPorID(s, idVeh) : nullptr;
    Mecanico* m = idMec > 0 ? buscarMecanicoPorID(s, idMec) : nullptr;
    Repuesto* rp = idRep > 0 ? buscarRepuestoPorID(s, idRep) : nullptr;
    OrdenServicio* o = idOrden > 0 ? buscarOrdenPorID(s, idOrden) : nullptr;
    switch (r) {
        case RES_OK: break;
        case RES_VEHICULO_NO_EXISTE: cout << "ERROR: El vehiculo indicado no existe.\n"; break;
        case RES_MECANICO_NO_EXISTE: cout << "ERROR: El mecanico indicado no existe.\n"; break;
        case RES_REPUESTO_NO_EXISTE: cout << "ERROR: El repuesto indicado no existe.\n"; break;
        case RES_ORDEN_NO_EXISTE: cout << "ERROR: La orden #" << idOrden << " no existe.\n"; break;
        case RES_DUPLICADO: cout << "ERROR: Ya existe un registro con ese codigo o cedula.\n"; break;
        case RES_DATO_INVALIDO: cout << "ERROR: Alguno de los datos ingresados es invalido.\n"; break;
        case RES_VEHICULO_CON_ORDEN_ABIERTA: {
            OrdenServicio* ab = idVeh > 0 ? buscarOrdenAbiertaDeVehiculo(s, idVeh) : nullptr;
            cout << "ERROR: El vehiculo '" << (v ? v->placa : "?") << "' ya tiene la orden #"
                 << (ab ? ab->id : 0) << " ABIERTA.\n";
            break;
        }
        case RES_MECANICO_SATURADO:
            cout << "ERROR: El mecanico '" << (m ? m->nombre : "?") << "' ya tiene "
                 << (m ? contarOrdenesAbiertasMecanico(s, m->id) : 3) << " ordenes abiertas.\n";
            break;
        case RES_KM_MENOR:
            cout << "ERROR: El kilometraje " << km << " es menor al ultimo registrado del vehiculo '"
                 << (v ? v->placa : "?") << "' (" << (v ? v->kilometraje : 0) << " km).\n";
            break;
        case RES_STOCK_INSUFICIENTE:
            cout << "ERROR: Stock insuficiente de '" << (rp ? rp->nombre : "?") << "'. Disponible: "
                 << (rp ? rp->stock : 0) << ", solicitado: " << cantidad << ".\n";
            break;
        case RES_ESTADO_INVALIDO:
            cout << "ERROR: La orden #" << idOrden << " esta " << (o ? o->estado : "?")
                 << " y no puede modificarse.\n";
            break;
        case RES_REPUESTO_NO_EN_ORDEN:
            cout << "ERROR: El repuesto '" << (rp ? rp->nombre : "?") << "' no esta en la orden #" << idOrden << ".\n";
            break;
        case RES_ORDEN_VACIA:
            cout << "ERROR: La orden #" << idOrden << " no tiene repuestos ni mano de obra; no puede cerrarse.\n";
            break;
        case RES_REPUESTO_EN_USO:
            cout << "ERROR: El repuesto '" << (rp ? rp->nombre : "?")
                 << "' aparece en el detalle de ordenes y no puede eliminarse.\n";
            break;
    }
}

void mostrarVehiculo(Vehiculo* v) {
    cout << "\nVehiculo: " << v->placa << " - " << v->marca << " " << v->modelo << " " << v->anio << "\n";
    cout << "  Kilometraje : " << v->kilometraje << " km\n";
    cout << "  Propietario : " << v->nombrePropietario << " (Cedula: " << v->cedulaPropietario << ")\n";
    cout << "  Telefono    : " << v->telefonoPropietario << "\n";
    cout << "  Registrado  : " << v->fechaRegistro << " (ID " << v->id << ")\n";
}

void mostrarListaVehiculos(Vehiculo** lista, int cantidad) {
    const int w[] = {6, 11, 24, 7, 10, 22};
    cout << "\n";
    lineaTabla("╔", "╦", "╗", w, 6);
    cout << "|"; celda("ID", w[0]); cout << "|"; celda("Placa", w[1]); cout << "|"; celda("Vehiculo", w[2]);
    cout << "|"; celda("Ano", w[3]); cout << "|"; celda("Km", w[4]); cout << "|"; celda("Propietario", w[5]); cout << "|\n";
    lineaTabla("+", "-", "+", w, 6);
    for (int i = 0; i < cantidad; i++) {
        Vehiculo* v = lista[i];
        char b0[16], nom[120], b3[16], b4[16];
        snprintf(b0, sizeof b0, "%d", v->id);
        snprintf(nom, sizeof nom, "%s %s", v->marca, v->modelo);
        snprintf(b3, sizeof b3, "%d", v->anio);
        snprintf(b4, sizeof b4, "%d", v->kilometraje);
        cout << "|"; celda(b0, w[0]); cout << "|"; celda(v->placa, w[1]); cout << "|"; celda(nom, w[2]);
        cout << "|"; celda(b3, w[3]); cout << "|"; celda(b4, w[4], true); cout << "|"; celda(v->nombrePropietario, w[5]); cout << "|\n";
    }
    lineaTabla("+", "-", "+", w, 6);
    cout << "Total de vehiculos: " << cantidad << "\n";
}

void menuRegistrarVehiculo(SistemaTaller* s) {
    cout << "\n--- REGISTRAR VEHICULO (escriba CANCELAR en cualquier momento para abortar) ---\n";
    Vehiculo d;
    memset(&d, 0, sizeof d);
    if (!leerPlaca("Placa: ", d.placa, sizeof d.placa)) return;
    if (buscarVehiculoPorPlaca(s, d.placa)) {
        cout << "ERROR: La placa '" << d.placa << "' ya esta registrada.\n";
        return;
    }
    if (!leerTexto("Marca: ", d.marca, sizeof d.marca)) return;
    if (!leerTexto("Modelo: ", d.modelo, sizeof d.modelo)) return;
    if (!leerEntero("Ano (1950-2027): ", &d.anio, 1950, 2027)) return;
    if (!leerEntero("Kilometraje: ", &d.kilometraje, 0, INT_MAX)) return;
    if (!leerTexto("Nombre del propietario: ", d.nombrePropietario, sizeof d.nombrePropietario)) return;
    if (!leerCedula("Cedula del propietario: ", d.cedulaPropietario, sizeof d.cedulaPropietario)) return;
    if (!leerTexto("Telefono del propietario: ", d.telefonoPropietario, sizeof d.telefonoPropietario)) return;
    bool ok;
    if (!leerSN("Confirmar el registro (S/N): ", &ok)) return;
    if (!ok) { cout << "Registro descartado.\n"; return; }
    Vehiculo* v = agregarVehiculo(s, d);
    if (!v) cout << "ERROR: No se pudo registrar el vehiculo '" << d.placa << "' (datos invalidos).\n";
    else cout << "Vehiculo '" << v->placa << "' (" << v->marca << " " << v->modelo << ") registrado correctamente.\n";
}

void menuBuscarVehiculo(SistemaTaller* s) {
    cout << "\nBuscar por: 1. Placa   2. Propietario   3. ID\n";
    int modo;
    if (!leerEntero("Seleccione: ", &modo, 1, 3)) return;
    if (modo == 1) {
        char placa[10];
        if (!leerPlaca("Placa exacta: ", placa, sizeof placa)) return;
        Vehiculo* v = buscarVehiculoPorPlaca(s, placa);
        if (v) mostrarVehiculo(v);
        else cout << "ERROR: No existe un vehiculo con la placa '" << placa << "'.\n";
    } else if (modo == 2) {
        char sub[100];
        if (!leerTexto("Nombre del propietario (parcial): ", sub, sizeof sub)) return;
        int n;
        Vehiculo** r = buscarVehiculosPorPropietario(s, sub, &n);
        if (n == 0) cout << "No hay vehiculos cuyo propietario contenga '" << sub << "'.\n";
        else mostrarListaVehiculos(r, n);
        delete[] r;   // solo el arreglo de punteros
    } else {
        int id;
        if (!leerEntero("ID del vehiculo: ", &id, 1, INT_MAX)) return;
        Vehiculo* v = buscarVehiculoPorID(s, id);
        if (v) mostrarVehiculo(v);
        else cout << "ERROR: No existe un vehiulo con ID " << id << ".\n";
    }
}

void menuActualizarVehiculo(SistemaTaller* s) {
    cout << "\n--- ACTUALIZAR VEHICULO (escriba CANCELAR para abortar) ---\n";
    char placa[10];
    if (!leerPlaca("Placa del vehiculo a actualizar: ", placa, sizeof placa)) return;
    Vehiculo* v = buscarVehiculoPorPlaca(s, placa);
    if (!v) { cout << "ERROR: No existe un vehiculo con la placa '" << placa << "'.\n"; return; }
    mostrarVehiculo(v);
    cout << "(La placa, el kilometraje y la fecha de registro no se modifican)\n";
    Vehiculo d;
    memset(&d, 0, sizeof d);
    if (!leerTexto("Nueva marca: ", d.marca, sizeof d.marca)) return;
    if (!leerTexto("Nuevo modelo: ", d.modelo, sizeof d.modelo)) return;
    if (!leerEntero("Nuevo ano (1950-2027): ", &d.anio, 1950, 2027)) return;
    if (!leerTexto("Nuevo nombre del propietario: ", d.nombrePropietario, sizeof d.nombrePropietario)) return;
    if (!leerCedula("Nueva cedula del propietario: ", d.cedulaPropietario, sizeof d.cedulaPropietario)) return;
    if (!leerTexto("Nuevo telefono del propietario: ", d.telefonoPropietario, sizeof d.telefonoPropietario)) return;
    bool ok;
    if (!leerSN("Confirmar la actualizacion (S/N): ", &ok)) return;
    if (!ok) { cout << "Actualizacion descartada.\n"; return; }
    if (actualizarVehiculo(s, v->id, d)) cout << "Vehiculo '" << placa << "' actualizado correctamente.\n";
    else cout << "ERROR: No se pudo actualizar el vehiculo '" << placa << "'.\n";
}

void menuEliminarVehiculo(SistemaTaller* s) {
    cout << "\n--- ELIMINAR VEHICULO (escriba CANCELAR para abortar) ---\n";
    char placa[10];
    if (!leerPlaca("Placa del vehiculo a eliminar: ", placa, sizeof placa)) return;
    Vehiculo* v = buscarVehiculoPorPlaca(s, placa);
    if (!v) { cout << "ERROR: No existe un vehiculo con la placa '" << placa << "'.\n"; return; }
    int n = contarOrdenesVehiculo(s, v->id);
    if (n > 0) {
        cout << "ERROR: El vehiculo '" << placa << "' tiene " << n
             << (n == 1 ? " orden registrada" : " ordenes registradas") << " y no puede eliminarse.\n";
        return;
    }
    bool ok;
    if (!leerSN("Confirmar la eliminacion (S/N): ", &ok)) return;
    if (!ok) { cout << "Eliminacion descartada.\n"; return; }
    if (eliminarVehiculo(s, v->id)) cout << "Vehiculo '" << placa << "' eliminado.\n";
    else cout << "ERROR: No se pudo eliminar el vehiculo '" << placa << "'.\n";
}

void menuListarVehiculos(SistemaTaller* s) {
    int n;
    Vehiculo** lista = listarVehiculos(s, &n);
    if (n == 0) cout << "No hay vehiculos registrados.\n";
    else {
        mostrarListaVehiculos(lista, n);
        cout << "(Capacidad actual del arreglo de vehiculos: " << s->capacidadVehiculos << ")\n";
    }
    delete[] lista;
}

void menuVehiculos(SistemaTaller* s) {
    const char* items[] = {"1. Registrar vehiculo", "2. Buscar vehiculo", "3. Actualizar vehiculo",
                           "4. Eliminar vehiculo", "5. Listar vehiculos", "0. Volver al menu principal"};
    while (!gEOF) {
        cout << "\n";
        caja("GESTION DE VEHICULOS", nullptr, items, 6);
        int op;
        if (!leerOpcion(&op, 5)) continue;
        switch (op) {
            case 1: menuRegistrarVehiculo(s); break;
            case 2: menuBuscarVehiculo(s); break;
            case 3: menuActualizarVehiculo(s); break;
            case 4: menuEliminarVehiculo(s); break;
            case 5: menuListarVehiculos(s); break;
            case 0: return;
        }
    }
}
//mecanicos
void mostrarMecanico(Mecanico* m, SistemaTaller* s) {
    cout << "\nMecánico: " << m->nombre << " (" << m->especialidad << ")\n";
    cout << "  Cédula        : " << m->cedula << "\n";
    cout << "  Ingreso       : " << m->fechaIngreso << " (ID " << m->id << ")\n";
    cout << "  Órdenes abiertas: " << contarOrdenesAbiertasMecanico(s, m->id) << "\n";
}

void mostrarListaMecanicos(Mecanico** lista, int cantidad, SistemaTaller* s) {
    const int w[] = {6, 24, 16, 16, 13, 12};
    cout << "\n";
    lineaTabla("╔", "╦", "╗", w, 6);
    cout << "║"; celda("ID", w[0]); cout << "║"; celda("Nombre", w[1]); cout << "║"; celda("Cédula", w[2]);
    cout << "║"; celda("Especialidad", w[3]); cout << "║"; celda("Ingreso", w[4]); cout << "║"; celda("Abiertas", w[5]); cout << "║\n";
    lineaTabla("╠", "╬", "╣", w, 6);
    for (int i = 0; i < cantidad; i++) {
        Mecanico* m = lista[i];
        char b0[16], b5[16];
        snprintf(b0, sizeof b0, "%d", m->id);
        snprintf(b5, sizeof b5, "%d", contarOrdenesAbiertasMecanico(s, m->id));
        cout << "║"; celda(b0, w[0]); cout << "║"; celda(m->nombre, w[1]); cout << "║"; celda(m->cedula, w[2]);
        cout << "║"; celda(m->especialidad, w[3]); cout << "║"; celda(m->fechaIngreso, w[4]); cout << "║"; celda(b5, w[5], true); cout << "║\n";
    }
    lineaTabla("╚", "╩", "╝", w, 6);
    cout << "Total de mecánicos: " << cantidad << "\n";
}

void menuRegistrarMecanico(SistemaTaller* s) {
    cout << "\n--- REGISTRAR MECÁNICO (escriba CANCELAR para abortar) ---\n";
    Mecanico d;
    memset(&d, 0, sizeof d);
    if (!leerTexto("Nombre completo: ", d.nombre, sizeof d.nombre)) return;
    if (!leerCedula("Cédula: ", d.cedula, sizeof d.cedula)) return;
    if (!leerEspecialidad("Especialidad (MOTOR/FRENOS/ELECTRICIDAD/SUSPENSION/GENERAL): ", d.especialidad, sizeof d.especialidad)) return;
    if (!leerFecha("Fecha de ingreso (YYYY-MM-DD): ", d.fechaIngreso)) return;
    bool ok;
    if (!leerSN("¿Confirmar el registro? (S/N): ", &ok)) return;
    if (!ok) { cout << "Registro descartado.\n"; return; }
    Resultado res;
    Mecanico* m = agregarMecanico(s, d, &res);
    if (m) cout << "Mecánico '" << m->nombre << "' (" << m->especialidad << ") registrado correctamente.\n";
    else if (res == RES_DUPLICADO) cout << "ERROR: La cédula '" << d.cedula << "' ya está registrada para otro mecánico.\n";
    else mostrarError(res, s, 0, 0, 0, 0, 0, 0);
}

void menuBuscarMecanico(SistemaTaller* s) {
    int id;
    if (!leerEntero("ID del mecánico: ", &id, 1, INT_MAX)) return;
    Mecanico* m = buscarMecanicoPorID(s, id);
    if (m) mostrarMecanico(m, s);
    else cout << "ERROR: No existe un mecánico con ID " << id << ".\n";
}

void menuListarMecanicos(SistemaTaller* s) {
    int n;
    Mecanico** lista = listarMecanicos(s, &n);
    if (n == 0) cout << "No hay mecánicos registrados.\n";
    else mostrarListaMecanicos(lista, n, s);
    delete[] lista;
}

void menuMecanicos(SistemaTaller* s) {
    const char* items[] = {"1. Registrar mecánico", "2. Buscar mecánico por ID", "3. Listar mecánicos",
                           "0. Volver al menú principal"};
    while (!gEOF) {
        cout << "\n";
        caja("GESTIÓN DE MECÁNICOS", nullptr, items, 4);
        int op;
        if (!leerOpcion(&op, 3)) continue;
        switch (op) {
            case 1: menuRegistrarMecanico(s); break;
            case 2: menuBuscarMecanico(s); break;
            case 3: menuListarMecanicos(s); break;
            case 0: return;
        }
    }
}

//repuestos
void mostrarRepuesto(Repuesto* r) {
    cout << "\nRepuesto: " << r->codigo << " - " << r->nombre << " (" << r->marca << ")\n";
    cout << "  Precio vigente : " << fixed;
    cout.precision(2);
    cout << r->precio << "\n";
    cout << "  Stock          : " << r->stock << " (mínimo: " << r->stockMinimo << ")"
         << (estaBajoStock(r) ? "  *** BAJO STOCK ***" : "") << "\n";
    cout << "  Registrado     : " << r->fechaRegistro << " (ID " << r->id << ")\n";
}

void mostrarListaRepuestos(Repuesto** lista, int cantidad) {
    const int w[] = {6, 11, 26, 12, 10, 8, 7, 14};
    cout << "\n";
    lineaTabla("╔", "╦", "╗", w, 8);
    cout << "║"; celda("ID", w[0]); cout << "║"; celda("Código", w[1]); cout << "║"; celda("Nombre", w[2]);
    cout << "║"; celda("Marca", w[3]); cout << "║"; celda("Precio", w[4]); cout << "║"; celda("Stock", w[5]);
    cout << "║"; celda("Mín", w[6]); cout << "║"; celda("Estado", w[7]); cout << "║\n";
    lineaTabla("╠", "╬", "╣", w, 8);
    for (int i = 0; i < cantidad; i++) {
        Repuesto* r = lista[i];
        char b0[16], b4[24], b5[16], b6[16];
        snprintf(b0, sizeof b0, "%d", r->id);
        fmtMoneda(b4, sizeof b4, r->precio);
        snprintf(b5, sizeof b5, "%d", r->stock);
        snprintf(b6, sizeof b6, "%d", r->stockMinimo);
        cout << "║"; celda(b0, w[0]); cout << "║"; celda(r->codigo, w[1]); cout << "║"; celda(r->nombre, w[2]);
        cout << "║"; celda(r->marca, w[3]); cout << "║"; celda(b4, w[4], true); cout << "║"; celda(b5, w[5], true);
        cout << "║"; celda(b6, w[6], true); cout << "║"; celda(estaBajoStock(r) ? "BAJO STOCK" : "OK", w[7]); cout << "║\n";
    }
    lineaTabla("╚", "╩", "╝", w, 8);
    cout << "Total de repuestos: " << cantidad << "\n";
}

void menuRegistrarRepuesto(SistemaTaller* s) {
    cout << "\n--- REGISTRAR REPUESTO (escriba CANCELAR para abortar) ---\n";
    Repuesto d;
    memset(&d, 0, sizeof d);
    if (!leerCodigo("Código: ", d.codigo, sizeof d.codigo)) return;
    if (buscarRepuestoPorCodigo(s, d.codigo)) {
        cout << "ERROR: El código '" << d.codigo << "' ya está registrado.\n";
        return;
    }
    if (!leerTexto("Nombre: ", d.nombre, sizeof d.nombre)) return;
    if (!leerTexto("Marca: ", d.marca, sizeof d.marca)) return;
    if (!leerFloat("Precio unitario (> 0): ", &d.precio, 0, true)) return;
    if (!leerEntero("Stock inicial (>= 0): ", &d.stock, 0, INT_MAX)) return;
    if (!leerEntero("Stock mínimo (>= 0): ", &d.stockMinimo, 0, INT_MAX)) return;
    bool ok;
    if (!leerSN("¿Confirmar el registro? (S/N): ", &ok)) return;
    if (!ok) { cout << "Registro descartado.\n"; return; }
    Resultado res;
    Repuesto* r = agregarRepuesto(s, d, &res);
    if (r) cout << "Repuesto '" << r->codigo << " - " << r->nombre << "' registrado correctamente.\n";
    else mostrarError(res, s, 0, 0, 0, 0, 0, 0);
}

void menuBuscarRepuesto(SistemaTaller* s) {
    cout << "\nBuscar por: 1. ID   2. Código o nombre parcial\n";
    int modo;
    if (!leerEntero("Seleccione: ", &modo, 1, 2)) return;
    if (modo == 1) {
        int id;
        if (!leerEntero("ID del repuesto: ", &id, 1, INT_MAX)) return;
        Repuesto* r = buscarRepuestoPorID(s, id);
        if (r) mostrarRepuesto(r);
        else cout << "ERROR: No existe un repuesto con ID " << id << ".\n";
    } else {
        char sub[100];
        if (!leerTexto("Código o nombre (parcial): ", sub, sizeof sub)) return;
        int n;
        Repuesto** lista = buscarRepuestosPorTexto(s, sub, &n);
        if (n == 0) cout << "No hay repuestos que coincidan con '" << sub << "'.\n";
        else mostrarListaRepuestos(lista, n);
        delete[] lista;
    }
}

void menuEntradaMercancia(SistemaTaller* s) {
    cout << "\n--- ENTRADA DE MERCANCÍA (escriba CANCELAR para abortar) ---\n";
    char cod[20];
    if (!leerCodigo("Código del repuesto: ", cod, sizeof cod)) return;
    Repuesto* r = buscarRepuestoPorCodigo(s, cod);
    if (!r) { cout << "ERROR: No existe un repuesto con el código '" << cod << "'.\n"; return; }
    cout << "Repuesto: " << r->nombre << " - stock actual: " << r->stock << "\n";
    int cant;
    if (!leerEntero("Cantidad que ingresa (> 0): ", &cant, 1, INT_MAX)) return;
    bool ok;
    if (!leerSN("¿Confirmar la entrada? (S/N): ", &ok)) return;
    if (!ok) { cout << "Entrada descartada.\n"; return; }
    int antes = r->stock;
    Resultado res = registrarEntradaMercancia(s, r->id, cant);
    if (res == RES_OK) cout << "Entrada registrada. Stock de '" << r->nombre << "': " << antes << " -> " << r->stock << ".\n";
    else mostrarError(res, s, 0, 0, 0, r->id, cant, 0);
}

void menuCambiarPrecio(SistemaTaller* s) {
    cout << "\n--- CAMBIAR PRECIO (escriba CANCELAR para abortar) ---\n";
    char cod[20];
    if (!leerCodigo("Código del repuesto: ", cod, sizeof cod)) return;
    Repuesto* r = buscarRepuestoPorCodigo(s, cod);
    if (!r) { cout << "ERROR: No existe un repuesto con el código '" << cod << "'.\n"; return; }
    cout << fixed;
    cout.precision(2);
    cout << "Repuesto: " << r->nombre << " - precio actual: " << r->precio << "\n";
    float nuevo;
    if (!leerFloat("Nuevo precio (> 0): ", &nuevo, 0, true)) return;
    bool ok;
    if (!leerSN("¿Confirmar el cambio de precio? (S/N): ", &ok)) return;
    if (!ok) { cout << "Cambio descartado.\n"; return; }
    float antes = r->precio;
    Resultado res = cambiarPrecioRepuesto(s, r->id, nuevo);
    if (res == RES_OK) cout << "Precio de '" << r->nombre << "': " << antes << " -> " << r->precio
                            << " (las órdenes ya registradas no cambian).\n";
    else mostrarError(res, s, 0, 0, 0, r->id, 0, 0);
}

void menuEliminarRepuesto(SistemaTaller* s) {
    cout << "\n--- ELIMINAR REPUESTO (escriba CANCELAR para abortar) ---\n";
    char cod[20];
    if (!leerCodigo("Código del repuesto: ", cod, sizeof cod)) return;
    Repuesto* r = buscarRepuestoPorCodigo(s, cod);
    if (!r) { cout << "ERROR: No existe un repuesto con el código '" << cod << "'.\n"; return; }
    if (repuestoEnUso(s, r->id)) { mostrarError(RES_REPUESTO_EN_USO, s, 0, 0, 0, r->id, 0, 0); return; }
    bool ok;
    if (!leerSN("¿Confirmar la eliminación? (S/N): ", &ok)) return;
    if (!ok) { cout << "Eliminación descartada.\n"; return; }
    Resultado res = eliminarRepuesto(s, r->id);
    if (res == RES_OK) cout << "Repuesto '" << cod << "' eliminado.\n";
    else mostrarError(res, s, 0, 0, 0, r->id, 0, 0);
}

void menuListarRepuestos(SistemaTaller* s) {
    int n;
    Repuesto** lista = listarRepuestos(s, &n);
    if (n == 0) cout << "No hay repuestos registrados.\n";
    else mostrarListaRepuestos(lista, n);
    delete[] lista;
}

void menuInventario(SistemaTaller* s) {
    const char* items[] = {"1. Registrar repuesto", "2. Buscar repuesto", "3. Registrar entrada de mercancía",
                           "4. Cambiar precio", "5. Eliminar repuesto", "6. Listar repuestos",
                           "0. Volver al menú principal"};
    while (!gEOF) {
        cout << "\n";
        caja("INVENTARIO DE REPUESTOS", nullptr, items, 7);
        int op;
        if (!leerOpcion(&op, 6)) continue;
        switch (op) {
            case 1: menuRegistrarRepuesto(s); break;
            case 2: menuBuscarRepuesto(s); break;
            case 3: menuEntradaMercancia(s); break;
            case 4: menuCambiarPrecio(s); break;
            case 5: menuEliminarRepuesto(s); break;
            case 6: menuListarRepuestos(s); break;
            case 0: return;
        }
    }
}
