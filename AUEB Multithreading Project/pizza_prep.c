#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "pizza_prep.h" 

//mutexes
pthread_mutex_t lock_tel;
pthread_mutex_t lock_cooks;
pthread_mutex_t lock_ovens;
pthread_mutex_t lock_packer;
pthread_mutex_t lock_deliverer;
pthread_mutex_t lock_screen;
pthread_mutex_t lock_stats;

//conditions
pthread_cond_t cond_tel;
pthread_cond_t cond_cooks;
pthread_cond_t cond_ovens;
pthread_cond_t cond_packer;
pthread_cond_t cond_deliverer;

int* id;
unsigned int seed;

//Αρχικοποίση μεταβήτών πόρων
int tel_available = TEL_N;
int cooks_available = COOK_N;
int ovens_available = OVEN_N;
int packer_available = 1;
int del_available = DELIVERER_N;

//Αρχικοποίση μεταβήτών για στατιστικά
int income = 0;
int failed_orders = 0;
int sum_wait_time = 0;
int max_wait_time = 0;
int sum_service_time = 0;
int max_service_time = 0;
int sum_cooling_time = 0;
int max_cooling_time = 0;


void* order (void* x);

int main(int argc, char* argv[]){

    int rc;

    int CUSTOMER_N = atoi(argv[1]);
    id = (int*)malloc(CUSTOMER_N * sizeof(int));
    
    pthread_t threads[CUSTOMER_N];
    
    seed = (unsigned int) atoi(argv[2]);

    //mutex & cond initiation
    pthread_mutex_init(&lock_tel, NULL);
    pthread_mutex_init(&lock_cooks, NULL);
    pthread_mutex_init(&lock_ovens, NULL);
    pthread_mutex_init(&lock_packer,NULL);
    pthread_mutex_init(&lock_deliverer, NULL);
    pthread_mutex_init(&lock_screen, NULL);
    pthread_mutex_init(&lock_stats, NULL);
    pthread_cond_init(&cond_tel, NULL);
    pthread_cond_init(&cond_cooks, NULL);
    pthread_cond_init(&cond_ovens, NULL);
    pthread_cond_init(&cond_packer,NULL);
    pthread_cond_init(&cond_deliverer, NULL);

    for (int i = 0; i < CUSTOMER_N; i++) {
	    id[i] = i+1;
        if (i != 0){
            sleep(rand_r(&seed) % (ORDER_HIGH_T + 1 ) + ORDER_LOW_T ); 
        }
    	rc = pthread_create(&threads[i], NULL, order, &id[i]);
    }
    
    for (int j = 0; j <CUSTOMER_N; j++) {
        pthread_join(threads[j], NULL);
    }
    
    //mutex & cond destruction
    pthread_mutex_destroy(&lock_tel);
    pthread_mutex_destroy(&lock_cooks);
    pthread_mutex_destroy(&lock_ovens);
    pthread_mutex_destroy(&lock_packer);
    pthread_mutex_destroy(&lock_deliverer);
    pthread_mutex_destroy(&lock_screen);
    pthread_mutex_destroy(&lock_stats);
    pthread_cond_destroy(&cond_tel);
    pthread_cond_destroy(&cond_cooks);
    pthread_cond_destroy(&cond_ovens);
    pthread_cond_destroy(&cond_packer);
    pthread_cond_destroy(&cond_deliverer);

    printf("\nΣτατιστικά :\n");
    printf("Έσοδα: %d\nΕπιτυχημένες παραγγελίες: %d\nΑποτυχημένες παραγγελίες:  %d\n",income, CUSTOMER_N-failed_orders, failed_orders);
    printf("Μέσος χρόνος αναμονής πελατών: %d\nΜέγιστος χρόνος αναμονής πελατών: %d\n",sum_wait_time/CUSTOMER_N, max_wait_time);
    printf("Μέσος χρόνος εξυπηρέτησης πελατών: %d\nΜέγιστος χρόνος εξυπηρέτησης πελατών: %d\n",sum_service_time/succesful_orders, max_service_time);
    printf("Μέσος χρόνος κρυώματος των παραγγελιών: %d\nΜέγιστος χρόνος κρυώματος των παραγγελιών: %d\n",sum_cooling_time/succesful_orders, max_cooling_time);
    
    free(id);
    return 0;
}

