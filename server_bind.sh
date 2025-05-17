#!/bin/bash

# Vérification des paramètres
if [ -z "$1" ]; then
  echo "Usage: server_bind <port>"
  exit 1
fi

PORT=$1
FILE="rust/src/bin/main.rs"

# Remplacer la ligne contenant la définition de PORT
sed -i "s/^const PORT\s*:\s*u16\s*=\s*[0-9]\+;/const PORT : u16 = $PORT;/" "$FILE"

echo "Port mis à jour à $PORT dans $FILE"
