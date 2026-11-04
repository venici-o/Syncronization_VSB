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

void station_wait_for_car(struct station *station){
    pthread_mutex_lock(&station->mutex);
    
    station->p_aguardando++;
    
    while (station->vagas == 0){
        pthread_cond_wait(&station->carro_disp, &station->mutex);
    }
    station->vagas--;
    station->p_aguardando--;
    station->p_embarcando++;

    pthread_mutex_unlock(&station->mutex);
}

void station_on_board(struct station *station){
    pthread_mutex_lock(&station->mutex);
    
    station->p_embarcando--;
    pthread_cond_signal(&station->carro_pronto);
    
    pthread_mutex_unlock(&station->mutex);
}   

void station_load_car(struct station *station, int count){
    pthread_mutex_lock(&station->mutex);
    
    station->vagas=count;

    pthread_cond_broadcast(&station->carro_disp);
    while((station->vagas>0 && station->p_aguardando>0)){
        pthread_cond_wait(&station->carro_pronto, &station->mutex);
    }

    pthread_mutex_unlock(&station->mutex);      
}