# Sincronização do metrô

Atividade de Infraestrutura de Software em C com threads POSIX. Usa um mutex e duas variáveis de condição por estação para controlar as reservas e permitir embarques simultâneos. O carro aguarda as confirmações antes de partir.

## Ambiente

Desenvolvido em **CachyOS Linux**, com **GCC 16.2.1** e **GNU Make 4.4.1**. Os testes adicionais exigem um linker compatível com `--wrap`, como o GNU ld.

## Arquivos

- `sync_car.c`: implementação das quatro funções de sincronização.
- `sync_car.h`: estrutura da estação e declarações das funções.
- `sync_car-runner.c`: teste original com 100 passageiros e capacidades aleatórias.
- `testes/sync_car-tests.c`: casos limites, embarque simultâneo e validação de entradas do programa de testes.
- `testes/executar.sh` e `testes/validar.sh`: execução da suíte e coleta de evidências.
- `Makefile`: compilação, testes e limpeza.
- `resultados/`: logs e capturas; `evidencias_completas.log` reúne as evidências.
- `vsb.docx`: relatório preenchido no modelo da atividade.

## Compilar, executar e testar

Na pasta do projeto:

```sh
make           # Gera o executável sync_car
./sync_car     # Executa o runner original
make check     # Verifica a sintaxe da implementação
make test      # Executa a suíte completa
make clean     # Remove executáveis e objetos
```

O runner imprime `Looks good!` quando passa. A suíte completa verifica sete cenários controlados e quatro entradas inválidas da interface de testes, além de executar o runner original.

Para testar cinco passageiros com carros de duas e três vagas, após `make test`:

```sh
./sync_car-tests --cenario 5 2 3
```

A implementação pressupõe um carro por estação por vez, capacidade não negativa e uma confirmação por reserva.
