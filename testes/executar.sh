#!/bin/sh
set -eu

echo '[RUNNER ORIGINAL]'
./sync_car
echo '[CENARIOS CONTROLADOS]'
./sync_car-tests

invalido() {
    echo "[ENTRADA INVALIDA] $*"
    if "$@"; then
        echo '[FALHA] entrada invalida foi aceita'
        exit 1
    else
        retorno=$?
        if [ "$retorno" -ne 2 ]; then
            echo "[FALHA] esperado retorno 2; observado $retorno"
            exit 1
        fi
    fi
    echo '[OK] entrada rejeitada com retorno 2 antes de criar threads'
}

invalido ./sync_car-tests --cenario -1 2
invalido ./sync_car-tests --cenario 2 -1
invalido ./sync_car-tests --cenario abc 2
invalido ./sync_car-tests --cenario 2 1
echo '[OK FINAL] runner original + 7 cenarios controlados + 4 entradas invalidas'
