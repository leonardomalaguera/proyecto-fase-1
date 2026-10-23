# Proyecto 1 - Programación 2

## 1. Descripción del Sistema e Integrantes

### Descripción

Este sistema consiste en una solución desarrollada en C++ para la gestión integral de un taller mecánico. El programa permite administrar el inventario de vehículos, repuestos, mecánicos y la gestión de órdenes de trabajo con sus respectivos detalles. Toda la lógica del sistema está implementada respetando el uso de memoria dinámica con punteros y arreglos dinámicos manuales (sin el uso de contenedores STL como `std::vector`), estando concentrada en un único archivo fuente llamado `proyecto.cpp`.

### Integrantes

* Leonardo Malaguera - **C.I:** 32882803\]}

* Maria Garcia - **C.I:** 33264902

## 2. Instrucciones de Compilación y Ejecución

Dado que todo el código fuente del proyecto se encuentra en el archivo `proyecto.cpp`, la compilación y ejecución se realizan de la siguiente manera:

### Opción A: Desde Visual Studio (IDE)

1. Abrir el proyecto o la solución en Visual Studio donde se encuentra el archivo `proyecto.cpp`.

2. Presionar `Ctrl + Shift + B` o seleccionar **Compilar > Compilar solución** para verificar que el código compile sin errores.

3. Presionar `F5` (modo depuración) o `Ctrl + F5` (ejecución sin depurar) para iniciar el programa en la ventana de consola.

4. Interactuar con el programa ingresando los datos directamente mediante la entrada estándar (`cin`).

### Opción B: Desde Línea de Comandos (Terminal con `g++`)

1. Abrir la terminal o consola en la carpeta donde se encuentra el archivo `proyecto.cpp`.

   1. Compilar el archivo ejecutando el siguiente comando:

      ```
      g++ ./proyecto.cpp -o proyecto
      
      ```

2. Ejecutar el programa interactivo:

   * **En Linux / macOS:**

     ```
     ./proyecto.exe
     
     ```

   * **En Windows (CMD / PowerShell):**

     ```
     proyecto.exe
     
     ```

## 3. Diagrama de Estructuras y sus Relaciones

A continuación se ilustra la estructura general de datos implementada en el archivo `proyecto.cpp` y la relación `Orden -> Detalles`:

```
+-----------------------------------------------------------+
|                          Sistema                          |
+-----------------------------------------------------------+
| - vehiculos : Vehiculo*   (Arreglo dinámico de Vehiculo)  |
| - repuestos : Repuesto*   (Arreglo dinámico de Repuesto)  |
| - mecanicos : Mecanico*   (Arreglo dinámico de Mecanico)  |
| - ordenes   : Orden*      (Arreglo dinámico de Orden)     |
+-----------------------------------------------------------+
                                  |
                                  | (1 a N)
                                  v
+-----------------------------------------------------------+
|                           Orden                           |
+-----------------------------------------------------------+
| - id              : int                                   |
| - idVehiculo      : int                                   |
| - idMecanico      : int                                   |
| - detalles        : DetalleOrden* (Arreglo dinámico)      |
| - numDetalles     : int                                   |
| - capacidadDetalles: int                                  |
+-----------------------------------------------------------+
                                  |
                                  | (Contiene N detalles)
                                  v
+-----------------------------------------------------------+
|                       DetalleOrden                        |
+-----------------------------------------------------------+
| - idRepuesto    : int                                     |
| - cantidad      : int                                     |
| - precioUnitario: double                                  |
+-----------------------------------------------------------+

```

### Relación Orden -> Detalles:

Cada estructura `Orden` contiene internamente un puntero (`DetalleOrden* detalles`) que apunta a un arreglo dinámico de estructuras `DetalleOrden`. La gestión de este arreglo es independiente para cada orden, manteniendo su propio recuento (`numDetalles`) y capacidad máxima (`capacidadDetalles`).

## 4. Explicación de la Separación de Capas

A pesar de que el código completo reside en un único archivo fuente (`proyecto.cpp`), el programa fue estructurado internamente manteniendo una estricta separación conceptual de responsabilidades:

1. **Capa de Datos (Estructuras):**

   * Definición de los `struct` que representan las entidades del dominio (`Vehiculo`, `Repuesto`, `Mecanico`, `DetalleOrden`, `Orden`, `Sistema`). Encargada únicamente de definir los atributos e información a almacenar.

