# Proyecto 2 - Programación 2 - Taller mecánico con persistencia y acceso aleatorio

**Integrantes:** Leonardo Malaguera (C.I. 32882803) · Maria Garcia (C.I. 33264902)

Evolución del Proyecto 1: los datos ya no viven en arreglos dinámicos en la RAM, sino en archivos binarios dentro de `datos/`. El programa nunca carga un archivo completo: para cada acción abre el archivo, calcula la posición en bytes del registro, se desplaza con `seekg`/`seekp`, lee o escribe **un solo registro** y cierra.

## 1. Compilación y ejecución

Todo el código está en un único archivo, `proyecto.cpp` (capas separadas por secciones: persistencia → lógica → presentación). Requiere un compilador con C++17 (`<filesystem>`).

```
g++ -std=c++17 proyecto.cpp -o proyecto
./proyecto            (Linux / macOS / Git Bash)
proyecto.exe          (Windows CMD / PowerShell)
```

* Se ejecuta **desde la carpeta del proyecto**: usa la ruta relativa `datos/`.
* En el primer arranque (sin `datos/taller.bin`) pide los datos del taller; en los siguientes solo los lee.
* En Visual Studio: configurar el estándar del lenguaje en C++17 (Propiedades → C/C++ → Lenguaje).
* Con MinGW antiguo (GCC 8) puede hacer falta añadir `-lstdc++fs`.

**Pruebas:** `sh pruebas/correr_pruebas.sh` ejecuta los 8 casos de uso y regenera `pruebas/salida_*.txt` (ver sección 9).

## 2. Tamaño de las estructuras y fórmula de posición

Medidos con `g++` 13 en Linux x86-64 (`sizeof`). En otro compilador o a 32 bits pueden variar (por el alineamiento de `time_t`); para verificarlos basta imprimir `sizeof(...)`.

| Estructura | `sizeof` (bytes) |
|---|---|
| `ArchivoHeader` | **16** |
| `Taller` | **360** |
| `Vehiculo` | **288** |
| `Mecanico` | **176** |
| `Repuesto` | **208** |
| `DetalleOrden` | **12** |
| `OrdenServicio` (con 20 líneas de detalle) | **520** |

**Fórmula de posición** del registro con índice físico `i` (0, 1, 2…):

```
posicion = sizeof(ArchivoHeader) + i * sizeof(Estructura)
```

Ejemplo real (Caso de Uso 2): el vehículo ID 5 está en el índice 4 → `16 + 4 * 288 = 1168`. El programa lo muestra con un mensaje `[DEPURACION]`.

`taller.bin` es la excepción: un único registro `Taller` (360 bytes), sin header.

Tamaños de los archivos pre-poblados (comprobación de la fórmula): `vehiculos.bin` = 16 + 8×288 = 2320 · `mecanicos.bin` = 16 + 4×176 = 720 · `repuestos.bin` = 16 + 12×208 = 2512 · `ordenes.bin` = 16 + 11×520 = 5736 · `taller.bin` = 360.

## 3. Relación entre ID e índice físico

**Relación:** `indice = id - 1`. Los IDs se asignan en orden de inserción desde 1 (`proximoID` del header), los registros se agregan siempre al final y nunca se mueven ni se borran físicamente. Por eso el registro con ID *n* es siempre el *n*-ésimo del archivo y se ubica con `posicion = 16 + (id - 1) * sizeof(Estructura)`, sin recorrer nada. Una lectura solo es válida si `1 <= id <= registros que caben en el archivo` (se calcula con el tamaño del archivo en bytes).

**Qué cambio la dejaría de hacer válida:**
* Borrado **físico** con compactación (los registros posteriores cambiarían de posición).
* **Ordenar** el archivo o insertar un registro en medio.
* Reutilizar IDs de registros eliminados, o asignar IDs que no sean consecutivos (por ejemplo, por pareja o por rango).
* Unir o restaurar archivos de distintos orígenes (una restauración parcial deja IDs que ya no coinciden con el contenido; la verificación de integridad también revisa que el registro de la posición *k* tenga ID *k+1*).

## 4. Diseño del detalle de la orden (sección 3 del enunciado)

### 4.1 Estructuras

