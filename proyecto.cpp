#include <cstring>
#include <cctype>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <climits>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#ifdef _WIN32
#include <io.h>
#define ES_TTY() _isatty(_fileno(stdin))
#else
#include <unistd.h>
#define ES_TTY() isatty(fileno(stdin))
#endif

//constantes
const int MAX_DETALLES = 20;        // lineas de detalle por orden 
const int MAX_RESULTADOS = 50;      // maximo de registros que entrega una busqueda o listado
const int MAX_PROBLEMAS = 100;      // maximo de mensajes que guarda el reporte de integridad
const int TOP_REPUESTOS = 10;       // tamano del ranking de repuestos mas usados
const int MAX_ORDENES_ABIERTAS_MECANICO = 3;

const char* const DIR_DATOS = "datos";
const char* const RUTA_TALLER = "datos/taller.bin";
const char* const RUTA_VEHICULOS = "datos/vehiculos.bin";
const char* const RUTA_MECANICOS = "datos/mecanicos.bin";
const char* const RUTA_REPUESTOS = "datos/repuestos.bin";
const char* const RUTA_ORDENES = "datos/ordenes.bin";


// estructuras
struct ArchivoHeader {
    int cantidadRegistros;   // total historico (incluye eliminados logicamente)
    int proximoID;           // siguiente ID a asignar
    int registrosActivos;    // registros con eliminado == false
    int version;             // version del formato del archivo
};

struct Taller {
    char nombre[100];
    char rif[20];
    char direccion[200];
    char telefono[20];
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
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
    bool eliminado;
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
};

struct Mecanico {
    int id;
    char nombre[100];
    char cedula[20];
    char especialidad[20];
    char fechaIngreso[11];
    bool eliminado;          // true = dado de baja
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
};

struct Repuesto {
    int id;
    char codigo[20];
    char nombre[100];
    char marca[50];
    float precio;
    int stock;
    int stockMinimo;
    bool eliminado;
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
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
    DetalleOrden detalles[MAX_DETALLES];   
    int numDetalles;                       
    bool eliminado;          
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
};
struct ReporteIntegridad {
    int vehiculos, mecanicos, repuestos, ordenes;   // registros verificados
    int problemas;                                  // total de problemas hallados
    int mensajesGuardados;                          // cuantos mensajes caben en 'mensajes'
    char mensajes[MAX_PROBLEMAS][160];
};

struct FilaUsoRepuesto {
    int idRepuesto;
    int unidades;
};

struct FilaFacturacion {
    Mecanico mecanico;
    int ordenesCerradas;
    float manoObra;
    float repuestos;
};

// Codigo de estado de la capa de persistencia
enum EstadoArchivo {
    ARCH_OK = 0,
    ARCH_NO_ABRE,           // el archivo no se pudo abrir
    ARCH_ID_INEXISTENTE,    // el ID no corresponde a ningun registro del archivo
    ARCH_YA_ELIMINADO,      // el registro ya estaba marcado como eliminado
    ARCH_ERROR_IO           // fallo de lectura o escritura
};

// Codigo de resultado para las operaciones de la capa de logica
enum Resultado {
    RES_OK = 0,
    RES_ERROR_ARCHIVO,
    RES_VEHICULO_NO_EXISTE,
    RES_VEHICULO_ELIMINADO,
    RES_MECANICO_NO_EXISTE,
    RES_MECANICO_DADO_BAJA,
    RES_REPUESTO_NO_EXISTE,
    RES_REPUESTO_ELIMINADO,
    RES_ORDEN_NO_EXISTE,
    RES_DUPLICADO,
    RES_DATO_INVALIDO,
    RES_VEHICULO_CON_ORDEN_ABIERTA,
    RES_VEHICULO_CON_ORDENES,
    RES_MECANICO_SATURADO,
    RES_KM_MENOR,
    RES_STOCK_INSUFICIENTE,
    RES_ESTADO_INVALIDO,
    RES_REPUESTO_NO_EN_ORDEN,
    RES_ORDEN_VACIA,
    RES_REPUESTO_EN_USO,
    RES_LIMITE_DETALLES,
};


 // utilidades generales

static void copiar(char* dest, const char* src, int n) {
    strncpy(dest, src, n - 1);
    dest[n - 1] = '\0';
}

static bool vacio(const char* s) {
    for (; *s; ++s) if (!isspace((unsigned char)*s)) return false;
    return true;
}

static void aMayusculas(char* s) {
    for (; *s; ++s) *s = (char)toupper((unsigned char)*s);
}

static bool esEstado(const OrdenServicio* o, const char* estado) {
    return strcmp(o->estado, estado) == 0;
}

void fechaActual(char* out) {
    time_t t = time(nullptr);
    struct tm* l = localtime(&t);
    strftime(out, 11, "%Y-%m-%d", l);
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

// capa de persistencia
// Lee y escribe registros crudos en archivos binarios con <fstream>.

// Nombre del ultimo archivo que fallo (para que la presentacion lo informe)
static char gArchivoConError[160] = "";

static void registrarFalloArchivo(const char* ruta) {
    strncpy(gArchivoConError, ruta, sizeof gArchivoConError - 1);
    gArchivoConError[sizeof gArchivoConError - 1] = '\0';
}

const char* ultimoArchivoConError() { return gArchivoConError; }

// posicion en bytes del registro con indice fisico i: sizeof(header) + i * sizeof(Estructura)
static long long posicionRegistro(size_t tamRegistro, int indice) {
    return (long long)sizeof(ArchivoHeader) + (long long)indice * (long long)tamRegistro;
}

// cantidad de registros que realmente caben en el archivo, calculada con su tamano en bytes
static long long totalRegistrosEnStream(std::istream& f, size_t tamRegistro) {
    f.seekg(0, std::ios::end);
    long long tam = (long long)f.tellg();
    if (tam < (long long)sizeof(ArchivoHeader)) return 0;
    return (tam - (long long)sizeof(ArchivoHeader)) / (long long)tamRegistro;
}

// -1 si el archivo no se puede abrir
int totalRegistrosEnArchivo(const char* ruta, size_t tamRegistro) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(ruta); return -1; }
    return (int)totalRegistrosEnStream(f, tamRegistro);
}

// Operaciones sobre el header (cualquier archivo de entidades) 

bool leerHeader(const char* ruta, ArchivoHeader& h) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(ruta); return false; }
    f.seekg(0, std::ios::beg);
    f.read(reinterpret_cast<char*>(&h), sizeof h);
    if (!f) { registrarFalloArchivo(ruta); return false; }
    return true;
}

// Sobrescribe unicamente el header (posicion 0); no toca los registros
bool escribirHeader(const char* ruta, const ArchivoHeader& h) {
    std::fstream f(ruta, std::ios::in | std::ios::out | std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(ruta); return false; }
    f.seekp(0, std::ios::beg);
    f.write(reinterpret_cast<const char*>(&h), sizeof h);
    f.flush();
    if (!f) { registrarFalloArchivo(ruta); return false; }
    return true;
}


bool inicializarArchivo(const char* ruta) {
    {
        std::ifstream existente(ruta, std::ios::binary);
        if (existente.is_open()) {
            ArchivoHeader h;
            existente.read(reinterpret_cast<char*>(&h), sizeof h);
            if (!existente) { registrarFalloArchivo(ruta); return false; }
            return true;
        }
    }
    std::ofstream nuevo(ruta, std::ios::binary);
    if (!nuevo.is_open()) { registrarFalloArchivo(ruta); return false; }
    ArchivoHeader h;
    h.cantidadRegistros = 0;
    h.proximoID = 1;
    h.registrosActivos = 0;
    h.version = 1;
    nuevo.write(reinterpret_cast<const char*>(&h), sizeof h);
    nuevo.flush();
    if (!nuevo) { registrarFalloArchivo(ruta); return false; }
    return true;
}

bool crearCarpeta(const char* ruta) {
    std::error_code ec;
    std::filesystem::create_directories(ruta, ec);
    if (ec || !std::filesystem::is_directory(ruta, ec)) { registrarFalloArchivo(ruta); return false; }
    return true;
}

bool existeArchivo(const char* ruta) {
    std::ifstream f(ruta, std::ios::binary);
    return f.is_open();
}

// Copia byte a byte un archivo binario
bool copiarArchivoBinario(const char* origen, const char* destino) {
    std::ifstream in(origen, std::ios::binary);
    if (!in.is_open()) { registrarFalloArchivo(origen); return false; }
    std::ofstream out(destino, std::ios::binary);
    if (!out.is_open()) { registrarFalloArchivo(destino); return false; }
    char buffer[4096];
    while (in) {
        in.read(buffer, sizeof buffer);
        std::streamsize leidos = in.gcount();
        if (leidos > 0) out.write(buffer, leidos);
    }
    out.flush();
    if (!out) { registrarFalloArchivo(destino); return false; }
    return true;
}

bool existeArchivoTaller() { return existeArchivo(RUTA_TALLER); }

bool leerTaller(Taller& t) {
    std::ifstream f(RUTA_TALLER, std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_TALLER); return false; }
    f.seekg(0, std::ios::beg);
    f.read(reinterpret_cast<char*>(&t), sizeof t);
    if (!f) { registrarFalloArchivo(RUTA_TALLER); return false; }
    return true;
}

bool guardarTaller(Taller& t) {
    time_t ahora = time(nullptr);
    t.fechaCreacion = ahora;
    t.fechaUltimaModificacion = ahora;
    std::ofstream f(RUTA_TALLER, std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_TALLER); return false; }
    f.write(reinterpret_cast<const char*>(&t), sizeof t);
    f.flush();
    if (!f) { registrarFalloArchivo(RUTA_TALLER); return false; }
    return true;
}

// Crea la carpeta datos/ y los archivos de entidades que falten (con su header inicial).
bool inicializarArchivosSistema() {
    if (!crearCarpeta(DIR_DATOS)) return false;
    return inicializarArchivo(RUTA_VEHICULOS) && inicializarArchivo(RUTA_MECANICOS) &&
           inicializarArchivo(RUTA_REPUESTOS) && inicializarArchivo(RUTA_ORDENES);
}

int registrosFisicosVehiculos() { return totalRegistrosEnArchivo(RUTA_VEHICULOS, sizeof(Vehiculo)); }

// 1.- Guardar un registro nuevo
EstadoArchivo guardarVehiculo(Vehiculo& x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return ARCH_NO_ABRE;
    time_t ahora = time(nullptr);
    x.id = h.proximoID;
    x.eliminado = false;
    x.fechaCreacion = ahora;
    x.fechaUltimaModificacion = ahora;
    std::fstream f(RUTA_VEHICULOS, std::ios::in | std::ios::out | std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_VEHICULOS); return ARCH_NO_ABRE; }
    f.seekp(posicionRegistro(sizeof(Vehiculo), h.cantidadRegistros), std::ios::beg);
    f.write(reinterpret_cast<const char*>(&x), sizeof(Vehiculo));
    f.flush();
    if (!f) { registrarFalloArchivo(RUTA_VEHICULOS); return ARCH_ERROR_IO; }
    f.close();
    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!escribirHeader(RUTA_VEHICULOS, h)) return ARCH_ERROR_IO;
    return ARCH_OK;
}

// 2.- Leer por ID
EstadoArchivo leerVehiculoPorID(int id, Vehiculo& out, long long* posicion = nullptr) {
    std::ifstream f(RUTA_VEHICULOS, std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_VEHICULOS); return ARCH_NO_ABRE; }
    long long total = totalRegistrosEnStream(f, sizeof(Vehiculo));
    if (id < 1 || id > total) return ARCH_ID_INEXISTENTE;
    long long pos = posicionRegistro(sizeof(Vehiculo), id - 1);
    f.seekg(pos, std::ios::beg);
    f.read(reinterpret_cast<char*>(&out), sizeof(Vehiculo));
    if (!f) { registrarFalloArchivo(RUTA_VEHICULOS); return ARCH_ERROR_IO; }
    if (posicion) *posicion = pos;
    return ARCH_OK;
}

