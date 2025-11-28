#include <stdio.h>
#include <string.h>

#define RING_SIZE 4
#define BUF_SIZE 64

typedef struct {
    char buffer[BUF_SIZE];
    int length;
    int own;  // 1 = hardware owns, 0 = CPU owns
} rx_desc;

int main() {
    rx_desc ring[RING_SIZE];
    
    // Initialize the ring
    for(int i = 0; i < RING_SIZE; i++) {
        ring[i].own = 1;  // hardware owns
        ring[i].length = 0;
        ring[i].buffer[0] = '\0';
    }

    int hw_index = 0; // hardware writes here
    int cpu_index = 0; // CPU reads from here

    // Simulate 5 packet arrivals
    for(int pkt = 1; pkt <= 5; pkt++) {
        // Hardware writes a packet if it owns the descriptor
        if(ring[hw_index].own == 1) {
            snprintf(ring[hw_index].buffer, BUF_SIZE, "Packet %d", pkt);
            ring[hw_index].length = strlen(ring[hw_index].buffer);
            ring[hw_index].own = 0; // CPU can now read
            printf("HW: wrote %s at desc %d\n", ring[hw_index].buffer, hw_index);
            hw_index = (hw_index + 1) % RING_SIZE;
        }

        // CPU processes any ready packets
        while(ring[cpu_index].own == 0) {
            printf("CPU: processing %s from desc %d\n", ring[cpu_index].buffer, cpu_index);
            ring[cpu_index].own = 1;  // give it back to hardware
            cpu_index = (cpu_index + 1) % RING_SIZE;
        }
    }

    return 0;
}