2. **Capa de Lógica de Negocio (Funciones de Gestión):**

   * Funciones encargadas de las operaciones del sistema, validación de reglas del taller, cálculos de totales, y la gestión manual de memoria dinámicamente asignada (`new` y `delete[]`), incluyendo el redimensionamiento de arreglos.

3. **Capa de Presentación / Interfaz de Usuario:**

   * Bloques de código (incluido `main` y funciones auxiliares de lectura/impresión) que interactúan con el usuario mediante la consola. Se encargan de dar formato a las listas, leer comandos y mostrar mensajes de confirmación o error.

## 5. Firmas Diseñadas y Justificación

Para los módulos de mecánicos, repuestos y órdenes se diseñaron firmas cuyos tipos de retorno permiten a la capa de presentación distinguir la causa específica de un fallo sin realizar lógica interna:

### Módulo de Mecánicos

```
int agregarMecanico(Sistema* s, int id, const char* nombre);

```

* **Justificación del tipo de retorno (`int`):**
  Devuelve un código numérico explícito: `1` si la inserción fue exitosa, `0` si el ID del mecánico ya se encuentra registrado (duplicado), y `-1` si ocurrió un error en la asignación de memoria dinámica durante el redimensionamiento.

### Módulo de Repuestos

```
bool buscarRepuestoPorId(const Sistema* s, int idRepuesto, Repuesto* repuestoEncontrado);

```

* **Justificación del tipo de retorno (`bool`):**
  Retorna `true` si el repuesto existe en el inventario y copia sus datos al puntero pasado por parámetro; retorna `false` si no se encuentra. Permite una rápida verificación condicional antes de procesar un detalle de orden.

### Módulo de Órdenes

```
int agregarDetalleAOrden(Orden* orden, int idRepuesto, int cantidad, double precioUnitario);

```

* **Justificación del tipo de retorno (`int`):**
  Retorna `1` si el detalle se añadió correctamente, `-1` si la cantidad ingresada no es válida (menor o igual a cero), y `-2` si falló la ampliación de memoria del arreglo interno de detalles.

## 6. Preguntas de Análisis

### 1. `agregarVehiculo` retorna un puntero a un elemento de `s->vehiculos`. Si guardan ese puntero en una variable, registran varios vehículos más y luego lo usan, ¿qué puede ocurrir y por qué?

**Respuesta:**
Puede ocurrir un error de acceso inválido a memoria o un fallo de segmentación (*segmentation fault*). Esto sucede porque al agregar más vehículos, el arreglo dinámico `s->vehiculos` alcanza su capacidad máxima y se redimensiona. Al redimensionar, el programa reserva un nuevo bloque de memoria más grande con `new[]`, copia los datos existentes y libera el bloque antiguo con `delete[]`. Por lo tanto, cualquier puntero guardado previamente que apuntara a una posición del arreglo viejo se convierte en un **puntero colgado** (*dangling pointer*), haciendo referencia a memoria liberada que ya no es válida.

### 2. ¿Por qué `DetalleOrden` guarda `precioUnitario` en lugar de consultar el precio del repuesto cada vez que se calcula el total?

**Respuesta:**
Se almacena el `precioUnitario` en la estructura `DetalleOrden` para congelar el valor histórico del repuesto en el momento exacto en que se genera la orden. Si en el futuro el precio del repuesto en el catálogo general cambia o se actualiza por inflación, los montos de las órdenes pasadas no deben verse alterados. Además, esto evita realizar búsquedas reiteradas en el arreglo global de repuestos cada vez que se necesita calcular el costo total de la orden.

### 3. Cuando se redimensiona el arreglo de órdenes, ¿qué ocurre con el arreglo de detalles de cada orden? ¿Se copia, se comparte o se pierde? Justifiquen con su implementación.

**Respuesta:**
El arreglo de detalles de cada orden **se comparte (se conserva la misma dirección de memoria)**.

Al redimensionar el arreglo dinámico de órdenes, se crea un nuevo arreglo de tipo `Orden*` y se copian las estructuras `Orden` una a una (*bit a bit* / copia superficial). Puesto que la variable `detalles` dentro de cada estructura `Orden` es simplemente un puntero (`DetalleOrden*`) que almacena una dirección de memoria en el *heap*, la nueva copia de la orden mantiene exactamente la misma dirección de memoria apuntando al arreglo de detalles original. Por ende, la memoria de los detalles no se pierde, ni se duplica, ni requiere ser copiada elemento por elemento durante el redimensionamiento del arreglo principal de órdenes.