// 3.- Sobrescribir el registro en su posicion original y actualizar fechaUltimaModificacion
EstadoArchivo sobrescribirVehiculo(Vehiculo& x) {
    std::fstream f(RUTA_VEHICULOS, std::ios::in | std::ios::out | std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_VEHICULOS); return ARCH_NO_ABRE; }
    long long total = totalRegistrosEnStream(f, sizeof(Vehiculo));
    if (x.id < 1 || x.id > total) return ARCH_ID_INEXISTENTE;
    x.fechaUltimaModificacion = time(nullptr);
    f.seekp(posicionRegistro(sizeof(Vehiculo), x.id - 1), std::ios::beg);
    f.write(reinterpret_cast<const char*>(&x), sizeof(Vehiculo));
    f.flush();
    if (!f) { registrarFalloArchivo(RUTA_VEHICULOS); return ARCH_ERROR_IO; }
    return ARCH_OK;
}

// 4.- Borrado logico
EstadoArchivo marcarVehiculoEliminado(int id) {
    Vehiculo x;
    EstadoArchivo e = leerVehiculoPorID(id, x);
    if (e != ARCH_OK) return e;
    if (x.eliminado) return ARCH_YA_ELIMINADO;
    x.eliminado = true;
    e = sobrescribirVehiculo(x);
    if (e != ARCH_OK) return e;
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return ARCH_NO_ABRE;
    h.registrosActivos--;
    if (!escribirHeader(RUTA_VEHICULOS, h)) return ARCH_ERROR_IO;
    return ARCH_OK;
}

// 5.- Contar activos
int contarVehiculosActivos() {
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return -1;
    return h.registrosActivos;
}

int registrosFisicosMecanicos() { return totalRegistrosEnArchivo(RUTA_MECANICOS, sizeof(Mecanico)); }

// 1.- Guardar un registro nuevo
EstadoArchivo guardarMecanico(Mecanico& x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return ARCH_NO_ABRE;
    time_t ahora = time(nullptr);
    x.id = h.proximoID;
    x.eliminado = false;
    x.fechaCreacion = ahora;
    x.fechaUltimaModificacion = ahora;
    std::fstream f(RUTA_MECANICOS, std::ios::in | std::ios::out | std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_MECANICOS); return ARCH_NO_ABRE; }
    f.seekp(posicionRegistro(sizeof(Mecanico), h.cantidadRegistros), std::ios::beg);
    f.write(reinterpret_cast<const char*>(&x), sizeof(Mecanico));
    f.flush();
    if (!f) { registrarFalloArchivo(RUTA_MECANICOS); return ARCH_ERROR_IO; }
    f.close();
    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!escribirHeader(RUTA_MECANICOS, h)) return ARCH_ERROR_IO;
    return ARCH_OK;
}

// 2.- Leer por ID
EstadoArchivo leerMecanicoPorID(int id, Mecanico& out, long long* posicion = nullptr) {
    std::ifstream f(RUTA_MECANICOS, std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_MECANICOS); return ARCH_NO_ABRE; }
    long long total = totalRegistrosEnStream(f, sizeof(Mecanico));
    if (id < 1 || id > total) return ARCH_ID_INEXISTENTE;
    long long pos = posicionRegistro(sizeof(Mecanico), id - 1);
    f.seekg(pos, std::ios::beg);
    f.read(reinterpret_cast<char*>(&out), sizeof(Mecanico));
    if (!f) { registrarFalloArchivo(RUTA_MECANICOS); return ARCH_ERROR_IO; }
    if (posicion) *posicion = pos;
    return ARCH_OK;
}

// 3.- Sobrescribir el registro en su posicion original y actualizar fechaUltimaModificacion
EstadoArchivo sobrescribirMecanico(Mecanico& x) {
    std::fstream f(RUTA_MECANICOS, std::ios::in | std::ios::out | std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_MECANICOS); return ARCH_NO_ABRE; }
    long long total = totalRegistrosEnStream(f, sizeof(Mecanico));
    if (x.id < 1 || x.id > total) return ARCH_ID_INEXISTENTE;
    x.fechaUltimaModificacion = time(nullptr);
    f.seekp(posicionRegistro(sizeof(Mecanico), x.id - 1), std::ios::beg);
    f.write(reinterpret_cast<const char*>(&x), sizeof(Mecanico));
    f.flush();
    if (!f) { registrarFalloArchivo(RUTA_MECANICOS); return ARCH_ERROR_IO; }
    return ARCH_OK;
}

// 4.- Borrado logico
EstadoArchivo marcarMecanicoEliminado(int id) {
    Mecanico x;
    EstadoArchivo e = leerMecanicoPorID(id, x);
    if (e != ARCH_OK) return e;
    if (x.eliminado) return ARCH_YA_ELIMINADO;
    x.eliminado = true;
    e = sobrescribirMecanico(x);
    if (e != ARCH_OK) return e;
    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return ARCH_NO_ABRE;
    h.registrosActivos--;
    if (!escribirHeader(RUTA_MECANICOS, h)) return ARCH_ERROR_IO;
    return ARCH_OK;
}

// 5.- Contar activos
int contarMecanicosActivos() {
    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return -1;
    return h.registrosActivos;
}

int registrosFisicosRepuestos() { return totalRegistrosEnArchivo(RUTA_REPUESTOS, sizeof(Repuesto)); }

// 1.- Guardar un registro nuevo
EstadoArchivo guardarRepuesto(Repuesto& x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return ARCH_NO_ABRE;
    time_t ahora = time(nullptr);
    x.id = h.proximoID;
    x.eliminado = false;
    x.fechaCreacion = ahora;
    x.fechaUltimaModificacion = ahora;
    std::fstream f(RUTA_REPUESTOS, std::ios::in | std::ios::out | std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_REPUESTOS); return ARCH_NO_ABRE; }
    f.seekp(posicionRegistro(sizeof(Repuesto), h.cantidadRegistros), std::ios::beg);
    f.write(reinterpret_cast<const char*>(&x), sizeof(Repuesto));
    f.flush();
    if (!f) { registrarFalloArchivo(RUTA_REPUESTOS); return ARCH_ERROR_IO; }
    f.close();
    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!escribirHeader(RUTA_REPUESTOS, h)) return ARCH_ERROR_IO;
    return ARCH_OK;
}

// 2.- Leer por ID
EstadoArchivo leerRepuestoPorID(int id, Repuesto& out, long long* posicion = nullptr) {
    std::ifstream f(RUTA_REPUESTOS, std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_REPUESTOS); return ARCH_NO_ABRE; }
    long long total = totalRegistrosEnStream(f, sizeof(Repuesto));
    if (id < 1 || id > total) return ARCH_ID_INEXISTENTE;
    long long pos = posicionRegistro(sizeof(Repuesto), id - 1);
    f.seekg(pos, std::ios::beg);
    f.read(reinterpret_cast<char*>(&out), sizeof(Repuesto));
    if (!f) { registrarFalloArchivo(RUTA_REPUESTOS); return ARCH_ERROR_IO; }
    if (posicion) *posicion = pos;
    return ARCH_OK;
}

// 3.- Sobrescribir el registro en su posicion original y actualizar fechaUltimaModificacion
EstadoArchivo sobrescribirRepuesto(Repuesto& x) {
    std::fstream f(RUTA_REPUESTOS, std::ios::in | std::ios::out | std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_REPUESTOS); return ARCH_NO_ABRE; }
    long long total = totalRegistrosEnStream(f, sizeof(Repuesto));
    if (x.id < 1 || x.id > total) return ARCH_ID_INEXISTENTE;
    x.fechaUltimaModificacion = time(nullptr);
    f.seekp(posicionRegistro(sizeof(Repuesto), x.id - 1), std::ios::beg);
    f.write(reinterpret_cast<const char*>(&x), sizeof(Repuesto));
    f.flush();
    if (!f) { registrarFalloArchivo(RUTA_REPUESTOS); return ARCH_ERROR_IO; }
    return ARCH_OK;
}

// 4.- Borrado logico
EstadoArchivo marcarRepuestoEliminado(int id) {
    Repuesto x;
    EstadoArchivo e = leerRepuestoPorID(id, x);
    if (e != ARCH_OK) return e;
    if (x.eliminado) return ARCH_YA_ELIMINADO;
    x.eliminado = true;
    e = sobrescribirRepuesto(x);
    if (e != ARCH_OK) return e;
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return ARCH_NO_ABRE;
    h.registrosActivos--;
    if (!escribirHeader(RUTA_REPUESTOS, h)) return ARCH_ERROR_IO;
    return ARCH_OK;
}

// 5.- Contar activos
int contarRepuestosActivos() {
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return -1;
    return h.registrosActivos;
}

// ordenes.bin: OrdenServicio

int registrosFisicosOrdenes() { return totalRegistrosEnArchivo(RUTA_ORDENES, sizeof(OrdenServicio)); }

// Guardar un registro nuevo: ID desde el header, escribe al final, actualiza el header
EstadoArchivo guardarOrden(OrdenServicio& x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return ARCH_NO_ABRE;
    time_t ahora = time(nullptr);
    x.id = h.proximoID;
    x.eliminado = false;
    x.fechaCreacion = ahora;
    x.fechaUltimaModificacion = ahora;
    std::fstream f(RUTA_ORDENES, std::ios::in | std::ios::out | std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_ORDENES); return ARCH_NO_ABRE; }
    f.seekp(posicionRegistro(sizeof(OrdenServicio), h.cantidadRegistros), std::ios::beg);
    f.write(reinterpret_cast<const char*>(&x), sizeof(OrdenServicio));
    f.flush();
    if (!f) { registrarFalloArchivo(RUTA_ORDENES); return ARCH_ERROR_IO; }
    f.close();
    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!escribirHeader(RUTA_ORDENES, h)) return ARCH_ERROR_IO;
    return ARCH_OK;
}

// Leer por ID: posicion calculada (indice = id - 1), sin recorrer el archivo.
//    Entrega el registro este activo o eliminado. 'posicion' (opcional) recibe el byte leido.
EstadoArchivo leerOrdenPorID(int id, OrdenServicio& out, long long* posicion = nullptr) {
    std::ifstream f(RUTA_ORDENES, std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_ORDENES); return ARCH_NO_ABRE; }
    long long total = totalRegistrosEnStream(f, sizeof(OrdenServicio));
    if (id < 1 || id > total) return ARCH_ID_INEXISTENTE;
    long long pos = posicionRegistro(sizeof(OrdenServicio), id - 1);
    f.seekg(pos, std::ios::beg);
    f.read(reinterpret_cast<char*>(&out), sizeof(OrdenServicio));
    if (!f) { registrarFalloArchivo(RUTA_ORDENES); return ARCH_ERROR_IO; }
    if (posicion) *posicion = pos;
    return ARCH_OK;
}

//Sobrescribir el registro en su posicion original y actualizar fechaUltimaModificacion
EstadoArchivo sobrescribirOrden(OrdenServicio& x) {
    std::fstream f(RUTA_ORDENES, std::ios::in | std::ios::out | std::ios::binary);
    if (!f.is_open()) { registrarFalloArchivo(RUTA_ORDENES); return ARCH_NO_ABRE; }
    long long total = totalRegistrosEnStream(f, sizeof(OrdenServicio));
    if (x.id < 1 || x.id > total) return ARCH_ID_INEXISTENTE;
    x.fechaUltimaModificacion = time(nullptr);
    f.seekp(posicionRegistro(sizeof(OrdenServicio), x.id - 1), std::ios::beg);
    f.write(reinterpret_cast<const char*>(&x), sizeof(OrdenServicio));
    f.flush();
    if (!f) { registrarFalloArchivo(RUTA_ORDENES); return ARCH_ERROR_IO; }
    return ARCH_OK;
}

//Borrado logico: eliminado = true y registrosActivos-- en el header
EstadoArchivo marcarOrdenEliminado(int id) {
    OrdenServicio x;
    EstadoArchivo e = leerOrdenPorID(id, x);
    if (e != ARCH_OK) return e;
    if (x.eliminado) return ARCH_YA_ELIMINADO;
    x.eliminado = true;
    e = sobrescribirOrden(x);
    if (e != ARCH_OK) return e;
    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return ARCH_NO_ABRE;
    h.registrosActivos--;
    if (!escribirHeader(RUTA_ORDENES, h)) return ARCH_ERROR_IO;
    return ARCH_OK;
}