```cpp
const int MAX_DETALLES = 20;

struct DetalleOrden {        // sizeof = 12
    int idRepuesto;
    int cantidad;
    float precioUnitario;    // precio histórico
};

struct OrdenServicio {       // sizeof = 520
    ... campos del Proyecto 2 ...
    DetalleOrden detalles[MAX_DETALLES];   // arreglo FIJO dentro del registro
    int numDetalles;                       // líneas en uso (0..MAX_DETALLES)
    bool eliminado; time_t fechaCreacion; time_t fechaUltimaModificacion;
};
```

**Decisión:** las líneas de detalle viven **dentro del registro de su orden**, en un arreglo de tamaño fijo con un contador. No hay archivo adicional.

### 4.2 Cuántos bytes ocupa una orden en disco

| Caso | Bytes en disco | Bytes de detalle en uso | Bytes de detalle desperdiciados |
|---|---|---|---|
| Orden con 1 línea | **520** | 12 | 228 de 240 (95 %) |
| Orden con 15 líneas | **520** | 180 | 60 de 240 (25 %) |
| Orden con 20 líneas (límite) | **520** | 240 | 0 |

Toda orden ocupa 520 bytes sin importar cuántas líneas tenga: esa es justamente la condición que mantiene posible el acceso aleatorio por cálculo. El costo es el espacio sin usar en órdenes con pocas líneas.

### 4.3 Cumplimiento de los requisitos

1. **Conserva las líneas entre ejecuciones** con repuesto, cantidad y precio histórico: se escriben junto con la orden (Caso de Uso 3).
2. **Al menos 15 líneas:** el límite es 20 (Caso de Uso 4 con 15 líneas).
3. **Todos los registros del archivo miden lo mismo:** 520 bytes.
4. **Consultar las líneas de una orden no carga otras órdenes:** se lee un solo registro de 520 bytes.
5. **Reglas del Proyecto 1:** acumular si el repuesto ya está en la orden, quitar la línea completa devolviendo el stock y conservar el precio histórico.

Si se alcanza el límite, `agregarRepuestoAOrden` retorna `RES_LIMITE_DETALLES` y no escribe nada.

### 4.4 Alternativa que consideramos y descartamos

**Archivo adicional `detalles.bin`** con un registro por línea (`idOrden`, `idRepuesto`, `cantidad`, `precioUnitario`, `eliminado`) y la orden guardando el índice de su primera línea y el de la siguiente en cada línea (lista enlazada en disco).

Ventajas: sin límite fijo de líneas y sin espacio desperdiciado (estimado, no implementado: una orden de 1 línea ocuparía unos 330 bytes y una de 15 líneas unos 1000, contando ~280 bytes de orden + ~48 por línea).

Por qué la descartamos:
* **Más puntos de falla:** agregar un repuesto pasaría de 2 escrituras (stock + orden) a 3 o 4 (stock, línea nueva, header de `detalles.bin`, orden/enlace). Cada escritura adicional es un lugar donde una interrupción deja datos inconsistentes.
* Consultar una orden obligaría a seguir la cadena de enlaces (varias lecturas con saltos) en lugar de una sola lectura.
* Habría que incluir otro archivo en el backup, en la verificación de integridad y en la regla "no eliminar un repuesto usado".
* Con 20 líneas fijas el desperdicio es de pocos cientos de bytes por orden, un costo razonable para un taller.

### 4.5 Cómo se resuelve "quitar una línea"

1. Se lee la orden (un registro) y se busca la línea del repuesto; si no está → `RES_REPUESTO_NO_EN_ORDEN`.
2. Se **compacta** el arreglo: cada línea posterior se corre una posición a la izquierda, `numDetalles--` y se limpia (con ceros) la ranura que quedó libre.
3. Se sobrescribe la orden en su posición y **después** se devuelven las unidades al `stock` del repuesto en `repuestos.bin`.

## 5. Firmas diseñadas

**Convenciones comunes a todas las capas**

* Ninguna función retorna un puntero a un registro. El registro leído se entrega como **copia por referencia** (parámetro de salida) y el valor de retorno informa el resultado.
* **Persistencia** retorna `EstadoArchivo`: `ARCH_OK`, `ARCH_NO_ABRE` (no se pudo abrir el archivo), `ARCH_ID_INEXISTENTE`, `ARCH_YA_ELIMINADO`, `ARCH_ERROR_IO`. El nombre del archivo que falló queda disponible en `ultimoArchivoConError()`, para que la presentación muestre un mensaje claro (la persistencia no usa `cout`).
* **Lógica** retorna `Resultado` (`RES_OK` o la causa específica del fallo). Si hay varias causas posibles, cada una tiene su propio código; así la presentación distingue "no existe" de "fue eliminado", "dado de baja", "stock insuficiente", etc.
* Las búsquedas y listados con varios resultados reciben un arreglo del llamador, su capacidad máxima y entregan `cantidad` y `hayMas` (el máximo es la constante `MAX_RESULTADOS = 50`); la presentación avisa cuando hay más.
* Las funciones que solo cuentan retornan `-1` si el archivo no se pudo leer.

