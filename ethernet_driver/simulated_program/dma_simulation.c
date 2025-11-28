#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define RING_SIZE 8
#define BUF_SIZE 64
#define PACKETS_TO_SEND 20
#define BATCH_SIZE 3

typedef struct {
    char buffer[BUF_SIZE];
    int length;
    int own;  // 1 = hardware owns, 0 = CPU owns
} rx_desc;

rx_desc ring[RING_SIZE];

pthread_mutex_t poll_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t poll_cond = PTHREAD_COND_INITIALIZER;

int hw_index = 0;   // where NIC/DMA writes
int poll_index = 0; // where CPU polls

// Simulate NIC/DMA writing packets
void* nic_core(void* arg) {
    for (int pkt = 1; pkt <= PACKETS_TO_SEND; pkt++) {
        usleep((rand() % 3 + 1) * 100000); // random delay 0.1-0.3s

        // Check descriptor ownership
        if (ring[hw_index].own == 1) {
            snprintf(ring[hw_index].buffer, BUF_SIZE, "Packet %d", pkt);
            ring[hw_index].length = strlen(ring[hw_index].buffer);
            ring[hw_index].own = 0; // give to CPU

            printf("NIC: wrote %s at desc %d -> signaling poll\n", ring[hw_index].buffer, hw_index);

            // Move to next descriptor
            hw_index = (hw_index + 1) % RING_SIZE;

            // Signal CPU poll core
            pthread_cond_signal(&poll_cond);
        } else {
            printf("NIC: descriptor %d busy, packet %d dropped\n", hw_index, pkt);
        }
    }
    return NULL;
}

// Simulate CPU polling packets in batches
void* poll_core(void* arg) {
    while (1) {
        pthread_mutex_lock(&poll_lock);

        // Wait until at least one packet is available
        int has_packet = 0;
        for (int i = 0; i < RING_SIZE; i++)
            if (ring[i].own == 0) has_packet = 1;

        if (!has_packet)
            pthread_cond_wait(&poll_cond, &poll_lock);

        // Process a batch of packets
        int processed = 0;
        while (processed < BATCH_SIZE && ring[poll_index].own == 0) {
            printf("CPU Poll: processing %s from desc %d\n", ring[poll_index].buffer, poll_index);
            ring[poll_index].own = 1; // return ownership to NIC
            poll_index = (poll_index + 1) % RING_SIZE;
            processed++;
        }

        pthread_mutex_unlock(&poll_lock);

        usleep(100000); // simulate processing delay
    }
    return NULL;
}

int main() {
    pthread_t nic_tid, poll_tid;

    // Initialize ring
    for (int i = 0; i < RING_SIZE; i++) {
        ring[i].own = 1;  // hardware owns initially
        ring[i].length = 0;
        ring[i].buffer[0] = '\0';
    }

    pthread_create(&nic_tid, NULL, nic_core, NULL);
    pthread_create(&poll_tid, NULL, poll_core, NULL);

    pthread_join(nic_tid, NULL);

    // In real kernel, poll thread runs forever; here we stop after NIC is done
    pthread_cancel(poll_tid);
    pthread_mutex_destroy(&poll_lock);
    pthread_cond_destroy(&poll_cond);

    return 0;
}