// 5) Contar activos: lee solo el header. Retorna -1 si no se pudo leer.
int contarOrdenesActivas() {
    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return -1;
    return h.registrosActivos;
}

// Traduce un EstadoArchivo de persistencia a un Resultado de negocio
static Resultado mapear(EstadoArchivo e, Resultado paraInexistente) {
    switch (e) {
        case ARCH_OK: return RES_OK;
        case ARCH_ID_INEXISTENTE: return paraInexistente;
        default: return RES_ERROR_ARCHIVO;
    }
}

int contarOrdenesVehiculo(int idVehiculo) {
    int total = registrosFisicosOrdenes();
    if (total < 0) return -1;
    int n = 0;
    OrdenServicio o;
    for (int i = 1; i <= total; i++) {
        if (leerOrdenPorID(i, o) != ARCH_OK) return -1;
        if (o.idVehiculo == idVehiculo) n++;
    }
    return n;
}

int contarOrdenesAbiertasMecanico(int idMecanico) {
    int total = registrosFisicosOrdenes();
    if (total < 0) return -1;
    int n = 0;
    OrdenServicio o;
    for (int i = 1; i <= total; i++) {
        if (leerOrdenPorID(i, o) != ARCH_OK) return -1;
        if (o.idMecanico == idMecanico && esEstado(&o, "ABIERTA")) n++;
    }
    return n;
}

