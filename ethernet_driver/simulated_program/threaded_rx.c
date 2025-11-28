#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define RING_SIZE 4
#define BUF_SIZE 64
#define PACKETS_TO_SEND 10

typedef struct {
    char buffer[BUF_SIZE];
    int length;
    int own;  // 1 = hardware owns, 0 = CPU owns
} rx_desc;

rx_desc ring[RING_SIZE];
int hw_index = 0;
int cpu_index = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t packet_arrived = PTHREAD_COND_INITIALIZER;

// Simulate the interrupt handler
void* cpu_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);

        // Wait until a packet is available
        while (ring[cpu_index].own == 1) {
            pthread_cond_wait(&packet_arrived, &lock);
        }

        // Process all available packets
        while (ring[cpu_index].own == 0) {
            printf("CPU: processing %s from desc %d\n", ring[cpu_index].buffer, cpu_index);
            ring[cpu_index].own = 1;  // give it back to hardware
            cpu_index = (cpu_index + 1) % RING_SIZE;
        }

        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

// Simulate hardware writing packets and raising interrupts
void* hw_thread(void* arg) {
    for (int pkt = 1; pkt <= PACKETS_TO_SEND; pkt++) {
        sleep(rand() % 2); // random delay to simulate asynchronous packet arrival

        pthread_mutex_lock(&lock);

        if (ring[hw_index].own == 1) {
            snprintf(ring[hw_index].buffer, BUF_SIZE, "Packet %d", pkt);
            ring[hw_index].length = strlen(ring[hw_index].buffer);
            ring[hw_index].own = 0; // CPU can now read

            printf("HW: wrote %s at desc %d -> raising interrupt\n", ring[hw_index].buffer, hw_index);
            hw_index = (hw_index + 1) % RING_SIZE;

            // Signal CPU thread like an interrupt
            pthread_cond_signal(&packet_arrived);
        }

        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main() {
    pthread_t cpu_tid, hw_tid;

    // Initialize ring
    for (int i = 0; i < RING_SIZE; i++) {
        ring[i].own = 1;  // hardware owns
        ring[i].length = 0;
        ring[i].buffer[0] = '\0';
    }

    pthread_create(&cpu_tid, NULL, cpu_thread, NULL);
    pthread_create(&hw_tid, NULL, hw_thread, NULL);

    pthread_join(hw_tid, NULL);
    // In real kernel, CPU thread would run forever
    // Here we terminate after hardware finishes
    pthread_cancel(cpu_tid);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&packet_arrived);

    return 0;
}