### 5.1 Capa de persistencia

```cpp
const char* ultimoArchivoConError();
int totalRegistrosEnArchivo(const char* ruta, size_t tamRegistro);
bool leerHeader(const char* ruta, ArchivoHeader& h);
bool escribirHeader(const char* ruta, const ArchivoHeader& h);
bool inicializarArchivo(const char* ruta);
bool crearCarpeta(const char* ruta);
bool existeArchivo(const char* ruta);
bool copiarArchivoBinario(const char* origen, const char* destino);
bool existeArchivoTaller();
bool leerTaller(Taller& t);
bool guardarTaller(Taller& t);
bool inicializarArchivosSistema();
int registrosFisicosVehiculos();
EstadoArchivo guardarVehiculo(Vehiculo& x);
EstadoArchivo leerVehiculoPorID(int id, Vehiculo& out, long long* posicion = nullptr);
EstadoArchivo sobrescribirVehiculo(Vehiculo& x);
EstadoArchivo marcarVehiculoEliminado(int id);
int contarVehiculosActivos();
int registrosFisicosMecanicos();
EstadoArchivo guardarMecanico(Mecanico& x);
EstadoArchivo leerMecanicoPorID(int id, Mecanico& out, long long* posicion = nullptr);
EstadoArchivo sobrescribirMecanico(Mecanico& x);
EstadoArchivo marcarMecanicoEliminado(int id);
int contarMecanicosActivos();
int registrosFisicosRepuestos();
EstadoArchivo guardarRepuesto(Repuesto& x);
EstadoArchivo leerRepuestoPorID(int id, Repuesto& out, long long* posicion = nullptr);
EstadoArchivo sobrescribirRepuesto(Repuesto& x);
EstadoArchivo marcarRepuestoEliminado(int id);
int contarRepuestosActivos();
int registrosFisicosOrdenes();
EstadoArchivo guardarOrden(OrdenServicio& x);
EstadoArchivo leerOrdenPorID(int id, OrdenServicio& out, long long* posicion = nullptr);
EstadoArchivo sobrescribirOrden(OrdenServicio& x);
EstadoArchivo marcarOrdenEliminado(int id);
int contarOrdenesActivas();
```

**Justificación:**
* `leerHeader`/`escribirHeader`/`inicializarArchivo` retornan `bool` porque solo hay dos desenlaces útiles; la causa (qué archivo) queda en `ultimoArchivoConError()`.
* `guardarX(X&)` recibe el registro **por referencia**: le asigna `id`, `eliminado=false` y las marcas de tiempo, de modo que quien llama conoce el ID asignado leyendo `x.id`.
* `leerXPorID(id, out, &pos)` entrega el registro esté activo o eliminado (la persistencia no decide qué es válido). Distingue `ARCH_ID_INEXISTENTE` de `ARCH_NO_ABRE`/`ARCH_ERROR_IO`. El tercer parámetro opcional devuelve la posición en bytes leída (para el mensaje de depuración del Caso de Uso 2).
* `marcarXEliminado` retorna `ARCH_YA_ELIMINADO` si ya estaba eliminado, para no restar dos veces en `registrosActivos`.
* `contarXActivos` solo lee el header (no recorre el archivo).
* Las cinco operaciones están escritas por entidad (vehículos, mecánicos, repuestos y órdenes), como pide el enunciado; sabemos que son casi idénticas y que el Proyecto 3 resolverá esa repetición.

### 5.2 Capa de lógica