// RES_OK si el vehiculo tiene una orden ABIERTA
// RES_ORDEN_NO_EXISTE si no tiene ninguna, RES_ERROR_ARCHIVO si falla la lectura.
Resultado buscarOrdenAbiertaDeVehiculo(int idVehiculo, int* idOrden) {
    int total = registrosFisicosOrdenes();
    if (total < 0) return RES_ERROR_ARCHIVO;
    OrdenServicio o;
    for (int i = 1; i <= total; i++) {
        if (leerOrdenPorID(i, o) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (o.idVehiculo == idVehiculo && esEstado(&o, "ABIERTA")) {
            if (idOrden) *idOrden = o.id;
            return RES_OK;
        }
    }
    return RES_ORDEN_NO_EXISTE;
}

static bool propietarioYModeloValidos(const Vehiculo& v) {
    if (vacio(v.marca) || vacio(v.modelo) || vacio(v.nombrePropietario) || vacio(v.telefonoPropietario)) return false;
    if (v.anio < 1950 || v.anio > 2027) return false;
    if (!cedulaValida(v.cedulaPropietario)) return false;
    return true;
}

// Solo vehiculos ACTIVOS. RES_OK / RES_VEHICULO_NO_EXISTE / RES_ERROR_ARCHIVO
Resultado obtenerVehiculoPorPlaca(const char* placa, Vehiculo& out) {
    int total = registrosFisicosVehiculos();
    if (total < 0) return RES_ERROR_ARCHIVO;
    Vehiculo v;
    for (int i = 1; i <= total; i++) {
        if (leerVehiculoPorID(i, v) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (!v.eliminado && strcmp(v.placa, placa) == 0) { out = v; return RES_OK; }
    }
    return RES_VEHICULO_NO_EXISTE;
}

// RES_OK (activo), RES_VEHICULO_ELIMINADO (existe pero fue eliminado; 'out' igual se llena)
Resultado obtenerVehiculoPorID(int id, Vehiculo& out, long long* pos = nullptr) {
    EstadoArchivo e = leerVehiculoPorID(id, out, pos);
    if (e != ARCH_OK) return mapear(e, RES_VEHICULO_NO_EXISTE);
    if (out.eliminado) return RES_VEHICULO_ELIMINADO;
    return RES_OK;
}

Resultado registrarVehiculo(Vehiculo datos, int* idNuevo) {
    aMayusculas(datos.placa);
    if (!placaValida(datos.placa)) return RES_DATO_INVALIDO;
    if (!propietarioYModeloValidos(datos)) return RES_DATO_INVALIDO;
    if (datos.kilometraje < 0) return RES_DATO_INVALIDO;
    Vehiculo existente;
    Resultado r = obtenerVehiculoPorPlaca(datos.placa, existente);
    if (r == RES_OK) return RES_DUPLICADO;            // placa unica entre los ACTIVOS
    if (r != RES_VEHICULO_NO_EXISTE) return r;        // error de archivo
    if (guardarVehiculo(datos) != ARCH_OK) return RES_ERROR_ARCHIVO;
    if (idNuevo) *idNuevo = datos.id;
    return RES_OK;
}

Resultado buscarVehiculosPorPropietario(const char* sub, Vehiculo* res, int max, int* cantidad, bool* hayMas) {
    *cantidad = 0; *hayMas = false;
    int total = registrosFisicosVehiculos();
    if (total < 0) return RES_ERROR_ARCHIVO;
    Vehiculo v;
    for (int i = 1; i <= total; i++) {
        if (leerVehiculoPorID(i, v) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (v.eliminado || !contieneIgnorandoCase(v.nombrePropietario, sub)) continue;
        if (*cantidad >= max) { *hayMas = true; break; }
        res[(*cantidad)++] = v;
    }
    return RES_OK;
}

Resultado listarVehiculos(Vehiculo* res, int max, int* cantidad, bool* hayMas) {
    *cantidad = 0; *hayMas = false;
    int total = registrosFisicosVehiculos();
    if (total < 0) return RES_ERROR_ARCHIVO;
    Vehiculo v;
    for (int i = 1; i <= total; i++) {
        if (leerVehiculoPorID(i, v) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (v.eliminado) continue;
        if (*cantidad >= max) { *hayMas = true; break; }
        res[(*cantidad)++] = v;
    }
    return RES_OK;
}

Resultado actualizarVehiculo(int id, Vehiculo datos) {
    Vehiculo v;
    Resultado r = obtenerVehiculoPorID(id, v);
    if (r != RES_OK) return r;
    if (!propietarioYModeloValidos(datos)) return RES_DATO_INVALIDO;
    copiar(v.marca, datos.marca, sizeof v.marca);
    copiar(v.modelo, datos.modelo, sizeof v.modelo);
    v.anio = datos.anio;
    copiar(v.nombrePropietario, datos.nombrePropietario, sizeof v.nombrePropietario);
    copiar(v.cedulaPropietario, datos.cedulaPropietario, sizeof v.cedulaPropietario);
    copiar(v.telefonoPropietario, datos.telefonoPropietario, sizeof v.telefonoPropietario);
    if (sobrescribirVehiculo(v) != ARCH_OK) return RES_ERROR_ARCHIVO;
    return RES_OK;
}

Resultado eliminarVehiculo(int id) {
    Vehiculo v;
    Resultado r = obtenerVehiculoPorID(id, v);
    if (r != RES_OK) return r;
    int n = contarOrdenesVehiculo(id);
    if (n < 0) return RES_ERROR_ARCHIVO;
    if (n > 0) return RES_VEHICULO_CON_ORDENES;       // tiene ordenes en cualquier estado
    if (marcarVehiculoEliminado(id) != ARCH_OK) return RES_ERROR_ARCHIVO;
    return RES_OK;
}

// RES_OK (activo), RES_MECANICO_DADO_BAJA (dado de baja; 'out' igual se llena),
// RES_MECANICO_NO_EXISTE o RES_ERROR_ARCHIVO
Resultado obtenerMecanicoPorID(int id, Mecanico& out) {
    EstadoArchivo e = leerMecanicoPorID(id, out);
    if (e != ARCH_OK) return mapear(e, RES_MECANICO_NO_EXISTE);
    if (out.eliminado) return RES_MECANICO_DADO_BAJA;
    return RES_OK;
}

// La cedula es unica contra TODOS los mecanicos:
Resultado registrarMecanico(Mecanico datos, int* idNuevo) {
    aMayusculas(datos.cedula);
    aMayusculas(datos.especialidad);
    if (vacio(datos.nombre) || !cedulaValida(datos.cedula) ||
        !especialidadValida(datos.especialidad) || !fechaValida(datos.fechaIngreso)) return RES_DATO_INVALIDO;
    int total = registrosFisicosMecanicos();
    if (total < 0) return RES_ERROR_ARCHIVO;
    Mecanico m;
    for (int i = 1; i <= total; i++) {
        if (leerMecanicoPorID(i, m) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (strcmp(m.cedula, datos.cedula) == 0) return RES_DUPLICADO;
    }
    if (guardarMecanico(datos) != ARCH_OK) return RES_ERROR_ARCHIVO;
    if (idNuevo) *idNuevo = datos.id;
    return RES_OK;
}

// Activos y dados de baja (el campo 'eliminado' los distingue)
Resultado listarMecanicos(Mecanico* res, int max, int* cantidad, bool* hayMas) {
    *cantidad = 0; *hayMas = false;
    int total = registrosFisicosMecanicos();
    if (total < 0) return RES_ERROR_ARCHIVO;
    Mecanico m;
    for (int i = 1; i <= total; i++) {
        if (leerMecanicoPorID(i, m) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (*cantidad >= max) { *hayMas = true; break; }
        res[(*cantidad)++] = m;
    }
    return RES_OK;
}

Resultado darDeBajaMecanico(int id) {
    Mecanico m;
    Resultado r = obtenerMecanicoPorID(id, m);
    if (r != RES_OK) return r;                         // no existe, ya esta de baja o error
    int abiertas = contarOrdenesAbiertasMecanico(id);
    if (abiertas < 0) return RES_ERROR_ARCHIVO;
    if (abiertas > 0) return RES_MECANICO_CON_ABIERTAS;
    if (marcarMecanicoEliminado(id) != ARCH_OK) return RES_ERROR_ARCHIVO;
    return RES_OK;
}

// REPUESTOS
// RES_OK (activo), RES_REPUESTO_ELIMINADO ('out' igual se llena), RES_REPUESTO_NO_EXISTE, RES_ERROR_ARCHIVO
Resultado obtenerRepuestoPorID(int id, Repuesto& out, long long* pos = nullptr) {
    EstadoArchivo e = leerRepuestoPorID(id, out, pos);
    if (e != ARCH_OK) return mapear(e, RES_REPUESTO_NO_EXISTE);
    if (out.eliminado) return RES_REPUESTO_ELIMINADO;
    return RES_OK;
}

// Solo activos
Resultado obtenerRepuestoPorCodigo(const char* codigo, Repuesto& out) {
    int total = registrosFisicosRepuestos();
    if (total < 0) return RES_ERROR_ARCHIVO;
    Repuesto r;
    for (int i = 1; i <= total; i++) {
        if (leerRepuestoPorID(i, r) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (!r.eliminado && strcmp(r.codigo, codigo) == 0) { out = r; return RES_OK; }
    }
    return RES_REPUESTO_NO_EXISTE;
}

Resultado registrarRepuesto(Repuesto datos, int* idNuevo) {
    aMayusculas(datos.codigo);
    if (vacio(datos.codigo) || vacio(datos.nombre) || vacio(datos.marca) ||
        datos.precio <= 0 || datos.stock < 0 || datos.stockMinimo < 0) return RES_DATO_INVALIDO;
    Repuesto existente;
    Resultado r = obtenerRepuestoPorCodigo(datos.codigo, existente);
    if (r == RES_OK) return RES_DUPLICADO;
    if (r != RES_REPUESTO_NO_EXISTE) return r;
    if (guardarRepuesto(datos) != ARCH_OK) return RES_ERROR_ARCHIVO;
    if (idNuevo) *idNuevo = datos.id;
    return RES_OK;
}

// Codigo o nombre parcial, sin distinguir mayusculas. Solo activos.
Resultado buscarRepuestosPorTexto(const char* sub, Repuesto* res, int max, int* cantidad, bool* hayMas) {
    *cantidad = 0; *hayMas = false;
    int total = registrosFisicosRepuestos();
    if (total < 0) return RES_ERROR_ARCHIVO;
    Repuesto r;
    for (int i = 1; i <= total; i++) {
        if (leerRepuestoPorID(i, r) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (r.eliminado) continue;
        if (!contieneIgnorandoCase(r.codigo, sub) && !contieneIgnorandoCase(r.nombre, sub)) continue;
        if (*cantidad >= max) { *hayMas = true; break; }
        res[(*cantidad)++] = r;
    }
    return RES_OK;
}

Resultado listarRepuestos(Repuesto* res, int max, int* cantidad, bool* hayMas) {
    *cantidad = 0; *hayMas = false;
    int total = registrosFisicosRepuestos();
    if (total < 0) return RES_ERROR_ARCHIVO;
    Repuesto r;
    for (int i = 1; i <= total; i++) {
        if (leerRepuestoPorID(i, r) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (r.eliminado) continue;
        if (*cantidad >= max) { *hayMas = true; break; }
        res[(*cantidad)++] = r;
    }
    return RES_OK;
}

Resultado registrarEntradaMercancia(int idRepuesto, int cantidad) {
    Repuesto r;
    Resultado res = obtenerRepuestoPorID(idRepuesto, r);
    if (res != RES_OK) return res;
    if (cantidad <= 0) return RES_DATO_INVALIDO;
    r.stock += cantidad;
    if (sobrescribirRepuesto(r) != ARCH_OK) return RES_ERROR_ARCHIVO;
    return RES_OK;
}

Resultado cambiarPrecioRepuesto(int idRepuesto, float nuevoPrecio) {
    Repuesto r;
    Resultado res = obtenerRepuestoPorID(idRepuesto, r);
    if (res != RES_OK) return res;
    if (nuevoPrecio <= 0) return RES_DATO_INVALIDO;
    r.precio = nuevoPrecio;   // las ordenes conservan su propio precioUnitario
    if (sobrescribirRepuesto(r) != ARCH_OK) return RES_ERROR_ARCHIVO;
    return RES_OK;
}

// *enUso = true si el repuesto aparece en el detalle de CUALQUIER orden (cualquier estado)
Resultado repuestoEnUso(int idRepuesto, bool* enUso) {
    *enUso = false;
    int total = registrosFisicosOrdenes();
    if (total < 0) return RES_ERROR_ARCHIVO;
    OrdenServicio o;
    for (int i = 1; i <= total; i++) {
        if (leerOrdenPorID(i, o) != ARCH_OK) return RES_ERROR_ARCHIVO;
        for (int j = 0; j < o.numDetalles && j < MAX_DETALLES; j++)
            if (o.detalles[j].idRepuesto == idRepuesto) { *enUso = true; return RES_OK; }
    }
    return RES_OK;
}

Resultado eliminarRepuesto(int idRepuesto) {
    Repuesto r;
    Resultado res = obtenerRepuestoPorID(idRepuesto, r);
    if (res != RES_OK) return res;
    bool enUso;
    res = repuestoEnUso(idRepuesto, &enUso);
    if (res != RES_OK) return res;
    if (enUso) return RES_REPUESTO_EN_USO;
    if (marcarRepuestoEliminado(idRepuesto) != ARCH_OK) return RES_ERROR_ARCHIVO;
    return RES_OK;
}

Resultado obtenerOrdenPorID(int id, OrdenServicio& out, long long* pos = nullptr) {
    return mapear(leerOrdenPorID(id, out, pos), RES_ORDEN_NO_EXISTE);
}

static int indiceDetalle(const OrdenServicio& o, int idRepuesto) {
    for (int i = 0; i < o.numDetalles; i++)
        if (o.detalles[i].idRepuesto == idRepuesto) return i;
    return -1;
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

// Orden de escritura: 1) ordenes.bin (registro nuevo)  2) vehiculos.bin (kilometraje)
Resultado abrirOrden(int idVehiculo, int idMecanico, int kilometraje, const char* descripcionFalla, int* idNueva) {
    Vehiculo v;
    Resultado r = obtenerVehiculoPorID(idVehiculo, v);
    if (r != RES_OK) return r;                         // no existe, eliminado o error de archivo
    Mecanico m;
    r = obtenerMecanicoPorID(idMecanico, m);
    if (r != RES_OK) return r;                         // no existe, dado de baja o error de archivo
    if (kilometraje < 0 || vacio(descripcionFalla) || strlen(descripcionFalla) >= sizeof(((OrdenServicio*)0)->descripcionFalla))
        return RES_DATO_INVALIDO;
    int idAbierta;
    r = buscarOrdenAbiertaDeVehiculo(idVehiculo, &idAbierta);
    if (r == RES_OK) return RES_VEHICULO_CON_ORDEN_ABIERTA;
    if (r != RES_ORDEN_NO_EXISTE) return r;
    int abiertas = contarOrdenesAbiertasMecanico(idMecanico);
    if (abiertas < 0) return RES_ERROR_ARCHIVO;
    if (abiertas >= MAX_ORDENES_ABIERTAS_MECANICO) return RES_MECANICO_SATURADO;
    if (kilometraje < v.kilometraje) return RES_KM_MENOR;

    OrdenServicio nueva;
    memset(&nueva, 0, sizeof nueva);
    nueva.idVehiculo = idVehiculo;
    nueva.idMecanico = idMecanico;
    fechaActual(nueva.fechaApertura);
    nueva.fechaCierre[0] = '\0';
    copiar(nueva.estado, "ABIERTA", sizeof nueva.estado);
    copiar(nueva.descripcionFalla, descripcionFalla, sizeof nueva.descripcionFalla);
    nueva.kilometrajeIngreso = kilometraje;
    nueva.manoObra = 0;
    nueva.numDetalles = 0;
    if (guardarOrden(nueva) != ARCH_OK) return RES_ERROR_ARCHIVO;
    v.kilometraje = kilometraje;
    if (sobrescribirVehiculo(v) != ARCH_OK) return RES_ERROR_ARCHIVO;
    if (idNueva) *idNueva = nueva.id;
    return RES_OK;
}

// Agregar repuesto. Orden de escritura: 1) repuestos.bin (stock)  2) ordenes.bin (linea de detalle)
Resultado agregarRepuestoAOrden(int idOrden, int idRepuesto, int cantidad) {
    OrdenServicio o;
    Resultado r = obtenerOrdenPorID(idOrden, o);
    if (r != RES_OK) return r;
    if (!esEstado(&o, "ABIERTA")) return RES_ESTADO_INVALIDO;
    Repuesto rep;
    r = obtenerRepuestoPorID(idRepuesto, rep);
    if (r != RES_OK) return r;                         // no existe, eliminado o error de archivo
    if (cantidad <= 0) return RES_DATO_INVALIDO;
    if (rep.stock < cantidad) return RES_STOCK_INSUFICIENTE;

    int idx = indiceDetalle(o, idRepuesto);
    if (idx >= 0) {
        o.detalles[idx].cantidad += cantidad;          // acumula en la linea existente
    } else {
        if (o.numDetalles >= MAX_DETALLES) return RES_LIMITE_DETALLES;
        o.detalles[o.numDetalles].idRepuesto = idRepuesto;
        o.detalles[o.numDetalles].cantidad = cantidad;
        o.detalles[o.numDetalles].precioUnitario = rep.precio;   // precio historico congelado
        o.numDetalles++;
    }
    rep.stock -= cantidad;
    if (sobrescribirRepuesto(rep) != ARCH_OK) return RES_ERROR_ARCHIVO;
    if (sobrescribirOrden(o) != ARCH_OK) return RES_ERROR_ARCHIVO;
    return RES_OK;
}

// Quitar repuesto. Orden de escritura: 1) ordenes.bin (se quita la linea)  2) repuestos.bin (stock vuelve)
Resultado quitarRepuestoDeOrden(int idOrden, int idRepuesto) {
    OrdenServicio o;
    Resultado r = obtenerOrdenPorID(idOrden, o);
    if (r != RES_OK) return r;
    if (!esEstado(&o, "ABIERTA")) return RES_ESTADO_INVALIDO;
    int idx = indiceDetalle(o, idRepuesto);
    if (idx < 0) return RES_REPUESTO_NO_EN_ORDEN;
    Repuesto rep;
    r = obtenerRepuestoPorID(idRepuesto, rep);
    if (r == RES_REPUESTO_NO_EXISTE || r == RES_ERROR_ARCHIVO) return r;
    int devueltas = o.detalles[idx].cantidad;
    for (int i = idx; i < o.numDetalles - 1; i++) o.detalles[i] = o.detalles[i + 1];   
    o.numDetalles--;
    memset(&o.detalles[o.numDetalles], 0, sizeof(DetalleOrden));                      // limpia la ranura libre
    if (sobrescribirOrden(o) != ARCH_OK) return RES_ERROR_ARCHIVO;
    rep.stock += devueltas;
    if (sobrescribirRepuesto(rep) != ARCH_OK) return RES_ERROR_ARCHIVO;
    return RES_OK;
}

Resultado cerrarOrden(int idOrden, float manoObra) {
    OrdenServicio o;
    Resultado r = obtenerOrdenPorID(idOrden, o);
    if (r != RES_OK) return r;
    if (!esEstado(&o, "ABIERTA")) return RES_ESTADO_INVALIDO;
    if (manoObra < 0) return RES_DATO_INVALIDO;
    if (o.numDetalles == 0 && manoObra <= 0) return RES_ORDEN_VACIA;
    o.manoObra = manoObra;
    copiar(o.estado, "CERRADA", sizeof o.estado);
    fechaActual(o.fechaCierre);
    if (sobrescribirOrden(o) != ARCH_OK) return RES_ERROR_ARCHIVO;
    return RES_OK;
}

// Anular. Orden de escritura: 1) ordenes.bin (estado ANULADA)  2) repuestos.bin (una escritura por linea)
Resultado anularOrden(int idOrden) {
    OrdenServicio o;
    Resultado r = obtenerOrdenPorID(idOrden, o);
    if (r != RES_OK) return r;
    if (!esEstado(&o, "ABIERTA") && !esEstado(&o, "CERRADA")) return RES_ESTADO_INVALIDO;
    for (int i = 0; i < o.numDetalles; i++) {          // validar que todos los repuestos existan
        Repuesto rep;
        r = obtenerRepuestoPorID(o.detalles[i].idRepuesto, rep);
        if (r == RES_REPUESTO_NO_EXISTE || r == RES_ERROR_ARCHIVO) return r;
    }
    copiar(o.estado, "ANULADA", sizeof o.estado);
    if (sobrescribirOrden(o) != ARCH_OK) return RES_ERROR_ARCHIVO;
    for (int i = 0; i < o.numDetalles; i++) {
        Repuesto rep;
        if (leerRepuestoPorID(o.detalles[i].idRepuesto, rep) != ARCH_OK) return RES_ERROR_ARCHIVO;
        rep.stock += o.detalles[i].cantidad;
        if (sobrescribirRepuesto(rep) != ARCH_OK) return RES_ERROR_ARCHIVO;
    }
    return RES_OK;
}

static Resultado listarOrdenesFiltrado(int modo, const char* estado, int idVehiculo,
                                       OrdenServicio* res, int max, int* cantidad, bool* hayMas) {
    *cantidad = 0; *hayMas = false;
    int total = registrosFisicosOrdenes();
    if (total < 0) return RES_ERROR_ARCHIVO;
    OrdenServicio o;
    for (int i = 1; i <= total; i++) {
        if (leerOrdenPorID(i, o) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (modo == 1 && !esEstado(&o, estado)) continue;
        if (modo == 2 && o.idVehiculo != idVehiculo) continue;
        if (*cantidad >= max) { *hayMas = true; break; }
        res[(*cantidad)++] = o;
    }
    return RES_OK;
}

Resultado listarOrdenes(OrdenServicio* res, int max, int* cantidad, bool* hayMas) {
    return listarOrdenesFiltrado(0, nullptr, 0, res, max, cantidad, hayMas);
}
Resultado listarOrdenesPorEstado(const char* estado, OrdenServicio* res, int max, int* cantidad, bool* hayMas) {
    return listarOrdenesFiltrado(1, estado, 0, res, max, cantidad, hayMas);
}
Resultado listarOrdenesPorVehiculo(int idVehiculo, OrdenServicio* res, int max, int* cantidad, bool* hayMas) {
    return listarOrdenesFiltrado(2, nullptr, idVehiculo, res, max, cantidad, hayMas);
}

// MANTENIMIENTO: verificacion de integridad y respaldo

static void agregarProblema(ReporteIntegridad& rep, const char* fmt, ...) {
    rep.problemas++;
    if (rep.mensajesGuardados >= MAX_PROBLEMAS) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(rep.mensajes[rep.mensajesGuardados], sizeof rep.mensajes[0], fmt, ap);
    va_end(ap);
    rep.mensajesGuardados++;
}

static const char* nombreArchivo(const char* ruta) { return ruta + strlen(DIR_DATOS) + 1; }

// e) el header debe coincidir con el contenido real del archivo
static bool verificarHeader(ReporteIntegridad& rep, const char* ruta, int reales, int activosReales) {
    ArchivoHeader h;
    if (!leerHeader(ruta, h)) return false;
    if (h.cantidadRegistros != reales)
        agregarProblema(rep, "Header de %s indica %d registros; el archivo contiene %d",
                        nombreArchivo(ruta), h.cantidadRegistros, reales);
    if (h.registrosActivos != activosReales)
        agregarProblema(rep, "Header de %s indica %d registros activos; se contaron %d",
                        nombreArchivo(ruta), h.registrosActivos, activosReales);
    return true;
}

Resultado verificarIntegridad(ReporteIntegridad& rep) {
    memset(&rep, 0, sizeof rep);
    int nV = registrosFisicosVehiculos(), nM = registrosFisicosMecanicos();
    int nR = registrosFisicosRepuestos(), nO = registrosFisicosOrdenes();
    if (nV < 0 || nM < 0 || nR < 0 || nO < 0) return RES_ERROR_ARCHIVO;

    int activos = 0;
    Vehiculo v;
    for (int i = 1; i <= nV; i++) {
        if (leerVehiculoPorID(i, v) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (v.id != i) agregarProblema(rep, "vehiculos.bin: el registro en la posicion %d tiene ID %d (se esperaba %d)", i - 1, v.id, i);
        if (!v.eliminado) activos++;
    }
    rep.vehiculos = nV;
    if (!verificarHeader(rep, RUTA_VEHICULOS, nV, activos)) return RES_ERROR_ARCHIVO;

    activos = 0;
    Mecanico m;
    for (int i = 1; i <= nM; i++) {
        if (leerMecanicoPorID(i, m) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (m.id != i) agregarProblema(rep, "mecanicos.bin: el registro en la posicion %d tiene ID %d (se esperaba %d)", i - 1, m.id, i);
        if (!m.eliminado) activos++;
    }
    rep.mecanicos = nM;
    if (!verificarHeader(rep, RUTA_MECANICOS, nM, activos)) return RES_ERROR_ARCHIVO;

    activos = 0;
    Repuesto rp;
    for (int i = 1; i <= nR; i++) {
        if (leerRepuestoPorID(i, rp) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (rp.id != i) agregarProblema(rep, "repuestos.bin: el registro en la posicion %d tiene ID %d (se esperaba %d)", i - 1, rp.id, i);
        if (!rp.eliminado) activos++;
        if (rp.stock < 0)                                                                    // d)
            agregarProblema(rep, "Repuesto %s (%s) tiene stock negativo: %d", rp.codigo, rp.nombre, rp.stock);
    }
    rep.repuestos = nR;
    if (!verificarHeader(rep, RUTA_REPUESTOS, nR, activos)) return RES_ERROR_ARCHIVO;

    activos = 0;
    OrdenServicio o;
    for (int i = 1; i <= nO; i++) {
        if (leerOrdenPorID(i, o) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (o.id != i) agregarProblema(rep, "ordenes.bin: el registro en la posicion %d tiene ID %d (se esperaba %d)", i - 1, o.id, i);
        if (!o.eliminado) activos++;
        bool abierta = esEstado(&o, "ABIERTA");

        EstadoArchivo e = leerVehiculoPorID(o.idVehiculo, v);                                // a) y b)
        if (e == ARCH_ID_INEXISTENTE)
            agregarProblema(rep, "Orden #%d apunta al vehiculo ID %d (no existe en vehiculos.bin)", o.id, o.idVehiculo);
        else if (e != ARCH_OK) return RES_ERROR_ARCHIVO;
        else if (abierta && v.eliminado)
            agregarProblema(rep, "Orden #%d ABIERTA apunta al vehiculo %s que esta eliminado", o.id, v.placa);

        e = leerMecanicoPorID(o.idMecanico, m);
        if (e == ARCH_ID_INEXISTENTE)
            agregarProblema(rep, "Orden #%d apunta al mecanico ID %d (no existe en mecanicos.bin)", o.id, o.idMecanico);
        else if (e != ARCH_OK) return RES_ERROR_ARCHIVO;
        else if (abierta && m.eliminado)      // en ordenes CERRADAS o ANULADAS un mecanico de baja es valido
            agregarProblema(rep, "Orden #%d ABIERTA apunta al mecanico %s que esta dado de baja", o.id, m.nombre);

        if (o.numDetalles < 0 || o.numDetalles > MAX_DETALLES) {
            agregarProblema(rep, "Orden #%d tiene un numero de lineas invalido: %d", o.id, o.numDetalles);
            continue;
        }
        for (int j = 0; j < o.numDetalles; j++) {                                            // c)
            e = leerRepuestoPorID(o.detalles[j].idRepuesto, rp);
            if (e == ARCH_ID_INEXISTENTE)
                agregarProblema(rep, "Orden #%d, linea %d: el repuesto ID %d no existe en repuestos.bin",
                                o.id, j + 1, o.detalles[j].idRepuesto);
            else if (e != ARCH_OK) return RES_ERROR_ARCHIVO;
        }
    }
    rep.ordenes = nO;
    if (!verificarHeader(rep, RUTA_ORDENES, nO, activos)) return RES_ERROR_ARCHIVO;
    return RES_OK;
}

Resultado crearBackup(char* rutaOut, int tam) {
    time_t t = time(nullptr);
    struct tm* l = localtime(&t);
    char carpeta[64];
    strftime(carpeta, sizeof carpeta, "backup_%Y-%m-%d_%H-%M", l);
    if (!crearCarpeta(carpeta)) return RES_ERROR_ARCHIVO;
    const char* archivos[] = {RUTA_TALLER, RUTA_VEHICULOS, RUTA_MECANICOS, RUTA_REPUESTOS, RUTA_ORDENES};
    for (int i = 0; i < 5; i++) {
        char destino[200];
        snprintf(destino, sizeof destino, "%s/%s", carpeta, nombreArchivo(archivos[i]));
        if (!copiarArchivoBinario(archivos[i], destino)) return RES_ERROR_ARCHIVO;
    }
    copiar(rutaOut, carpeta, tam);
    return RES_OK;
}

// Reporte 1: datos del vehiculo, sus ordenes y total facturado en ordenes cerradas.
Resultado reporteHistorialVehiculo(const char* placa, Vehiculo& veh, OrdenServicio* ordenes, int max,
                                   int* cantidad, bool* hayMas, float* totalFacturado) {
    *cantidad = 0; *hayMas = false; *totalFacturado = 0;
    Resultado r = obtenerVehiculoPorPlaca(placa, veh);
    if (r != RES_OK) return r;
    int total = registrosFisicosOrdenes();
    if (total < 0) return RES_ERROR_ARCHIVO;
    OrdenServicio o;
    for (int i = 1; i <= total; i++) {
        if (leerOrdenPorID(i, o) != ARCH_OK) return RES_ERROR_ARCHIVO;
        if (o.idVehiculo != veh.id) continue;
        if (esEstado(&o, "CERRADA")) *totalFacturado += calcularTotalOrden(&o);
        if (*cantidad < max) ordenes[(*cantidad)++] = o; else *hayMas = true;
    }
    return RES_OK;
}

// Reporte 2: los TOP_REPUESTOS repuestos con mas unidades en ordenes cerradas.
Resultado reporteRepuestosMasUsados(FilaUsoRepuesto* filas, int* cantidad) {
    *cantidad = 0;
    int nRep = registrosFisicosRepuestos();
    int nOrd = registrosFisicosOrdenes();
    if (nRep < 0 || nOrd < 0) return RES_ERROR_ARCHIVO;
    int* usos = new int[nRep + 1]();
    OrdenServicio o;
    for (int i = 1; i <= nOrd; i++) {
        if (leerOrdenPorID(i, o) != ARCH_OK) { delete[] usos; return RES_ERROR_ARCHIVO; }
        if (!esEstado(&o, "CERRADA")) continue;
        for (int j = 0; j < o.numDetalles && j < MAX_DETALLES; j++) {
            int id = o.detalles[j].idRepuesto;
            if (id >= 1 && id <= nRep) usos[id] += o.detalles[j].cantidad;
        }
    }
    for (int k = 0; k < TOP_REPUESTOS; k++) {
        int mejor = 0;
        for (int id = 1; id <= nRep; id++) if (usos[id] > 0 && (mejor == 0 || usos[id] > usos[mejor])) mejor = id;
        if (mejor == 0) break;
        filas[*cantidad].idRepuesto = mejor;
        filas[*cantidad].unidades = usos[mejor];
        (*cantidad)++;
        usos[mejor] = 0;                               // ya fue elegido
    }
    delete[] usos;
    return RES_OK;
}

// Reporte 3: por cada mecanico (activos y de baja), ordenes cerradas, mano de obra y repuestos.
Resultado reporteFacturacionMecanicos(FilaFacturacion* filas, int max, int* cantidad, bool* hayMas) {
    *cantidad = 0; *hayMas = false;
    int nMec = registrosFisicosMecanicos();
    int nOrd = registrosFisicosOrdenes();
    if (nMec < 0 || nOrd < 0) return RES_ERROR_ARCHIVO;
    for (int i = 1; i <= nMec; i++) {
        if (*cantidad >= max) { *hayMas = true; break; }
        FilaFacturacion f;
        memset(&f, 0, sizeof f);
        if (leerMecanicoPorID(i, f.mecanico) != ARCH_OK) return RES_ERROR_ARCHIVO;
        OrdenServicio o;
        for (int j = 1; j <= nOrd; j++) {
            if (leerOrdenPorID(j, o) != ARCH_OK) return RES_ERROR_ARCHIVO;
            if (o.idMecanico != f.mecanico.id || !esEstado(&o, "CERRADA")) continue;
            f.ordenesCerradas++;
            f.manoObra += o.manoObra;
            f.repuestos += calcularSubtotalRepuestos(&o);
        }
        filas[(*cantidad)++] = f;
    }
    return RES_OK;
}

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
        cout << "Operacion cancelada. No se realizo ningun cambio.\n";
        return false;
    }
    return true;
}

static bool leerTexto(const char* prompt, char* dest, int max) {
    string t;
    while (true) {
        if (!leerLinea(prompt, t)) return false;
        if (t.empty()) { cout << "ERROR: Este campo no puede estar vacio.\n"; continue; }
        if ((int)t.size() >= max) {
            cout << "ERROR: Texto demasiado largo (maximo " << max - 1 << " caracteres).\n";
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
            cout << "ERROR: '" << t << "' no es un numero entero válido.\n";
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
            cout << "ERROR: '" << t << "' no es un número valido.\n";
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
            cout << "ERROR: Cedula invalida. Use solo digitos (se admite prefijo V-, E- o J-).\n";
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
            cout << "ERROR: Placa invalida. Debe ser alfanumerica de 6 o 7 caracteres.\n";
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
            cout << "ERROR: Fecha invalida. Use el formato YYYY-MM-DD con valores reales.\n";
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
            cout << "ERROR: Especialidad invalida. Valores permitidos: MOTOR, FRENOS, ELECTRICIDAD, SUSPENSION, GENERAL.\n";
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
        if (t.empty()) { cout << "ERROR: Este campo no puede estar vacio.\n"; continue; }
        if ((int)t.size() >= max) { cout << "ERROR: Codigo demasiado largo.\n"; continue; }
        strcpy(dest, t.c_str());
        return true;
    }
}

static bool leerOpcion(int* op, int maxOp) {
    return leerEntero("Seleccione una opcion: ", op, 0, maxOp);
}

static int largoUtf8(const char* s) {
    int n = 0;
    for (; *s; ++s) if (((unsigned char)*s & 0xC0) != 0x80) n++;
    return n;
}

// Imprime s ocupando exactamente 'ancho' caracteres.
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

// convierte un time_t guardado en disco a texto legible AAAA-MM-DD HH:MM
static void fmtFechaHora(char* buf, size_t n, time_t t) {
    struct tm* l = localtime(&t);
    if (l) strftime(buf, n, "%Y-%m-%d %H:%M", l);
    else snprintf(buf, n, "-");
}

static void avisarSiHayMas(bool hayMas) {
    if (hayMas) cout << "AVISO: hay mas resultados; se muestran solo los primeros " << MAX_RESULTADOS
                     << ". Refine la busqueda para ver el resto.\n";
}

// Cargan una COPIA del registro para poder mostrar su nombre.
static bool cargarVehiculo(int id, Vehiculo& v) {
    Resultado r = obtenerVehiculoPorID(id, v);
    return r == RES_OK || r == RES_VEHICULO_ELIMINADO;
}
static bool cargarMecanico(int id, Mecanico& m) {
    Resultado r = obtenerMecanicoPorID(id, m);
    return r == RES_OK || r == RES_MECANICO_DADO_BAJA;
}
static bool cargarRepuesto(int id, Repuesto& rp) {
    Resultado r = obtenerRepuestoPorID(id, rp);
    return r == RES_OK || r == RES_REPUESTO_ELIMINADO;
}

// Traduce un Resultado de la logica a un mensaje. Los datos de contexto.
static void mostrarError(Resultado r, int idOrden, int idVeh, int idMec, int idRep, int cantidad, int km) {
    Vehiculo v;      bool hayV = idVeh > 0 && cargarVehiculo(idVeh, v);
    Mecanico m;      bool hayM = idMec > 0 && cargarMecanico(idMec, m);
    Repuesto rp;     bool hayR = idRep > 0 && cargarRepuesto(idRep, rp);
    OrdenServicio o; bool hayO = idOrden > 0 && obtenerOrdenPorID(idOrden, o) == RES_OK;
    switch (r) {
        case RES_OK: break;
        case RES_ERROR_ARCHIVO:
            cout << "ERROR: No se pudo leer o escribir el archivo '" << ultimoArchivoConError() << "'. La operacion no continua.\n";
            break;
        case RES_VEHICULO_NO_EXISTE: cout << "ERROR: El vehiculo indicado no existe.\n"; break;
        case RES_VEHICULO_ELIMINADO:
            cout << "ERROR: El vehiculo '" << (hayV ? v.placa : "?") << "' fue eliminado y no esta disponible.\n"; break;
        case RES_MECANICO_NO_EXISTE: cout << "ERROR: El mecanico indicado no existe.\n"; break;
        case RES_MECANICO_DADO_BAJA:
            cout << "ERROR: El mecanico '" << (hayM ? m.nombre : "?") << "' esta dado de baja.\n"; break;
        case RES_REPUESTO_NO_EXISTE: cout << "ERROR: El repuesto indicado no existe.\n"; break;
        case RES_REPUESTO_ELIMINADO:
            cout << "ERROR: El repuesto '" << (hayR ? rp.nombre : "?") << "' fue eliminado y no esta disponible.\n"; break;
        case RES_ORDEN_NO_EXISTE: cout << "ERROR: La orden #" << idOrden << " no existe.\n"; break;
        case RES_DUPLICADO: cout << "ERROR: Ya existe un registro con esa placa, codigo o cedula.\n"; break;
        case RES_DATO_INVALIDO: cout << "ERROR: Alguno de los datos ingresados es invalido.\n"; break;
        case RES_VEHICULO_CON_ORDEN_ABIERTA: {
            int idAb = 0;
            if (idVeh > 0) buscarOrdenAbiertaDeVehiculo(idVeh, &idAb);
            cout << "ERROR: El vehiculo '" << (hayV ? v.placa : "?") << "' ya tiene la orden #" << idAb << " ABIERTA.\n";
            break;
        }
        case RES_VEHICULO_CON_ORDENES: {
            int n = idVeh > 0 ? contarOrdenesVehiculo(idVeh) : 0;
            cout << "ERROR: El vehiculo '" << (hayV ? v.placa : "?") << "' tiene " << n
                 << (n == 1 ? " orden registrada" : " ordenes registradas") << " y no puede eliminarse.\n";
            break;
        }
        case RES_MECANICO_SATURADO:
            cout << "ERROR: El mecanico '" << (hayM ? m.nombre : "?") << "' ya tiene "
                 << (hayM ? contarOrdenesAbiertasMecanico(m.id) : MAX_ORDENES_ABIERTAS_MECANICO) << " ordenes abiertas.\n";
            break;
        case RES_MECANICO_CON_ABIERTAS:
            cout << "ERROR: El mecanico '" << (hayM ? m.nombre : "?") << "' tiene "
                 << (hayM ? contarOrdenesAbiertasMecanico(m.id) : 0) << " orden(es) ABIERTA(S) y no puede darse de baja.\n";
            break;
        case RES_KM_MENOR:
            cout << "ERROR: El kilometraje " << km << " es menor al ultimo registrado del vehiculo '"
                 << (hayV ? v.placa : "?") << "' (" << (hayV ? v.kilometraje : 0) << " km).\n";
            break;
        case RES_STOCK_INSUFICIENTE:
            cout << "ERROR: Stock insuficiente de '" << (hayR ? rp.nombre : "?") << "'. Disponible: "
                 << (hayR ? rp.stock : 0) << ", solicitado: " << cantidad << ".\n";
            break;
        case RES_ESTADO_INVALIDO:
            cout << "ERROR: La orden #" << idOrden << " esta " << (hayO ? o.estado : "?")
                 << " y no puede modificarse.\n";
            break;
        case RES_REPUESTO_NO_EN_ORDEN:
            cout << "ERROR: El repuesto '" << (hayR ? rp.nombre : "?") << "' no esta en la orden #" << idOrden << ".\n";
            break;
        case RES_ORDEN_VACIA:
            cout << "ERROR: La orden #" << idOrden << " no tiene repuestos ni mano de obra; no puede cerrarse.\n";
            break;
        case RES_REPUESTO_EN_USO:
            cout << "ERROR: El repuesto '" << (hayR ? rp.nombre : "?")
                 << "' aparece en el detalle de ordenes y no puede eliminarse.\n";
            break;
        case RES_LIMITE_DETALLES:
            cout << "ERROR: La orden #" << idOrden << " ya alcanzo el limite de " << MAX_DETALLES
                 << " lineas de detalle; el repuesto no se agrego.\n";
            break;
    }
}


// ---------------- VEHICULOS ----------------
void mostrarVehiculo(const Vehiculo& v) {
    char fecha[24];
    fmtFechaHora(fecha, sizeof fecha, v.fechaCreacion);
    cout << "\nVehiculo: " << v.placa << " - " << v.marca << " " << v.modelo << " " << v.anio << "\n";
    cout << "  Kilometraje : " << v.kilometraje << " km\n";
    cout << "  Propietario : " << v.nombrePropietario << " (Cedula: " << v.cedulaPropietario << ")\n";
    cout << "  Telefono    : " << v.telefonoPropietario << "\n";
    cout << "  Registrado  : " << fecha << " (ID " << v.id << ")\n";
}

void mostrarListaVehiculos(const Vehiculo* lista, int cantidad) {
    const int w[] = {6, 11, 24, 7, 10, 22};
    cout << "\n";
    lineaTabla("╔", "╦", "╗", w, 6);
    cout << "|"; celda("ID", w[0]); cout << "|"; celda("Placa", w[1]); cout << "|"; celda("Vehiculo", w[2]);
    cout << "|"; celda("Ano", w[3]); cout << "|"; celda("Km", w[4]); cout << "|"; celda("Propietario", w[5]); cout << "|\n";
    lineaTabla("+", "-", "+", w, 6);
    for (int i = 0; i < cantidad; i++) {
        const Vehiculo& v = lista[i];
        char b0[16], nom[120], b3[16], b4[16];
        snprintf(b0, sizeof b0, "%d", v.id);
        snprintf(nom, sizeof nom, "%s %s", v.marca, v.modelo);
        snprintf(b3, sizeof b3, "%d", v.anio);
        snprintf(b4, sizeof b4, "%d", v.kilometraje);
        cout << "|"; celda(b0, w[0]); cout << "|"; celda(v.placa, w[1]); cout << "|"; celda(nom, w[2]);
        cout << "|"; celda(b3, w[3]); cout << "|"; celda(b4, w[4], true); cout << "|"; celda(v.nombrePropietario, w[5]); cout << "|\n";
    }
    lineaTabla("+", "-", "+", w, 6);
    cout << "Total de vehiculos mostrados: " << cantidad << "\n";
}

void menuRegistrarVehiculo() {
    cout << "\n--- REGISTRAR VEHICULO (escriba CANCELAR en cualquier momento para abortar) ---\n";
    Vehiculo d;
    memset(&d, 0, sizeof d);
    if (!leerPlaca("Placa: ", d.placa, sizeof d.placa)) return;
    Vehiculo existente;
    if (obtenerVehiculoPorPlaca(d.placa, existente) == RES_OK) {
        cout << "ERROR: La placa '" << d.placa << "' ya esta registrada.\n";
        return;
    }
    if (!leerTexto("Marca: ", d.marca, sizeof d.marca)) return;
    if (!leerTexto("Modelo: ", d.modelo, sizeof d.modelo)) return;
    if (!leerEntero("Año (1950-2027): ", &d.anio, 1950, 2027)) return;
    if (!leerEntero("Kilometraje: ", &d.kilometraje, 0, INT_MAX)) return;
    if (!leerTexto("Nombre del propietario: ", d.nombrePropietario, sizeof d.nombrePropietario)) return;
    if (!leerCedula("Cedula del propietario: ", d.cedulaPropietario, sizeof d.cedulaPropietario)) return;
    if (!leerTexto("Telefono del propietario: ", d.telefonoPropietario, sizeof d.telefonoPropietario)) return;
    bool ok;
    if (!leerSN("Confirmar el registro (S/N): ", &ok)) return;
    if (!ok) { cout << "Registro descartado.\n"; return; }
    int idNuevo = 0;
    Resultado res = registrarVehiculo(d, &idNuevo);
    if (res == RES_OK) cout << "Vehiculo '" << d.placa << "' (" << d.marca << " " << d.modelo << ") registrado correctamente (ID " << idNuevo << ").\n";
    else mostrarError(res, 0, 0, 0, 0, 0, 0);
}

void menuBuscarVehiculo() {
    cout << "\nBuscar por: 1. Placa   2. Propietario   3. ID\n";
    int modo;
    if (!leerEntero("Seleccione: ", &modo, 1, 3)) return;
    if (modo == 1) {
        char placa[10];
        if (!leerPlaca("Placa exacta: ", placa, sizeof placa)) return;
        Vehiculo v;
        Resultado r = obtenerVehiculoPorPlaca(placa, v);
        if (r == RES_OK) mostrarVehiculo(v);
        else if (r == RES_VEHICULO_NO_EXISTE) cout << "ERROR: No existe un vehiculo con la placa '" << placa << "'.\n";
        else mostrarError(r, 0, 0, 0, 0, 0, 0);
    } else if (modo == 2) {
        char sub[100];
        if (!leerTexto("Nombre del propietario (parcial): ", sub, sizeof sub)) return;
        Vehiculo lista[MAX_RESULTADOS];
        int n;
        bool hayMas;
        Resultado r = buscarVehiculosPorPropietario(sub, lista, MAX_RESULTADOS, &n, &hayMas);
        if (r != RES_OK) { mostrarError(r, 0, 0, 0, 0, 0, 0); return; }
        if (n == 0) cout << "No hay vehiculos cuyo propietario contenga '" << sub << "'.\n";
        else { mostrarListaVehiculos(lista, n); avisarSiHayMas(hayMas); }
    } else {
        int id;
        if (!leerEntero("ID del vehiculo: ", &id, 1, INT_MAX)) return;
        Vehiculo v;
        long long pos = -1;
        Resultado r = obtenerVehiculoPorID(id, v, &pos);
        if (r == RES_OK || r == RES_VEHICULO_ELIMINADO) {
            // mensaje de depuracion: prueba del acceso directo por calculo de posicion
            cout << "[DEPURACION] Vehiculo ID " << id << " leido en la posicion " << pos << " bytes (header "
                 << sizeof(ArchivoHeader) << " + indice " << (id - 1) << " x " << sizeof(Vehiculo) << " bytes)\n";
            if (r == RES_OK) mostrarVehiculo(v);
            else cout << "El vehiculo con ID " << id << " (placa " << v.placa << ") fue ELIMINADO y no esta disponible.\n";
        }
        else if (r == RES_VEHICULO_NO_EXISTE) cout << "ERROR: No existe un vehiculo con ID " << id << ".\n";
        else mostrarError(r, 0, 0, 0, 0, 0, 0);
    }
}

void menuActualizarVehiculo() {
    cout << "\n--- ACTUALIZAR VEHICULO (escriba CANCELAR para abortar) ---\n";
    char placa[10];
    if (!leerPlaca("Placa del vehiculo a actualizar: ", placa, sizeof placa)) return;
    Vehiculo v;
    Resultado r = obtenerVehiculoPorPlaca(placa, v);
    if (r == RES_VEHICULO_NO_EXISTE) { cout << "ERROR: No existe un vehiculo con la placa '" << placa << "'.\n"; return; }
    if (r != RES_OK) { mostrarError(r, 0, 0, 0, 0, 0, 0); return; }
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
    r = actualizarVehiculo(v.id, d);
    if (r == RES_OK) cout << "Vehiculo '" << placa << "' actualizado correctamente.\n";
    else mostrarError(r, 0, v.id, 0, 0, 0, 0);
}

void menuEliminarVehiculo() {
    cout << "\n--- ELIMINAR VEHICULO (escriba CANCELAR para abortar) ---\n";
    char placa[10];
    if (!leerPlaca("Placa del vehiculo a eliminar: ", placa, sizeof placa)) return;
    Vehiculo v;
    Resultado r = obtenerVehiculoPorPlaca(placa, v);
    if (r == RES_VEHICULO_NO_EXISTE) { cout << "ERROR: No existe un vehiculo con la placa '" << placa << "'.\n"; return; }
    if (r != RES_OK) { mostrarError(r, 0, 0, 0, 0, 0, 0); return; }
    int n = contarOrdenesVehiculo(v.id);
    if (n < 0) { mostrarError(RES_ERROR_ARCHIVO, 0, 0, 0, 0, 0, 0); return; }
    if (n > 0) { mostrarError(RES_VEHICULO_CON_ORDENES, 0, v.id, 0, 0, 0, 0); return; }
    bool ok;
    if (!leerSN("Confirmar la eliminacion (S/N): ", &ok)) return;
    if (!ok) { cout << "Eliminacion descartada.\n"; return; }
    r = eliminarVehiculo(v.id);
    if (r == RES_OK) cout << "Vehiculo '" << placa << "' eliminado (borrado logico: el registro se conserva en el archivo).\n";
    else mostrarError(r, 0, v.id, 0, 0, 0, 0);
}

void menuListarVehiculos() {
    Vehiculo lista[MAX_RESULTADOS];
    int n;
    bool hayMas;
    Resultado r = listarVehiculos(lista, MAX_RESULTADOS, &n, &hayMas);
    if (r != RES_OK) { mostrarError(r, 0, 0, 0, 0, 0, 0); return; }
    if (n == 0) cout << "No hay vehiculos registrados.\n";
    else { mostrarListaVehiculos(lista, n); avisarSiHayMas(hayMas); }
}

void menuVehiculos() {
    const char* items[] = {"1. Registrar vehiculo", "2. Buscar vehiculo", "3. Actualizar vehiculo",
                           "4. Eliminar vehiculo", "5. Listar vehiculos", "0. Volver al menu principal"};
    while (!gEOF) {
        cout << "\n";
        caja("GESTION DE VEHICULOS", nullptr, items, 6);
        int op;
        if (!leerOpcion(&op, 5)) continue;
        switch (op) {
            case 1: menuRegistrarVehiculo(); break;
            case 2: menuBuscarVehiculo(); break;
            case 3: menuActualizarVehiculo(); break;
            case 4: menuEliminarVehiculo(); break;
            case 5: menuListarVehiculos(); break;
            case 0: return;
        }
    }
}
        
void menuEntradaMercancia(SistemaTaller* s) {
    cout << "\n--- ENTRADA DE MERCANCÍA (escriba CANCELAR para abortar) ---\n";
    char cod[20];
    if (!leerCodigo("Codigo del repuesto: ", cod, sizeof cod)) return;
    Repuesto* r = buscarRepuestoPorCodigo(s, cod);
    if (!r) { cout << "ERROR: No existe un repuesto con el codigo '" << cod << "'.\n"; return; }
    cout << "Repuesto: " << r->nombre << " - stock actual: " << r->stock << "\n";
    int cant;
    if (!leerEntero("Cantidad que ingresa (> 0): ", &cant, 1, INT_MAX)) return;
    bool ok;
    if (!leerSN("Confirmar la entrada? (S/N): ", &ok)) return;
    if (!ok) { cout << "Entrada descartada.\n"; return; }
    int antes = r->stock;
    Resultado res = registrarEntradaMercancia(s, r->id, cant);
    if (res == RES_OK) cout << "Entrada registrada. Stock de '" << r->nombre << "': " << antes << " -> " << r->stock << ".\n";
    else mostrarError(res, s, 0, 0, 0, r->id, cant, 0);
}

void menuCambiarPrecio(SistemaTaller* s) {
    cout << "\n--- CAMBIAR PRECIO (escriba CANCELAR para abortar) ---\n";
    char cod[20];
    if (!leerCodigo("Codigo del repuesto: ", cod, sizeof cod)) return;
    Repuesto* r = buscarRepuestoPorCodigo(s, cod);
    if (!r) { cout << "ERROR: No existe un repuesto con el codigo '" << cod << "'.\n"; return; }
    cout << fixed;
    cout.precision(2);
    cout << "Repuesto: " << r->nombre << " - precio actual: " << r->precio << "\n";
    float nuevo;
    if (!leerFloat("Nuevo precio (> 0): ", &nuevo, 0, true)) return;
    bool ok;
    if (!leerSN("Confirmar el cambio de precio? (S/N): ", &ok)) return;
    if (!ok) { cout << "Cambio descartado.\n"; return; }
    float antes = r->precio;
    Resultado res = cambiarPrecioRepuesto(s, r->id, nuevo);
    if (res == RES_OK) cout << "Precio de '" << r->nombre << "': " << antes << " -> " << r->precio
                            << " (las ordenes ya registradas no cambian).\n";
    else mostrarError(res, s, 0, 0, 0, r->id, 0, 0);
}

void menuEliminarRepuesto(SistemaTaller* s) {
    cout << "\n--- ELIMINAR REPUESTO (escriba CANCELAR para abortar) ---\n";
    char cod[20];
    if (!leerCodigo("Código del repuesto: ", cod, sizeof cod)) return;
    Repuesto* r = buscarRepuestoPorCodigo(s, cod);
    if (!r) { cout << "ERROR: No existe un repuesto con el codigo '" << cod << "'.\n"; return; }
    if (repuestoEnUso(s, r->id)) { mostrarError(RES_REPUESTO_EN_USO, s, 0, 0, 0, r->id, 0, 0); return; }
    bool ok;
    if (!leerSN("Confirmar la eliminacion? (S/N): ", &ok)) return;
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
    const char* items[] = {"1. Registrar repuesto", "2. Buscar repuesto", "3. Registrar entrada de mercancia",
                           "4. Cambiar precio", "5. Eliminar repuesto", "6. Listar repuestos",
                           "0. Volver al menu principal"};
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

void mostrarOrden(OrdenServicio* o, SistemaTaller* s) {
    const int W1 = 10, W2 = 25, W3 = 6, W4 = 10, W5 = 12;
    const int INNER = W1 + W2 + W3 + W4 + W5 + 4;   // ancho interior de la caja
    const int LABEL = W1 + W2 + W3 + W4 + 3;        // ancho de las etiquetas de totales
    char linea[400], tmp[200];
    Vehiculo* v = buscarVehiculoPorID(s, o->idVehiculo);
    Mecanico* m = buscarMecanicoPorID(s, o->idMecanico);

    auto fila = [&](const char* txt) {
        cout << "| "; imprimirPad(txt, INNER - 2, false); cout << " |\n";
    };

    cout << "\n+"; rep("-", INNER); cout << "+\n";
    snprintf(tmp, sizeof tmp, "ORDEN DE SERVICIO #%d", o->id);
    {
        int izq = (INNER - largoUtf8(tmp)) / 2;
        cout << "|" << string(izq, ' '); imprimirPad(tmp, INNER - izq, false); cout << "|\n";
    }
    cout << "+"; rep("-", INNER); cout << "+\n";
    snprintf(linea, sizeof linea, "Estado   : %-10s Apertura: %s", o->estado, o->fechaApertura);
    fila(linea);
    snprintf(linea, sizeof linea, "Cierre   : %s", o->fechaCierre[0] ? o->fechaCierre : "-");
    fila(linea);
    if (v) snprintf(linea, sizeof linea, "Vehiculo : %s - %s %s %d (%d km)", v->placa, v->marca, v->modelo, v->anio, v->kilometraje);
    else snprintf(linea, sizeof linea, "Vehiculo : (no disponible)");
    fila(linea);
    snprintf(linea, sizeof linea, "Dueno    : %s", v ? v->nombrePropietario : "-");
    fila(linea);
    if (m) snprintf(linea, sizeof linea, "Mecanico : %s (%s)", m->nombre, m->especialidad);
    else snprintf(linea, sizeof linea, "Mecanico : (no disponible)");
    fila(linea);
    snprintf(linea, sizeof linea, "Km ingreso: %d", o->kilometrajeIngreso);
    fila(linea);
    snprintf(linea, sizeof linea, "Falla    : %s", o->descripcionFalla);
    fila(linea);

    const int w[] = {W1, W2, W3, W4, W5};
    lineaTabla("+", "+", "+", w, 5);
    cout << "|"; celda("Codigo", W1); cout << "|"; celda("Repuesto", W2); cout << "|"; celda("Cant", W3);
    cout << "|"; celda("P.Unit", W4); cout << "|"; celda("Total", W5); cout << "|\n";
    lineaTabla("+", "+", "+", w, 5);
    for (int i = 0; i < o->numDetalles; i++) {
        DetalleOrden* d = &o->detalles[i];
        Repuesto* r = buscarRepuestoPorID(s, d->idRepuesto);
        char b3[16], b4[24], b5[24];
        snprintf(b3, sizeof b3, "%d", d->cantidad);
        fmtMoneda(b4, sizeof b4, d->precioUnitario);
        fmtMoneda(b5, sizeof b5, d->cantidad * d->precioUnitario);
        cout << "|"; celda(r ? r->codigo : "?", W1); cout << "|"; celda(r ? r->nombre : "?", W2);
        cout << "|"; celda(b3, W3, true); cout << "|"; celda(b4, W4, true); cout << "|"; celda(b5, W5, true); cout << "|\n";
    }
    cout << "+"; rep("-", W1); cout << "+"; rep("-", W2); cout << "+"; rep("-", W3); cout << "+";
    rep("-", W4); cout << "+"; rep("-", W5); cout << "+\n";
    char val[24];
    fmtMoneda(val, sizeof val, calcularSubtotalRepuestos(o));
    cout << "|"; celda("Repuestos", LABEL); cout << "|"; celda(val, W5, true); cout << "|\n";
    fmtMoneda(val, sizeof val, o->manoObra);
    cout << "|"; celda("Mano de obra", LABEL); cout << "|"; celda(val, W5, true); cout << "|\n";
    fmtMoneda(val, sizeof val, calcularTotalOrden(o));
    cout << "|"; celda("TOTAL", LABEL); cout << "|"; celda(val, W5, true); cout << "|\n";
    cout << "+"; rep("-", LABEL); cout << "+"; rep("-", W5); cout << "+\n";
    cout << "Lineas de detalle: " << o->numDetalles << " (capacidad del arreglo de detalles: "
         << o->capacidadDetalles << ")\n";
}

void mostrarListaOrdenes(OrdenServicio** lista, int cantidad, SistemaTaller* s) {
    const int w[] = {7, 11, 30, 20, 12, 12};
    cout << "\n";
    lineaTabla("+", "+", "+", w, 6);
    cout << "|"; celda("Orden", w[0]); cout << "|"; celda("Estado", w[1]); cout << "|"; celda("Vehículo", w[2]);
    cout << "|"; celda("Mecanico", w[3]); cout << "|"; celda("Apertura", w[4]); cout << "|"; celda("Total", w[5]); cout << "|\n";
    lineaTabla("+", "+", "+", w, 6);
    for (int i = 0; i < cantidad; i++) {
        OrdenServicio* o = lista[i];
        Vehiculo* v = buscarVehiculoPorID(s, o->idVehiculo);
        Mecanico* m = buscarMecanicoPorID(s, o->idMecanico);
        char b0[16], veh[160], tot[24];
        snprintf(b0, sizeof b0, "#%d", o->id);
        if (v) snprintf(veh, sizeof veh, "%s - %s %s", v->placa, v->marca, v->modelo);
        else snprintf(veh, sizeof veh, "-");
        fmtMoneda(tot, sizeof tot, calcularTotalOrden(o));
        cout << "|"; celda(b0, w[0]); cout << "|"; celda(o->estado, w[1]); cout << "|"; celda(veh, w[2]);
        cout << "|"; celda(m ? m->nombre : "-", w[3]); cout << "|"; celda(o->fechaApertura, w[4]);
        cout << "|"; celda(tot, w[5], true); cout << "|\n";
    }
    lineaTabla("+", "+", "+", w, 6);
    cout << "Total de ordenes: " << cantidad << "\n";
}

void menuAbrirOrden(SistemaTaller* s) {
    cout << "\n--- ABRIR ORDEN DE SERVICIO (escriba CANCELAR para abortar) ---\n";
    int nm;
    Mecanico** lm = listarMecanicos(s, &nm);
    if (nm == 0) { cout << "ERROR: No hay mecanicos registrados.\n"; delete[] lm; return; }
    mostrarListaMecanicos(lm, nm, s);
    delete[] lm;
    char placa[10];
    if (!leerPlaca("Placa del vehiculo: ", placa, sizeof placa)) return;
    Vehiculo* v = buscarVehiculoPorPlaca(s, placa);
    if (!v) { cout << "ERROR: No existe un vehiculo con la placa '" << placa << "'.\n"; return; }
    int idMec;
    if (!leerEntero("ID del mecanico responsable (segun la lista): ", &idMec, 1, INT_MAX)) return;
    Mecanico* m = buscarMecanicoPorID(s, idMec);
    if (!m) { cout << "ERROR: No existe un mecanico con ID " << idMec << ".\n"; return; }
    int km;
    if (!leerEntero("Kilometraje de ingreso: ", &km, 0, INT_MAX)) return;
    char falla[200];
    if (!leerTexto("Falla reportada: ", falla, sizeof falla)) return;
    bool ok;
    if (!leerSN("Confirmar la apertura de la orden (S/N): ", &ok)) return;
    if (!ok) { cout << "Apertura descartada.\n"; return; }
    int idVeh = v->id, idM = m->id;
    Resultado res;
    OrdenServicio* o = abrirOrden(s, idVeh, idM, km, falla, &res);
    if (o) cout << "Orden #" << o->id << " abierta para el vehiculo '" << placa << "' (mecanico: " << m->nombre << ").\n";
    else mostrarError(res, s, 0, idVeh, idM, 0, 0, km);
}

void menuAgregarRepuestoOrden(SistemaTaller* s) {
    cout << "\n--- AGREGAR REPUESTO A ORDEN (escriba CANCELAR para abortar) ---\n";
    int idOrden;
    if (!leerEntero("Numero de orden: ", &idOrden, 1, INT_MAX)) return;
    char cod[20];
    if (!leerCodigo("Codigo del repuesto: ", cod, sizeof cod)) return;
    Repuesto* r = buscarRepuestoPorCodigo(s, cod);
    if (!r) { cout << "ERROR: No existe un repuesto con el codigo '" << cod << "'.\n"; return; }
    int cant;
    if (!leerEntero("Cantidad (> 0): ", &cant, 1, INT_MAX)) return;
    int idRep = r->id;
    Resultado res = agregarRepuestoAOrden(s, idOrden, idRep, cant);
    if (res == RES_OK) cout << "Se agregaron " << cant << " unidad(es) de '" << r->nombre << "' a la orden #" << idOrden
                            << ". Stock restante: " << r->stock << ".\n";
    else mostrarError(res, s, idOrden, 0, 0, idRep, cant, 0);
}

void menuQuitarRepuestoOrden(SistemaTaller* s) {
    cout << "\n--- QUITAR REPUESTO DE ORDEN (escriba CANCELAR para abortar) ---\n";
    int idOrden;
    if (!leerEntero("Numero de orden: ", &idOrden, 1, INT_MAX)) return;
    char cod[20];
    if (!leerCodigo("Codigo del repuesto: ", cod, sizeof cod)) return;
    Repuesto* r = buscarRepuestoPorCodigo(s, cod);
    if (!r) { cout << "ERROR: No existe un repuesto con el codigo '" << cod << "'.\n"; return; }
    int idRep = r->id;
    Resultado res = quitarRepuestoDeOrden(s, idOrden, idRep);
    if (res == RES_OK) cout << "Se quito '" << r->nombre << "' de la orden #" << idOrden
                            << ". Stock actual: " << r->stock << ".\n";
    else mostrarError(res, s, idOrden, 0, 0, idRep, 0, 0);
}

void menuCerrarOrden(SistemaTaller* s) {
    cout << "\n--- CERRAR ORDEN (escriba CANCELAR para abortar) ---\n";
    int idOrden;
    if (!leerEntero("Número de orden: ", &idOrden, 1, INT_MAX)) return;
    OrdenServicio* o = buscarOrdenPorID(s, idOrden);
    if (!o) { mostrarError(RES_ORDEN_NO_EXISTE, s, idOrden, 0, 0, 0, 0, 0); return; }
    float mano;
    if (!leerFloat("Mano de obra (>= 0): ", &mano, 0, false)) return;
    bool ok;
    if (!leerSN("Confirmar el cierre de la orden? (S/N): ", &ok)) return;
    if (!ok) { cout << "Cierre descartado.\n"; return; }
    Resultado res = cerrarOrden(s, idOrden, mano);
    if (res == RES_OK) {
        cout << fixed;
        cout.precision(2);
        cout << "Orden #" << idOrden << " cerrada. Total: " << calcularTotalOrden(o) << ".\n";
    } else mostrarError(res, s, idOrden, 0, 0, 0, 0, 0);
}

void menuAnularOrden(SistemaTaller* s) {
    cout << "\n--- ANULAR ORDEN (escriba CANCELAR para abortar) ---\n";
    int idOrden;
    if (!leerEntero("Número de orden: ", &idOrden, 1, INT_MAX)) return;
    OrdenServicio* o = buscarOrdenPorID(s, idOrden);
    if (!o) { mostrarError(RES_ORDEN_NO_EXISTE, s, idOrden, 0, 0, 0, 0, 0); return; }
    bool ok;
    if (!leerSN("Confirmar la anulación? Se devolverá el stock al inventario (S/N): ", &ok)) return;
    if (!ok) { cout << "Anulación descartada.\n"; return; }
    Resultado res = anularOrden(s, idOrden);
    if (res == RES_OK) cout << "Orden #" << idOrden << " ANULADA. El inventario fue restituido.\n";
    else mostrarError(res, s, idOrden, 0, 0, 0, 0, 0);
}

void menuVerDetalleOrden(SistemaTaller* s) {
    int idOrden;
    if (!leerEntero("Número de orden: ", &idOrden, 1, INT_MAX)) return;
    OrdenServicio* o = buscarOrdenPorID(s, idOrden);
    if (o) mostrarOrden(o, s);
    else mostrarError(RES_ORDEN_NO_EXISTE, s, idOrden, 0, 0, 0, 0, 0);
}

void menuListarOrdenes(SistemaTaller* s) {
    cout << "\nListar: 1. Todas   2. Por estado   3. Historial de un vehiculo\n";
    int modo;
    if (!leerEntero("Seleccione: ", &modo, 1, 3)) return;
    int n = 0;
    OrdenServicio** lista = nullptr;
    if (modo == 1) {
        lista = listarOrdenes(s, &n);
    } else if (modo == 2) {
        string t;
        while (true) {
            if (!leerLinea("Estado (ABIERTA/CERRADA/ANULADA): ", t)) return;
            mayusculas(t);
            if (t == "ABIERTA" || t == "CERRADA" || t == "ANULADA") break;
            cout << "ERROR: Estado inválido.\n";
        }
        lista = listarOrdenesPorEstado(s, t.c_str(), &n);
    } else {
        char placa[10];
        if (!leerPlaca("Placa del vehículo: ", placa, sizeof placa)) return;
        Vehiculo* v = buscarVehiculoPorPlaca(s, placa);
        if (!v) { cout << "ERROR: No existe un vehículo con la placa '" << placa << "'.\n"; return; }
        lista = listarOrdenesPorVehiculo(s, v->id, &n);
    }
    if (n == 0) cout << "No hay órdenes que mostrar.\n";
    else mostrarListaOrdenes(lista, n, s);
    delete[] lista;
}

void menuOrdenes(SistemaTaller* s) {
    const char* items[] = {"1. Abrir orden", "2. Agregar repuesto a orden", "3. Quitar repuesto de orden",
                           "4. Cerrar orden", "5. Anular orden", "6. Ver detalle de orden",
                           "7. Listar órdenes", "0. Volver al menú principal"};
    while (!gEOF) {
        cout << "\n";
        caja("ORDENES DE SERVICIO", nullptr, items, 8);
        int op;
        if (!leerOpcion(&op, 7)) continue;
        switch (op) {
            case 1: menuAbrirOrden(s); break;
            case 2: menuAgregarRepuestoOrden(s); break;
            case 3: menuQuitarRepuestoOrden(s); break;
            case 4: menuCerrarOrden(s); break;
            case 5: menuAnularOrden(s); break;
            case 6: menuVerDetalleOrden(s); break;
            case 7: menuListarOrdenes(s); break;
            case 0: return;
        }
    }
}

//menu principal
void menuPrincipal(SistemaTaller* s) {
    const char* items[] = {"1. Gestion de Vehiculos", "2. Gestion de Mecanicos", "3. Inventario de Repuestos",
                           "4. Ordenes de Servicio", "0. Salir"};
    char sub[150];
    snprintf(sub, sizeof sub, "Taller: %s", s->taller.nombre);
    while (!gEOF) {
        cout << "\n";
        caja("SISTEMA DE GESTION DE TALLER", sub, items, 5);
        int op;
        if (!leerOpcion(&op, 4)) continue;
        switch (op) {
            case 1: menuVehiculos(s); break;
            case 2: menuMecanicos(s); break;
            case 3: menuInventario(s); break;
            case 4: menuOrdenes(s); break;
            case 0: {
                bool ok;
                if (!leerSN("Seguro que desea salir? Los datos en memoria se perderan (S/N): ", &ok)) break;
                if (ok) { cout << "Hasta luego.\n"; return; }
                break;
            }
        }
    }
}

int main() {
    Taller datos;
    memset(&datos, 0, sizeof datos);
    strcpy(datos.nombre, "Autoservicio El Lago");
    strcpy(datos.rif, "J-12345678-9");
    strcpy(datos.direccion, "Av. 5 de Julio, Maracaibo, Zulia");
    strcpy(datos.telefono, "0261-7000000");

    SistemaTaller sistema;
    inicializarSistema(&sistema, datos);
    menuPrincipal(&sistema);
    liberarSistema(&sistema);
    return 0;
}

