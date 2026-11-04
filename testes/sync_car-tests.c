/* Testes adicionais gerados com auxilio de IA.
 * O --wrap do GNU ld observa as esperas sem modificar sync_car.c.
 * Os contadores do controlador pertencem somente ao teste, nao a station.
 * Nao ha sleeps/polling para estabelecer a ordem dos eventos.
 */
#include "../sync_car.h"
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PASSAGEIROS 256

struct controle {
    struct station estacao;
    pthread_mutex_t mutex;
    pthread_cond_t mudou;
    int chegaram, reservaram, confirmaram, permissoes;
    int esperas_carro, carro_retornou, capacidade;
};

static struct controle *atual;
static _Thread_local int chegada_registrada;

static void exigir(int condicao, const char *mensagem)
{
    if (!condicao) {
        fprintf(stderr, "[FALHA] %s\n", mensagem);
        exit(1);
    }
}

static void verificar_pthread(int resultado, const char *operacao)
{
    if (resultado) {
        fprintf(stderr, "[FALHA] %s: %s\n", operacao, strerror(resultado));
        exit(1);
    }
}

#define PTHREAD(expr) verificar_pthread((expr), #expr)

int __real_pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);

int __wrap_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    struct controle *c = atual;
    /* Aqui a thread ainda possui o mutex da estacao. Ordem dos locks:
     * estacao -> controlador. O controlador nunca faz o caminho inverso. */
    if (c && (cond == &c->estacao.carro_disp ||
              cond == &c->estacao.carro_pronto)) {
        PTHREAD(pthread_mutex_lock(&c->mutex));
        if (cond == &c->estacao.carro_disp && !chegada_registrada) {
            chegada_registrada = 1;
            c->chegaram++;
        }
        if (cond == &c->estacao.carro_pronto)
            c->esperas_carro++;
        PTHREAD(pthread_cond_broadcast(&c->mudou));
        PTHREAD(pthread_mutex_unlock(&c->mutex));
    }
    return __real_pthread_cond_wait(cond, mutex);
}

static void *passageiro(void *arg)
{
    struct controle *c = arg;
    station_wait_for_car(&c->estacao);
    PTHREAD(pthread_mutex_lock(&c->mutex));
    c->reservaram++;
    PTHREAD(pthread_cond_broadcast(&c->mudou));
    /* A confirmacao so e liberada pelo controlador depois das reservas. */
    while (c->permissoes == 0)
        PTHREAD(__real_pthread_cond_wait(&c->mudou, &c->mutex));
    c->permissoes--;
    PTHREAD(pthread_mutex_unlock(&c->mutex));
    station_on_board(&c->estacao);
    PTHREAD(pthread_mutex_lock(&c->mutex));
    c->confirmaram++;
    PTHREAD(pthread_cond_broadcast(&c->mudou));
    PTHREAD(pthread_mutex_unlock(&c->mutex));
    return NULL;
}

static void *carro(void *arg)
{
    struct controle *c = arg;
    station_load_car(&c->estacao, c->capacidade);
    PTHREAD(pthread_mutex_lock(&c->mutex));
    c->carro_retornou = 1;
    PTHREAD(pthread_cond_broadcast(&c->mudou));
    PTHREAD(pthread_mutex_unlock(&c->mutex));
    return NULL;
}

static void conferir_estado(struct controle *c, int aguardando,
                            int vagas, int embarcando, int retornou)
{
    PTHREAD(pthread_mutex_lock(&c->estacao.mutex));
    PTHREAD(pthread_mutex_lock(&c->mutex));
    printf("  estado: aguardando=%d vagas=%d embarcando=%d carro_retornou=%d\n",
           c->estacao.p_aguardando, c->estacao.vagas,
           c->estacao.p_embarcando, c->carro_retornou);
    exigir(c->estacao.p_aguardando == aguardando, "quantidade aguardando incorreta");
    exigir(c->estacao.vagas == vagas, "vagas incorretas (reserva indevida ou vaga residual)");
    exigir(c->estacao.p_embarcando == embarcando, "quantidade embarcando incorreta");
    exigir(c->carro_retornou == retornou, "carro partiu antes da ultima confirmacao");
    PTHREAD(pthread_mutex_unlock(&c->mutex));
    PTHREAD(pthread_mutex_unlock(&c->estacao.mutex));
}