```cpp
int contarOrdenesVehiculo(int idVehiculo);
int contarOrdenesAbiertasMecanico(int idMecanico);
Resultado buscarOrdenAbiertaDeVehiculo(int idVehiculo, int* idOrden);
Resultado obtenerVehiculoPorPlaca(const char* placa, Vehiculo& out);
Resultado obtenerVehiculoPorID(int id, Vehiculo& out, long long* pos = nullptr);
Resultado registrarVehiculo(Vehiculo datos, int* idNuevo);
Resultado buscarVehiculosPorPropietario(const char* sub, Vehiculo* res, int max, int* cantidad, bool* hayMas);
Resultado listarVehiculos(Vehiculo* res, int max, int* cantidad, bool* hayMas);
Resultado actualizarVehiculo(int id, Vehiculo datos);
Resultado eliminarVehiculo(int id);
Resultado obtenerMecanicoPorID(int id, Mecanico& out);
Resultado registrarMecanico(Mecanico datos, int* idNuevo);
Resultado listarMecanicos(Mecanico* res, int max, int* cantidad, bool* hayMas);
Resultado darDeBajaMecanico(int id);
Resultado obtenerRepuestoPorID(int id, Repuesto& out, long long* pos = nullptr);
Resultado obtenerRepuestoPorCodigo(const char* codigo, Repuesto& out);
Resultado registrarRepuesto(Repuesto datos, int* idNuevo);
Resultado buscarRepuestosPorTexto(const char* sub, Repuesto* res, int max, int* cantidad, bool* hayMas);
Resultado listarRepuestos(Repuesto* res, int max, int* cantidad, bool* hayMas);
Resultado registrarEntradaMercancia(int idRepuesto, int cantidad);
Resultado cambiarPrecioRepuesto(int idRepuesto, float nuevoPrecio);
Resultado repuestoEnUso(int idRepuesto, bool* enUso);
Resultado eliminarRepuesto(int idRepuesto);
Resultado obtenerOrdenPorID(int id, OrdenServicio& out, long long* pos = nullptr);
float calcularSubtotalRepuestos(const OrdenServicio* o);
float calcularTotalOrden(const OrdenServicio* o);
Resultado abrirOrden(int idVehiculo, int idMecanico, int kilometraje, const char* descripcionFalla, int* idNueva);
Resultado agregarRepuestoAOrden(int idOrden, int idRepuesto, int cantidad);
Resultado quitarRepuestoDeOrden(int idOrden, int idRepuesto);
Resultado cerrarOrden(int idOrden, float manoObra);
Resultado anularOrden(int idOrden);
Resultado listarOrdenes(OrdenServicio* res, int max, int* cantidad, bool* hayMas);
Resultado listarOrdenesPorEstado(const char* estado, OrdenServicio* res, int max, int* cantidad, bool* hayMas);
Resultado listarOrdenesPorVehiculo(int idVehiculo, OrdenServicio* res, int max, int* cantidad, bool* hayMas);
Resultado verificarIntegridad(ReporteIntegridad& rep);
Resultado crearBackup(char* rutaOut, int tam);
Resultado reporteRepuestosMasUsados(FilaUsoRepuesto* filas, int* cantidad);
Resultado reporteFacturacionMecanicos(FilaFacturacion* filas, int max, int* cantidad, bool* hayMas);
```

**Justificación de cómo informan éxito, fallo y causa:**
* `Resultado` único para toda la lógica: `RES_OK` = éxito; cada regla de negocio violada tiene su código (`RES_DUPLICADO`, `RES_VEHICULO_CON_ORDENES`, `RES_MECANICO_CON_ABIERTAS`, `RES_STOCK_INSUFICIENTE`, `RES_ESTADO_INVALIDO`, `RES_LIMITE_DETALLES`…); `RES_ERROR_ARCHIVO` agrupa los fallos de disco.
* `obtenerXPorID` retorna `RES_OK` (activo), `RES_X_ELIMINADO`/`RES_MECANICO_DADO_BAJA` (existe pero fue dado de baja; **la copia igualmente se llena**, para poder mostrar el nombre del mecánico en órdenes cerradas) o `RES_X_NO_EXISTE`.
* Los `registrarX` entregan el ID asignado por un parámetro de salida `int* idNuevo`.
* `buscarOrdenAbiertaDeVehiculo` usa `RES_OK` = encontró (ID en `*idOrden`) y `RES_ORDEN_NO_EXISTE` = no tiene ninguna.
* Las funciones de reporte e integridad no imprimen: llenan estructuras (`ReporteIntegridad`, `FilaUsoRepuesto`, `FilaFacturacion`) que la presentación dibuja.

## 6. Operaciones compuestas: orden de escritura

Todas las validaciones se hacen **antes** de la primera escritura. Después, el orden elegido es:

