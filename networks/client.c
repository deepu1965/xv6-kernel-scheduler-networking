#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>

#define MAX_CHUNKS 100 // Maximum number of chunks for message division
#define MAX_MESSAGE_LENGTH 1000 // Maximum allowed message length

int CHUNK_SIZE = 10; // Default chunk size (modifiable via arguments)
int PORT = 8080; // Default port number (modifiable via arguments)
int TIMEOUT = 100000; // Timeout for resending unacknowledged packets in microseconds (0.1 second)

struct packet {
    int seq_no; // Sequence number of the packet
    int total_chunks; // Total number of chunks the message is divided into
    char data[MAX_MESSAGE_LENGTH / MAX_CHUNKS]; // Data portion of the packet (chunk of the message)
};

int sockfd; // Socket file descriptor
struct sockaddr_in server_addr; // Server address structure
socklen_t addr_len; // Length of the address structure
struct packet send_packet; // Packet structure for sending data
int ack_received[MAX_CHUNKS] = {0}; // Track ACKs received for each chunk
char message[MAX_MESSAGE_LENGTH]; // Message to be sent
int total_chunks; // Total number of chunks for the message
int transmission_complete = 0; // Flag to indicate transmission completion
volatile sig_atomic_t keep_running = 1; // Flag to handle interruption signals

// Error handling function
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Signal handler for SIGINT (Ctrl+C)
void sigint_handler(int signum) {
    keep_running = 0;
}

// Thread function to receive ACKs from the server
void *receive_acks(void *arg) {
    int ack;
    while (!transmission_complete && keep_running) {
        // Receive an ACK from the server
        if (recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&server_addr, &addr_len) > 0) {
            printf("Received ACK for chunk %d\n", ack + 1);
            ack_received[ack] = 1; // Mark the chunk as acknowledged
        }
    }
    return NULL;
}

// Thread function to receive response data from the server
void *receive_response(void *arg) {
    struct packet recv_packet; // Packet structure for receiving data
    char received_data[MAX_MESSAGE_LENGTH] = {0}; // Buffer to store received data
    int received_chunks[MAX_CHUNKS] = {0}; // Track received chunks
    int total_response_chunks = -1; // Total chunks in the server's response (initialized as unknown)

    while (keep_running) {
        // Receive a chunk of the response from the server
        if (recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&server_addr, &addr_len) > 0) {
            // Initialize total response chunks when receiving the first packet
            if (total_response_chunks == -1) {
                total_response_chunks = recv_packet.total_chunks;
            }

            // If the chunk has not been received yet, process it
            if (received_chunks[recv_packet.seq_no] == 0) {
                received_chunks[recv_packet.seq_no] = 1; // Mark chunk as received
                memcpy(received_data + recv_packet.seq_no * CHUNK_SIZE, recv_packet.data, CHUNK_SIZE);

                // Send an ACK back to the server for the received chunk
                sendto(sockfd, &recv_packet.seq_no, sizeof(recv_packet.seq_no), 0, (struct sockaddr *)&server_addr, addr_len);
                printf("Received chunk %d/%d from server, sent ACK\n", recv_packet.seq_no + 1, total_response_chunks);
            }

            // Check if all chunks have been received
            int all_received = 1;
            for (int i = 0; i < total_response_chunks; i++) {
                if (!received_chunks[i]) {
                    all_received = 0;
                    break;
                }
            }

            // If all chunks are received, display the full response
            if (all_received) {
                printf("All response chunks received. Full message from server: %s\n", received_data);
                break;
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    // Allow user to modify CHUNK_SIZE, PORT, and TIMEOUT via command-line arguments
    if (argc > 1) CHUNK_SIZE = atoi(argv[1]);
    if (argc > 2) PORT = atoi(argv[2]);
    if (argc > 3) TIMEOUT = atoi(argv[3]) * 1000; // Convert timeout to microseconds

    // Set up signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, sigint_handler);

    addr_len = sizeof(server_addr);

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error("Socket creation failed");
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = htons(PORT); // Server port
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept any incoming address

    // Get message from the user
    printf("Enter the message to send (max %d characters): ", MAX_MESSAGE_LENGTH - 1);
    if (fgets(message, MAX_MESSAGE_LENGTH, stdin) == NULL) {
        error("Error reading input");
    }
    message[strcspn(message, "\n")] = 0; // Remove newline character from input

    // Divide the message into chunks and prepare the packet structure
    total_chunks = (strlen(message) + CHUNK_SIZE - 1) / CHUNK_SIZE;
    send_packet.total_chunks = total_chunks;

    // Create a thread to handle incoming ACKs
    pthread_t ack_thread;
    if (pthread_create(&ack_thread, NULL, receive_acks, NULL) != 0) {
        error("Failed to create ACK thread");
    }

    // Create a thread to handle incoming response from the server
    pthread_t response_thread;
    if (pthread_create(&response_thread, NULL, receive_response, NULL) != 0) {
        error("Failed to create response thread");
    }

    // Send all message chunks to the server without waiting for ACKs initially
    for (int i = 0; i < total_chunks && keep_running; i++) {
        send_packet.seq_no = i; // Set the sequence number
        memset(send_packet.data, 0, CHUNK_SIZE); // Clear the packet data
        strncpy(send_packet.data, message + i * CHUNK_SIZE, CHUNK_SIZE); // Copy the chunk of the message

        // Send the packet to the server
        if (sendto(sockfd, &send_packet, sizeof(send_packet), 0, (struct sockaddr *)&server_addr, addr_len) < 0) {
            error("Failed to send packet");
        }
        printf("Sent chunk %d/%d\n", i + 1, total_chunks);

        // Simulate random ACK loss for every third chunk
        if (i % 3 == 0) {
            printf("Simulating ACK loss for chunk %d\n", i + 1);
            continue;
        }
    }

    // Periodically check for unacknowledged chunks and resend them
    while (keep_running) {
        int all_acked = 1; // Flag to check if all chunks are acknowledged
        for (int i = 0; i < total_chunks; i++) {
            if (!ack_received[i]) { // If a chunk is not acknowledged, resend it
                all_acked = 0;
                printf("Resending chunk %d\n", i + 1);
                send_packet.seq_no = i;
                memset(send_packet.data, 0, CHUNK_SIZE); // Clear packet data
                strncpy(send_packet.data, message + i * CHUNK_SIZE, CHUNK_SIZE); // Copy the chunk again
                if (sendto(sockfd, &send_packet, sizeof(send_packet), 0, (struct sockaddr *)&server_addr, addr_len) < 0) {
                    error("Failed to resend packet");
                }
            }
        }
        if (all_acked) break; // If all chunks are acknowledged, exit the loop
        usleep(TIMEOUT); // Wait before checking again
    }

    // Transmission complete or interrupted
    if (keep_running) {
        printf("All chunks acknowledged. Transmission complete.\n");
    } else {
        printf("Transmission interrupted by user.\n");
    }
    transmission_complete = 1; // Mark transmission as complete

    // Wait for the response and ACK threads to finish
    pthread_join(response_thread, NULL);
    pthread_join(ack_thread, NULL);

    // Close the socket
    close(sockfd);
    return 0;
}