static void executar_cenario(const char *nome, int quantidade,
                             const int *capacidades, int numero_carros)
{
    struct controle c = {0};
    pthread_t passageiros[MAX_PASSAGEIROS];
    int total = 0;
    printf("\n[CASO] %s | passageiros=%d\n", nome, quantidade);
    alarm(10); /* Limite de falha, nao mecanismo de sincronizacao. */
    station_init(&c.estacao);
    PTHREAD(pthread_mutex_init(&c.mutex, NULL));
    PTHREAD(pthread_cond_init(&c.mudou, NULL));
    atual = &c;
    for (int i = 0; i < quantidade; i++)
        PTHREAD(pthread_create(&passageiros[i], NULL, passageiro, &c));

    PTHREAD(pthread_mutex_lock(&c.mutex));
    while (c.chegaram < quantidade)
        PTHREAD(__real_pthread_cond_wait(&c.mudou, &c.mutex));
    exigir(c.reservaram == 0, "passageiro reservou sem carro");
    PTHREAD(pthread_mutex_unlock(&c.mutex));
    conferir_estado(&c, quantidade, 0, 0, 0);

    for (int k = 0; k < numero_carros; k++) {
        pthread_t trem;
        int restantes = quantidade - total;
        int esperados = restantes < capacidades[k] ? restantes : capacidades[k];
        PTHREAD(pthread_mutex_lock(&c.mutex));
        c.capacidade = capacidades[k];
        c.carro_retornou = 0;
        PTHREAD(pthread_mutex_unlock(&c.mutex));
        printf("  carro %d: capacidade=%d, embarques esperados=%d\n",
               k + 1, capacidades[k], esperados);
        PTHREAD(pthread_create(&trem, NULL, carro, &c));

        if (esperados > 0) {
            PTHREAD(pthread_mutex_lock(&c.mutex));
            while (c.reservaram < total + esperados && !c.carro_retornou)
                PTHREAD(__real_pthread_cond_wait(&c.mudou, &c.mutex));
            exigir(c.reservaram == total + esperados, "numero de reservas incorreto");
            exigir(c.confirmaram == total, "confirmacao ocorreu antes da liberacao");
            exigir(!c.carro_retornou, "carro saiu antes das confirmacoes");
            PTHREAD(pthread_mutex_unlock(&c.mutex));
            conferir_estado(&c, restantes - esperados,
                            capacidades[k] - esperados, esperados, 0);
            printf("  %d reserva(s) concluidas antes de qualquer confirmacao deste carro\n",
                   esperados);

            /* Confirma um de cada vez. Cada confirmacao parcial deve fazer
             * o carro voltar a esperar (ou denunciara uma partida precoce). */
            for (int j = 1; j <= esperados; j++) {
                PTHREAD(pthread_mutex_lock(&c.mutex));
                int esperas_antes = c.esperas_carro;
                c.permissoes++;
                PTHREAD(pthread_cond_broadcast(&c.mudou));
                while (c.confirmaram < total + j)
                    PTHREAD(__real_pthread_cond_wait(&c.mudou, &c.mutex));
                if (j < esperados) {
                    while (c.esperas_carro <= esperas_antes && !c.carro_retornou)
                        PTHREAD(__real_pthread_cond_wait(&c.mudou, &c.mutex));
                    exigir(!c.carro_retornou, "carro partiu com confirmacao pendente");
                }
                PTHREAD(pthread_mutex_unlock(&c.mutex));
                if (j == esperados - 1) {
                    conferir_estado(&c, restantes - esperados,
                                    capacidades[k] - esperados, 1, 0);
                    puts("  ultima confirmacao retida: carro continua esperando");
                }
            }
        }
        PTHREAD(pthread_join(trem, NULL));
        total += esperados;
        conferir_estado(&c, quantidade - total, 0, 0, 1);
    }
    exigir(total == quantidade, "cenario terminou sem atender todos os passageiros");
    for (int i = 0; i < quantidade; i++)
        PTHREAD(pthread_join(passageiros[i], NULL));
    exigir(c.reservaram == quantidade && c.confirmaram == quantidade,
           "totais finais inconsistentes");
    atual = NULL;
    PTHREAD(pthread_cond_destroy(&c.estacao.carro_disp));
    PTHREAD(pthread_cond_destroy(&c.estacao.carro_pronto));
    PTHREAD(pthread_mutex_destroy(&c.estacao.mutex));
    PTHREAD(pthread_cond_destroy(&c.mudou));
    PTHREAD(pthread_mutex_destroy(&c.mutex));
    alarm(0);
    printf("[OK] %s\n", nome);
}

static int numero(const char *texto, int limite, int *valor)
{
    char *fim;
    errno = 0;
    long n = strtol(texto, &fim, 10);
    if (errno || fim == texto || *fim || n < 0 || n > limite)
        return 0;
    *valor = (int)n;
    return 1;
}

static void timeout_handler(int sinal)
{
    (void)sinal;
    const char msg[] = "[FALHA] timeout de 10 s: possivel bloqueio ou ambiente sobrecarregado\n";
    (void)write(STDERR_FILENO, msg, sizeof(msg) - 1);
    _exit(1);
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGALRM, timeout_handler);
    if (argc > 1) {
        int passageiros, capacidades[32];
        long total = 0;
        if (argc < 4 || argc > 35 || strcmp(argv[1], "--cenario") ||
            !numero(argv[2], MAX_PASSAGEIROS, &passageiros)) {
            fputs("Entrada invalida: use --cenario PASSAGEIROS VAGAS [VAGAS...]\n", stderr);
            return 2;
        }
        for (int i = 3; i < argc; i++) {
            if (!numero(argv[i], 1000000, &capacidades[i - 3])) {
                fputs("Entrada invalida: vagas devem ser inteiras entre 0 e 1000000\n", stderr);
                return 2;
            }
            total += capacidades[i - 3];
        }
        if (total < passageiros) {
            fputs("Entrada invalida: o cenario precisa de carros suficientes para concluir\n", stderr);
            return 2;
        }
        executar_cenario("personalizado", passageiros, capacidades, argc - 3);
        return 0;
    }
    const int vazio[] = {0}, sem_passageiros[] = {10};
    const int sem_vagas[] = {0, 3}, sobras[] = {5}, exato[] = {4};
    const int varios[] = {2, 3}, simultaneo[] = {3};
    executar_cenario("T01 - estacao vazia e zero vagas", 0, vazio, 1);
    executar_cenario("T02 - carro sai vazio e retira vagas residuais", 0, sem_passageiros, 1);
    executar_cenario("T03 - zero vagas com tres passageiros; proximo carro atende", 3, sem_vagas, 2);
    executar_cenario("T04 - dois passageiros e cinco vagas", 2, sobras, 1);
    executar_cenario("T05 - quatro passageiros e quatro vagas", 4, exato, 1);
    executar_cenario("T06 - cinco passageiros em carros de duas e tres vagas", 5, varios, 2);
    executar_cenario("T07 - tres reservas simultaneas e ultima confirmacao retida", 3, simultaneo, 1);
    puts("\n[OK] 7 cenarios controlados aprovados.");
    return 0;
}
