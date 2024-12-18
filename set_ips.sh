#!/bin/bash

# Verificar si se proporcionaron los cuatro argumentos esperados (IP_MEMORIA, IP_CPU, IP_KERNEL, IP_FILESYSTEM)
if [ $# -ne 4 ]; then
  echo "Uso: $0 IP_MEMORIA IP_CPU IP_KERNEL IP_FILESYSTEM"
  exit 1
fi

# Asignar los argumentos a variables
IP_MEMORIA=$1
IP_CPU=$2
IP_KERNEL=$3
IP_FILESYSTEM=$4

# Definimos la ruta principal como el directorio actual
RUTA_PRINCIPAL="$PWD"

# Función para actualizar el valor de una clave en un archivo de configuración
function actualizar_valor {
  archivo=$1
  clave=$2
  valor=$3

  # Usamos el comando "sed" para reemplazar la línea que contiene la clave con el nuevo valor
  sed -i "s/^\($clave\s*=\s*\).*$/\1$valor/" "$archivo"
}

# Función para recorrer recursivamente los subdirectorios y actualizar los archivos de configuración
function actualizar_archivos {
  directorio=$1

  # Recorremos todos los archivos en el directorio actual
  for archivo in "$directorio"/*.config; do
    # Verificamos si el archivo existe y es un archivo regular
    if [ -f "$archivo" ]; then
      actualizar_valor "$archivo" "IP_MEMORIA" "$IP_MEMORIA"
      actualizar_valor "$archivo" "IP_CPU" "$IP_CPU"
      actualizar_valor "$archivo" "IP_KERNEL" "$IP_KERNEL"
      actualizar_valor "$archivo" "IP_FILESYSTEM" "$IP_FILESYSTEM"
    fi
  done

  # Recorremos todos los subdirectorios
  for subdir in "$directorio"/*/; do
    # Verificamos si el directorio existe y es un directorio
    if [ -d "$subdir" ]; then
      actualizar_archivos "$subdir" # Llamada recursiva para actualizar los archivos dentro del subdirectorio
    fi
  done
}

# Llamamos a la función para actualizar los archivos dentro de la ruta principal
actualizar_archivos "$RUTA_PRINCIPAL"
