#!/bin/sh
# Ejecuta los 8 casos de uso y guarda cada salida en pruebas/salida_*.txt
# Uso (Git Bash en Windows, Linux o macOS), desde cualquier carpeta:  sh pruebas/correr_pruebas.sh
# Requiere el programa ya compilado en la carpeta raiz del proyecto:
#   g++ -std=c++17 proyecto.cpp -o proyecto.exe     (en Windows)   |   -o proyecto   (Linux/macOS)
# Respalda la carpeta datos/ al inicio y la restaura al final.
cd "$(dirname "$0")/.." || exit 1
if [ -f ./proyecto.exe ]; then PROG=./proyecto.exe; elif [ -f ./proyecto ]; then PROG=./proyecto; else echo "Compile primero el programa."; exit 1; fi
cp -r datos .datos_semilla || exit 1

limpio()  { rm -rf datos; }
sembrar() { rm -rf datos; cp -r .datos_semilla datos; }
correr()  { $PROG < "pruebas/entrada_$1.txt" > "pruebas/salida_$1.txt" 2>&1; echo "  ejecutado: entrada_$1.txt -> salida_$1.txt"; }

echo "CU1 persistencia entre ejecuciones (datos vacios)"; limpio; correr 1a; correr 1b
echo "CU2 acceso directo por ID (datos vacios)";          limpio; correr 2
echo "CU3 la orden sobrevive con su detalle";             sembrar; correr 3a; correr 3b; correr 3c
echo "CU4 orden con 15 lineas";                           sembrar; correr 4a; correr 4b
echo "CU5 anulacion con reversion en disco";              sembrar; correr 5a; correr 5b
echo "CU6 baja de mecanico e historial";                  sembrar; correr 6
echo "CU7 integridad tras restauracion parcial";          sembrar; rm -rf backup_*
correr 7a
cp backup_*/vehiculos.bin datos/vehiculos.bin     # reemplazo manual de vehiculos.bin por la copia del backup
correr 7b
rm -rf backup_*
echo "CU8 reportes";                                      sembrar; correr 8

rm -rf datos; cp -r .datos_semilla datos; rm -rf .datos_semilla
echo "Listo. Carpeta datos/ restaurada."