| Operación | Orden de escritura | Si se interrumpe entre ambas… |
|---|---|---|
| Abrir orden | 1) `ordenes.bin` (registro nuevo) → 2) `vehiculos.bin` (kilometraje) | Queda la orden abierta y el vehículo con el kilometraje anterior (dato menor, corregible; la orden guarda el km de ingreso). |
| Agregar repuesto | 1) `repuestos.bin` (stock −) → 2) `ordenes.bin` (línea) | El stock queda **menor** al real: unidades "perdidas", no vendidas dos veces. |
| Quitar repuesto | 1) `ordenes.bin` (quitar línea) → 2) `repuestos.bin` (stock +) | Las unidades no vuelven al inventario: otra vez stock **menor** al real. |
| Cerrar orden | solo `ordenes.bin` | Una sola escritura: no hay estado intermedio. |
| Anular orden | 1) `ordenes.bin` (estado ANULADA) → 2) `repuestos.bin` (stock + por cada línea) | Algunas unidades no vuelven al inventario (stock menor); la orden ya no se puede anular de nuevo. |

**Criterio:** en cualquier interrupción el inventario debe quedar **por debajo** del real y nunca por encima. Y al **crear** un registro se escribe primero el registro y después el header; si se corta entre ambos, el header queda desactualizado y la verificación de integridad (e) lo detecta.

## 7. Preguntas de análisis

### 7.1 Escribimos con `write` una `OrdenServicio` del Proyecto 1 (con `DetalleOrden*`) y la leímos en otra ejecución. ¿Qué observamos?

Hicimos el experimento (`pruebas/experimento_puntero/experimento.cpp`, dos ejecuciones separadas). Resultado real:

```
[escribir] sizeof(OrdenServicioP1) = 272 bytes
[escribir] direccion guardada en 'detalles' = 0x55a801e612b0
[escribir] detalles[0].idRepuesto = 1 (aqui si funciona)
--- (otra ejecucion) ---
[leer] id = 1, estado = ABIERTA, numDetalles = 3 (los datos simples si volvieron)
[leer] direccion leida en 'detalles' = 0x55a801e612b0
[leer] intentando leer detalles[0].idRepuesto ...
Segmentation fault   (codigo de salida 139)
```

Los campos simples (`id`, `estado`, `numDetalles`) volvieron bien, pero al acceder a `detalles[0]` el programa se cerró con **fallo de segmentación**. Ocurre porque `write` copia los bytes de la estructura tal cual, y en el caso de `detalles` esos bytes son **la dirección de memoria** (un número) del arreglo creado con `new`, no el contenido del arreglo. Cuando el programa termina, el sistema operativo libera esa memoria; en la nueva ejecución la dirección guardada apunta a un lugar que no pertenece al proceso (o a memoria cualquiera), así que leerla es inválido. Los datos del detalle nunca llegaron al archivo. Por eso en disco hace falta una estructura de tamaño fijo, sin punteros, con los datos dentro.

### 7.2 Al agregar un repuesto se escriben el detalle y el stock. Si se interrumpe entre ambas, ¿qué inconsistencia queda y qué orden conviene?

* **Si se escribe primero la orden y después el stock:** queda la línea en la orden pero el stock no descontado. El repuesto está **facturado y a la vez disponible**: se pueden vender unidades que ya se usaron, el stock queda *mayor* que el real y podría terminar negativo cuando el inventario físico se agote. Es difícil de detectar a tiempo (la verificación de integridad solo vería el problema cuando el stock ya sea negativo).
* **Si se escribe primero el stock y después la orden (nuestra elección):** el stock queda descontado sin línea en la orden: unas unidades "desaparecen" del inventario, pero el cliente no es facturado por algo que no se anotó y no se puede vender dos veces lo mismo. El daño es **conservador y fácil de corregir** con una entrada de mercancía por esa cantidad.

Elegimos el segundo orden por la regla general que aplicamos a todo el sistema: ante una interrupción, el inventario debe quedar por debajo del real, nunca por encima. Para quitar y anular usamos el orden inverso (primero la orden o el estado, luego el stock) por la misma razón; en la anulación, además, cambiar primero el estado evita que se repita y devuelva el stock dos veces.

### 7.3 Si en el Proyecto 1 se hubiera permitido la eliminación física de un mecánico con órdenes, ¿qué habría pasado con el historial?

