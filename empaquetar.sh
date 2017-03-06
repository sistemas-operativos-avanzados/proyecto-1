#!/bin/bash

PAQUETE=proyecto-1-RMDON.tgz

# Borrando paquete, si existe
rm -rf $PAQUETE

# Borrando la carpeta target de cada proyecto, si existe
rm -rf secuencial/target forked/target threaded/target preforked/target prethreaded/target cliente/target

# Empaquetar
echo "==> Empaquetando..."
tar -zcvf $PAQUETE secuencial/ forked/ threaded/ preforked/ prethreaded/ cliente/ README.md
echo $'<== Fin de empaquetado \n\n\n'

echo "%%% Probando los contenidos del paquete"
# Probar que todo se empaqueto correctamente
tar -zvtf $PAQUETE
echo "%%% Fin de prueba"
