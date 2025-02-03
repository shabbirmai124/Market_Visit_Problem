#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define INITIAL_INVENTORY 1
#define NUM_CUSTOMERS 5
#define RESTOCK_INTERVAL 5

sem_t vase_sem;
pthread_mutex_t inventory_mutex;
pthread_cond_t inventory_cond;

int market_inventory = INITIAL_INVENTORY;
int online_inventory = INITIAL_INVENTORY;

void* visit_market(void* arg) {
    int customer_id = *((int*)arg);
    printf("Customer %d is visiting the market...\n", customer_id);
    sleep(1);

    pthread_mutex_lock(&inventory_mutex);
    if (market_inventory > 0) {
        printf("Customer %d sees a vase at the market but decides not to buy it.\n", customer_id);
    } else {
        printf("Customer %d finds the vase not available at the market.\n", customer_id);
    }
    pthread_mutex_unlock(&inventory_mutex);

    return NULL;
}

void* search_online(void* arg) {
    int customer_id = *((int*)arg);
    printf("Customer %d is searching for the vase online...\n", customer_id);
    sleep(2);

    pthread_mutex_lock(&inventory_mutex);
    if (online_inventory > 0) {
        printf("Customer %d finds the vase online and decides to buy it.\n", customer_id);
        online_inventory--;
        printf("Online inventory decreased. Remaining: %d\n", online_inventory);
    } else {
        printf("Customer %d finds the vase not available online.\n", customer_id);
    }
    pthread_mutex_unlock(&inventory_mutex);

    return NULL;
}

void* other_customer(void* arg) {
    int customer_id = *((int*)arg);
    printf("Another customer %d is interested in the vase...\n", customer_id);
    sleep(3);

    sem_wait(&vase_sem);

    pthread_mutex_lock(&inventory_mutex);
    if (market_inventory > 0) {
        printf("Another customer %d buys the vase at the market.\n", customer_id);
        market_inventory--;
        printf("Market inventory decreased. Remaining: %d\n", market_inventory);
    } else if (online_inventory > 0) {
        printf("Another customer %d buys the vase online.\n", customer_id);
        online_inventory--;
        printf("Online inventory decreased. Remaining: %d\n", online_inventory);
    } else {
        printf("The vase is sold out for customer %d.\n", customer_id);
    }
    pthread_mutex_unlock(&inventory_mutex);

    sem_post(&vase_sem);

    return NULL;
}

void* restock_inventory(void* arg) {
    while (1) {
        sleep(RESTOCK_INTERVAL);
        pthread_mutex_lock(&inventory_mutex);

        market_inventory++;
        online_inventory++;

        printf("Restocked! Market inventory: %d, Online inventory: %d\n", market_inventory, online_inventory);

        pthread_cond_broadcast(&inventory_cond);
        pthread_mutex_unlock(&inventory_mutex);
    }
}

int main() {
    pthread_t customers[NUM_CUSTOMERS];
    pthread_t restocker_thread;

    int customer_ids[NUM_CUSTOMERS];

    sem_init(&vase_sem, 0, 1);
    pthread_mutex_init(&inventory_mutex, NULL);
    pthread_cond_init(&inventory_cond, NULL);

    pthread_create(&restocker_thread, NULL, restock_inventory, NULL);

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        customer_ids[i] = i + 1;

        if (pthread_create(&customers[i], NULL, visit_market, &customer_ids[i]) != 0) {
            perror("Failed to create visit_market thread");
            exit(EXIT_FAILURE);
        }

        if (pthread_create(&customers[i], NULL, search_online, &customer_ids[i]) != 0) {
            perror("Failed to create search_online thread");
            exit(EXIT_FAILURE);
        }

        if (pthread_create(&customers[i], NULL, other_customer, &customer_ids[i]) != 0) {
            perror("Failed to create other_customer thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(customers[i], NULL);
    }

    sem_destroy(&vase_sem);
    pthread_mutex_destroy(&inventory_mutex);
    pthread_cond_destroy(&inventory_cond);

    return 0;
}
