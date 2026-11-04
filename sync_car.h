#include <pthread.h>

struct station{
    pthread_mutex_t mutex;
    pthread_cond_t carro_disp;
    pthread_cond_t carro_pronto;
    int p_aguardando;
    int vagas;
    int p_embarcando;
};

void station_init(struct station *station);
void station_load_car(struct station *station, int count);
void station_wait_for_car(struct station *station);
void station_on_board(struct station *station);
