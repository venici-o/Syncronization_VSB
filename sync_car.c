#include <stdio.h>
#include <stdlib.h>

#include "sync_car.h"

void station_init(struct station *station){
    station->p_aguardando=0;
    station->vagas=0;
    station->p_embarcando=0;
    pthread_mutex_init(&station->mutex, NULL);
    pthread_cond_init(&station->carro_disp, NULL);
    pthread_cond_init(&station->carro_pronto, NULL);
}