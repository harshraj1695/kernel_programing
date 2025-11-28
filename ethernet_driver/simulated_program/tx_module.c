#include <stdio.h>
#include <string.h>

#define RING_SIZE 4
#define BUF_SIZE 64



/*
RX (receive): Hardware writes packets into buffers, CPU reads them.

So own = 1 → hardware owns → can write.

own = 0 → CPU owns → can read.

TX (transmit): CPU writes packets into buffers, hardware sends them.

So own = 0 → CPU owns → can write.

own = 1 → hardware owns → can send.
*/
typedef struct {
    char buffer[BUF_SIZE];
    int length;
    int own;  // 1 = hardware owns, 0 = CPU owns
} rx_desc;

typedef struct {
    char buffer[BUF_SIZE];
    int length;
    int own;  // 0 = CPU owns, 1 = hardware owns
} tx_desc;

int main() {
    // RX ring
    rx_desc rx_ring[RING_SIZE];
    // TX ring
    tx_desc tx_ring[RING_SIZE];

    // Initialize RX ring (hardware owns initially)
    for(int i = 0; i < RING_SIZE; i++) {
        rx_ring[i].own = 1;
        rx_ring[i].length = 0;
        rx_ring[i].buffer[0] = '\0';

        tx_ring[i].own = 0;  // CPU owns TX initially
        tx_ring[i].length = 0;
        tx_ring[i].buffer[0] = '\0';
    }

    int rx_hw_index = 0, rx_cpu_index = 0;
    int tx_cpu_index = 0, tx_hw_index = 0;

    // Simulate RX and TX for 5 cycles
    for(int cycle = 1; cycle <= 5; cycle++) {
        // --- RX: Hardware writes a packet ---
        if(rx_ring[rx_hw_index].own == 1) {
            snprintf(rx_ring[rx_hw_index].buffer, BUF_SIZE, "RX Packet %d", cycle);
            rx_ring[rx_hw_index].length = strlen(rx_ring[rx_hw_index].buffer);
            rx_ring[rx_hw_index].own = 0;  // CPU can read
            printf("HW RX: wrote %s at desc %d\n", rx_ring[rx_hw_index].buffer, rx_hw_index);
            rx_hw_index = (rx_hw_index + 1) % RING_SIZE;
        }

        // --- RX: CPU reads packet ---
        while(rx_ring[rx_cpu_index].own == 0) {
            printf("CPU RX: processing %s from desc %d\n", rx_ring[rx_cpu_index].buffer, rx_cpu_index);
            rx_ring[rx_cpu_index].own = 1; // hand back to hardware
            rx_cpu_index = (rx_cpu_index + 1) % RING_SIZE;
        }

        // --- TX: CPU prepares a packet ---
        if(tx_ring[tx_cpu_index].own == 0) {
            snprintf(tx_ring[tx_cpu_index].buffer, BUF_SIZE, "TX Packet %d", cycle);
            tx_ring[tx_cpu_index].length = strlen(tx_ring[tx_cpu_index].buffer);
            tx_ring[tx_cpu_index].own = 1;  // Hardware can send
            printf("CPU TX: prepared %s at desc %d\n", tx_ring[tx_cpu_index].buffer, tx_cpu_index);
            tx_cpu_index = (tx_cpu_index + 1) % RING_SIZE;
        }

        // --- TX: Hardware sends packet ---
        while(tx_ring[tx_hw_index].own == 1) {
            printf("HW TX: sending %s from desc %d\n", tx_ring[tx_hw_index].buffer, tx_hw_index);
            tx_ring[tx_hw_index].own = 0; // Hand back to CPU
            tx_hw_index = (tx_hw_index + 1) % RING_SIZE;
        }

        printf("---- Cycle %d complete ----\n", cycle);
    }

    return 0;
}