void *order(void *x){
    
    int order_id = *(int *)x;
    int rc;

    struct timespec start_of_thread;
    struct timespec waiting_time;
    struct timespec prep_time; 
    struct timespec baking_time;
    struct timespec cooling_time;
    struct timespec service_time;

    clock_gettime(CLOCK_REALTIME, &start_of_thread);

    //TELEPHONE STAGE
    pthread_mutex_lock(&lock_tel);

    while (tel_available == 0) {
        pthread_cond_wait(&cond_tel, &lock_tel);
    }

    tel_available--;

    pthread_mutex_unlock(&lock_tel);
    pthread_cond_signal(&cond_tel);

    pthread_mutex_lock(&lock_stats);
    clock_gettime(CLOCK_REALTIME, &waiting_time);
    int real_waiting_time = waiting_time.tv_sec - start_of_thread.tv_sec;
    sum_wait_time = sum_wait_time + real_waiting_time;
    if (real_waiting_time > max_wait_time){
        max_wait_time = real_waiting_time;
    }
    pthread_mutex_unlock(&lock_stats);

    int pizza_n = (rand_r(&seed) % ORDER_HIGH_N + ORDER_LOW_N);
    int card_accepted = rand_r(&seed) % 100;
	
    
    sleep(rand_r(&seed) % PAYMENT_HIGH_T + PAYMENT_LOW_T );
    

    if(card_accepted > 0 && card_accepted <= FAIL_P){

        pthread_mutex_lock(&lock_screen);//screen lock
        printf("Η παραγγελία με αριθμό %d απέτυχε.\n", order_id);
        pthread_mutex_unlock(&lock_screen);//screen unlock

        pthread_mutex_lock(&lock_stats);
        failed_orders++;
        pthread_mutex_unlock(&lock_stats);

        pthread_mutex_lock(&lock_tel);
        tel_available++;
        pthread_mutex_unlock(&lock_tel);
        pthread_cond_signal(&cond_tel);
    }
    else{

        pthread_mutex_lock(&lock_screen);//screen lock
        printf("Η παραγγελία με αριθμό %d καταχωρήθηκε.\n", order_id);
        pthread_mutex_unlock(&lock_screen);//screen unlock
    
        income = income + pizza_n * PIZZA_C;

        pthread_mutex_lock(&lock_tel);
        tel_available++;
        pthread_mutex_unlock(&lock_tel);
        pthread_cond_signal(&cond_tel);

        //PREPERATION BY COOKS STAGE
        pthread_mutex_lock(&lock_cooks);
        while (cooks_available == 0) {
	        pthread_cond_wait(&cond_cooks, &lock_cooks);
        }

        cooks_available--;
        pthread_mutex_unlock(&lock_cooks);
        pthread_cond_signal(&cond_cooks);

        sleep(pizza_n * PREP_T);
        
        //BAKING IN THE OVENS STAGE
        pthread_mutex_lock(&lock_ovens);

        while (pizza_n > ovens_available){
	        pthread_cond_wait(&cond_ovens, &lock_ovens);
        }
        ovens_available = ovens_available - pizza_n;
        pthread_mutex_unlock(&lock_ovens);
        pthread_cond_signal(&cond_ovens);

        pthread_mutex_lock(&lock_cooks);
        cooks_available++;//Αποδέσμευση μάγειρα
        pthread_mutex_unlock(&lock_cooks);
        pthread_cond_signal(&cond_cooks);
        
        sleep(BAKE_T);
       
        //PACKING STAGE
        pthread_mutex_lock(&lock_packer);
        while (packer_available == 0) {
            pthread_cond_wait(&cond_packer, &lock_packer);
        }
        packer_available--;
        pthread_mutex_unlock(&lock_packer);
        pthread_cond_signal(&cond_packer);

        pthread_mutex_lock(&lock_ovens);//2
        ovens_available = ovens_available + pizza_n;

        pthread_mutex_unlock(&lock_ovens);
        pthread_cond_signal(&cond_ovens);

        clock_gettime(CLOCK_REALTIME, &baking_time);//End of baking time 
        int real_baking_time = baking_time.tv_sec - start_of_thread.tv_sec;

        sleep(PACK_T);

        clock_gettime(CLOCK_REALTIME, &prep_time);
        int real_prep_time = prep_time.tv_sec - start_of_thread.tv_sec;

        pthread_mutex_lock(&lock_screen);//screen lock
        printf("Η παραγγελία με αριθμό %d ετοιμάστηκε σε %d λεπτά.\n", order_id, real_prep_time);
        pthread_mutex_unlock(&lock_screen);//screen unlock

        pthread_mutex_lock(&lock_packer);
        packer_available++;
        pthread_mutex_unlock(&lock_packer);
        pthread_cond_signal(&cond_packer);

        //DELIVERY STAGE
        pthread_mutex_lock(&lock_deliverer);
        while (del_available == 0) {
            pthread_cond_wait(&cond_deliverer, &lock_deliverer);
        }

        del_available--;
        int xronos = rand_r(&seed) % DEL_HIGH_T + DEL_LOW_T;  

        pthread_mutex_unlock(&lock_deliverer);
        pthread_cond_signal(&cond_deliverer);

        sleep(xronos);

        pthread_mutex_lock(&lock_stats);
        clock_gettime(CLOCK_REALTIME, &service_time);
        int real_service_time = service_time.tv_sec - start_of_thread.tv_sec;
        sum_service_time = sum_service_time + real_service_time;
        if(real_service_time > max_service_time){
            max_service_time = real_service_time;
        }

        int real_cooling_time = real_service_time - real_baking_time;
        sum_cooling_time = sum_cooling_time + real_cooling_time;
        if(real_cooling_time > max_cooling_time){
            max_cooling_time = real_cooling_time;
        }
        pthread_mutex_unlock(&lock_stats);

        pthread_mutex_lock(&lock_screen);//screen lock
        printf("Η παραγγελία με αριθμό %d παραδόθηκε σε %d λεπτά.\n", order_id, real_service_time);
        pthread_mutex_unlock(&lock_screen);//screen unlock

        sleep(xronos);

        pthread_mutex_lock(&lock_deliverer);
        del_available++;
    
        pthread_mutex_unlock(&lock_deliverer);
        pthread_cond_signal(&cond_deliverer);
    }
    pthread_exit(NULL);
}