Las órdenes guardan solo `idMecanico`. Si el mecánico se elimina físicamente, esas órdenes quedarían apuntando a un ID que ya no existe: no se podría mostrar quién hizo el trabajo, el reporte de facturación por mecánico perdería esas filas, y (en archivos) los registros posteriores se correrían de posición, rompiendo la relación `indice = id - 1` y apuntando cada ID de orden a otro mecánico. La **baja lógica** lo resuelve porque el registro permanece en su lugar y solo cambia `eliminado = true`: no se asigna a órdenes nuevas, pero las órdenes cerradas y anuladas siguen mostrando su nombre (marcado como BAJA) y los IDs siguen siendo válidos.

## 8. Verificación de integridad y respaldo

* **`verificarIntegridad`** (Mantenimiento → 1) recorre los cuatro archivos registro por registro y reporta, con datos que permiten ubicar el problema: (a) órdenes con vehículo o mecánico inexistente, (b) órdenes ABIERTAS con vehículo/mecánico no activo (en cerradas o anuladas un mecánico de baja es válido), (c) líneas de detalle con repuesto inexistente, (d) repuestos con stock negativo, (e) headers que no coinciden con el contenido real. Agregamos además: registro cuyo ID no coincide con `posición + 1` y número de líneas fuera de rango.
* **`crearBackup`** (Mantenimiento → 2) crea la carpeta `backup_AAAA-MM-DD_HH-MM` y copia byte a byte (bloques de 4096) los cinco `.bin`.

## 9. Datos pre-poblados y pruebas

La carpeta `datos/` incluye: 8 vehículos, 4 mecánicos (Luis Fuenmayor dado de baja), 12 repuestos (RAD-011 con BAJO STOCK) y 11 órdenes: 7 CERRADAS, 2 ANULADAS (una anulada desde ABIERTA y otra desde CERRADA) y 2 ABIERTAS; las órdenes #1, #2 y #6 tienen 4 o más líneas. Se generaron con el propio programa (`pruebas/entrada_semilla.txt`).

La carpeta `pruebas/` contiene `entrada_*.txt` y `salida_*.txt` de los 8 casos de uso. Los casos con reinicio usan archivos numerados por ejecución (`3a`, `3b`, `3c`…):

| Caso | Entradas | Qué se verifica (resultado obtenido) |
|---|---|---|
| 1 Persistencia | `1a`, `1b` | Tras reiniciar siguen los 3 vehículos, 2 mecánicos y 4 repuestos con los mismos IDs. |
| 2 Acceso directo | `2` | ID 5 leído en la posición 1168 (`16 + 4×288`); ID 3 se informa como ELIMINADO. |
| 3 La orden sobrevive | `3a`, `3b`, `3c` | Las líneas, el precio histórico y el total (84.00) se mantienen; las unidades se acumulan (BUJ-003: 2+2=4); al quitar DIS-010 su stock pasa de 12 a 13. |
| 4 Orden con 15 líneas | `4a`, `4b` | Tras reiniciar se muestran las 15 líneas. |
| 5 Anulación | `5a`, `5b` | Stock de la orden #1: FIL-001 36→37, PAS-002 21→22, BUJ-003 52→56, ACE-004 70→74. |
| 6 Baja de mecánico | `6` | Con orden ABIERTA se rechaza; al cerrarla se acepta; sus órdenes siguen mostrando "José Pérez (BAJA)"; una orden nueva con él se rechaza. |
| 7 Integridad | `7a`, `7b` | Detecta: `ROTO: Orden #12 apunta al vehiculo ID 9 (no existe en vehiculos.bin)`. |
| 8 Reportes | `8` | Las órdenes ANULADAS y ABIERTAS no suman en los tres reportes (p. ej. BAT-005 no aparece en el ranking). |

El Caso 7 reemplaza `datos/vehiculos.bin` por la copia del backup entre `7a` y `7b`; el script `correr_pruebas.sh` lo hace automáticamente.

## 10. Decisiones y límites

* **Cédula de mecánico:** única contra *todos* los mecánicos (activos y de baja), porque la baja conserva la identidad que aparece en el historial. La placa de un vehículo, en cambio, solo es única entre los activos.
* **Límite de resultados:** `MAX_RESULTADOS = 50` en listados y búsquedas; el aviso indica cuando hay más. El total facturado del historial de un vehículo se calcula sobre todas sus órdenes aunque se muestren solo las primeras 50.
* **Reporte 2** usa un contador entero por repuesto (no se cargan los registros de repuestos ni de órdenes).
* El taller se identifica con el nombre que se ingresa en el primer arranque; no se puede editar desde el menú.
