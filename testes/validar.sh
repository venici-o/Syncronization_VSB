#!/bin/sh
# Executar a partir da raiz do projeto; script(1) captura esta sessao.
set -eu
export TZ=America/Recife
set -x
date --iso-8601=seconds
whoami
pwd
uname -srmo
gcc --version
make --version
make clean
make
make check
make test
sha256sum sync_car.c sync_car.h sync_car-runner.c Makefile testes/sync_car-tests.c
date --iso-8601=seconds
