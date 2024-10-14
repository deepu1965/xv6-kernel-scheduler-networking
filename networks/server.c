#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#define MAX_CHUNKS 100
#define MAX_MESSAGE_LENGTH 1000

int CHUNK_SIZE = 10;  // Chunk size for sending data in parts
int PORT = 8080;  // Default port number
int TIMEOUT = 100000; // Timeout for retransmission (0.1 second)

// Packet structure to represent each data chunk
struct packet {
    int seq_no;  // Sequence number of the chunk
    int total_chunks;  // Total number of chunks in the message
    char data[MAX_MESSAGE_LENGTH / MAX_CHUNKS];  // Data in this chunk
};

// Global variables for socket communication
int sockfd;
struct sockaddr_in server_addr, client_addr;
socklen_t addr_len;
char received_data[MAX_MESSAGE_LENGTH];  // Buffer to store received data
int received_chunks[MAX_CHUNKS] = {0};  // Track which chunks have been received
int total_chunks = -1;  // Total number of chunks in the current message
int ack_received[MAX_CHUNKS] = {0};  // Track which chunks have been acknowledged by the client
char response[MAX_MESSAGE_LENGTH];  // Buffer to store the response to be sent
int response_total_chunks;  // Total number of chunks in the response
int transmission_complete = 0;  // Flag to indicate if the response transmission is complete
volatile sig_atomic_t keep_running = 1;  // Flag for graceful shutdown on SIGINT

void error(const char *msg) {
    perror(msg);  // Print error message and exit
    exit(1);
}

// Signal handler for SIGINT to stop the server gracefully
void sigint_handler(int signum) {
    keep_running = 0;  // Set flag to stop the server loop
}

// Thread function to receive ACKs from the client
void *receive_acks(void *arg) {
    int ack;
    while (!transmission_complete && keep_running) {
        // Receive acknowledgment from the client
        if (recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&client_addr, &addr_len) > 0) {
            printf("Received ACK for chunk %d\n", ack + 1);
            ack_received[ack] = 1;  // Mark the chunk as acknowledged
        }
    }
    return NULL;
}

// Function to send response data to the client
void send_response() {
    struct packet send_packet;
    response_total_chunks = (strlen(response) + CHUNK_SIZE - 1) / CHUNK_SIZE;  // Calculate total chunks
    send_packet.total_chunks = response_total_chunks;

    // Create a thread to handle receiving ACKs in parallel
    pthread_t ack_thread;
    if (pthread_create(&ack_thread, NULL, receive_acks, NULL) != 0) {
        error("Failed to create ACK thread");
    }

    // Send response chunks to the client
    for (int i = 0; i < response_total_chunks && keep_running; i++) {
        send_packet.seq_no = i;
        memset(send_packet.data, 0, CHUNK_SIZE);  // Clear previous chunk data
        strncpy(send_packet.data, response + i * CHUNK_SIZE, CHUNK_SIZE);  // Copy next chunk

        // Send the chunk
        if (sendto(sockfd, &send_packet, sizeof(send_packet), 0, (struct sockaddr *)&client_addr, addr_len) < 0) {
            error("Failed to send packet");
        }
        printf("Sent chunk %d/%d\n", i + 1, response_total_chunks);

        // Simulate random ACK loss for every 3rd packet
        if (i % 3 == 0) {
            printf("Simulating ACK loss for chunk %d\n", i + 1);
            continue;  // Skip sending ACK for this chunk to simulate loss
        }
    }

    // Resend chunks that have not been acknowledged
    while (keep_running) {
        int all_acked = 1;
        for (int i = 0; i < response_total_chunks; i++) {
            if (!ack_received[i]) {
                all_acked = 0;
                printf("Resending chunk %d\n", i + 1);
                send_packet.seq_no = i;
                memset(send_packet.data, 0, CHUNK_SIZE);
                strncpy(send_packet.data, response + i * CHUNK_SIZE, CHUNK_SIZE);
                if (sendto(sockfd, &send_packet, sizeof(send_packet), 0, (struct sockaddr *)&client_addr, addr_len) < 0) {
                    error("Failed to resend packet");
                }
            }
        }
        if (all_acked) break;  // Exit loop when all chunks are acknowledged
        usleep(TIMEOUT);  // Wait before checking again
    }

    // Check if the transmission was successful or interrupted
    if (keep_running) {
        printf("All response chunks acknowledged. Transmission complete.\n");
    } else {
        printf("Transmission interrupted by user.\n");
    }
    transmission_complete = 1;  // Mark the transmission as complete
    pthread_join(ack_thread, NULL);  // Wait for ACK thread to finish
}

int main(int argc, char *argv[]) {
    // Allow the user to set chunk size, port, and timeout from command-line arguments
    if (argc > 1) CHUNK_SIZE = atoi(argv[1]);
    if (argc > 2) PORT = atoi(argv[2]);
    if (argc > 3) TIMEOUT = atoi(argv[3]) * 1000; // Convert to microseconds

    signal(SIGINT, sigint_handler);  // Set up signal handler for graceful shutdown

    addr_len = sizeof(client_addr);  // Initialize address length
    struct packet recv_packet;

    memset(received_data, 0, sizeof(received_data));  // Clear received data buffer

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error("Socket creation failed");
    }

    // Initialize server and client addresses
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;  // Set address family to IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any available address
    server_addr.sin_port = htons(PORT);  // Set the port number

    // Bind the socket to the port
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        error("Bind failed");
    }

    printf("Server is listening on port %d...\n", PORT);

    // Main server loop
    while (keep_running) {
        // Receive data packet from client
        if (recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&client_addr, &addr_len) < 0) {
            if (keep_running) {
                error("Failed to receive packet");
            } else {
                break;  // Exit the loop if server is shutting down
            }
        }
        
        // Set total chunks on first received packet
        if (total_chunks == -1) {
            total_chunks = recv_packet.total_chunks;
        }

        // Process the received chunk if it hasn't been received yet
        if (received_chunks[recv_packet.seq_no] == 0) {
            received_chunks[recv_packet.seq_no] = 1;  // Mark chunk as received
            memcpy(received_data + recv_packet.seq_no * CHUNK_SIZE, recv_packet.data, CHUNK_SIZE);  // Copy the chunk data

            // Send an acknowledgment for the received chunk
            if (sendto(sockfd, &recv_packet.seq_no, sizeof(recv_packet.seq_no), 0, (struct sockaddr *)&client_addr, addr_len) < 0) {
                error("Failed to send ACK");
            }
            printf("Received chunk %d/%d, sent ACK\n", recv_packet.seq_no + 1, total_chunks);
        }

        // Check if all chunks have been received
        int all_received = 1;
        for (int i = 0; i < total_chunks; i++) {
            if (!received_chunks[i]) {
                all_received = 0;
                break;
            }
        }

        // If all chunks received, display the full message and send a response
        if (all_received) {
            printf("All chunks received. Full message: %s\n", received_data);
            
            // Prompt the user for a response
            printf("Enter response message (max %d characters): ", MAX_MESSAGE_LENGTH - 1);
            if (fgets(response, MAX_MESSAGE_LENGTH, stdin) == NULL) {
                error("Error reading input");
            }
            response[strcspn(response, "\n")] = 0; // Remove newline character
            
            send_response();  // Send the response to the client
            
            // Reset for next message
            memset(received_data, 0, sizeof(received_data));
            memset(received_chunks, 0, sizeof(received_chunks));
            total_chunks = -1;
            transmission_complete = 0;
        }
    }

    printf("Server shutting down...\n");
    close(sockfd);  // Close the socket and clean up
    return 0;
}
