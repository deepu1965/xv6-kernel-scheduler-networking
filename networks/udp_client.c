#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define PORT 8080       // Port number for the server
#define BUFFER_SIZE 1024 // Size of the buffer for data transfer
#define SERVER_IP "127.0.0.1" // IP address of the server

int main() {
    int sockfd;  // Socket file descriptor
    struct sockaddr_in server_addr;  // Structure for server address
    char buffer[BUFFER_SIZE];  // Buffer to store messages
    socklen_t addr_len = sizeof(server_addr); // Length of the server address

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed"); // Print error if socket creation fails
        exit(EXIT_FAILURE);  // Exit the program
    }

    // Clear the server address structure and set its fields
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;  // Use IPv4
    server_addr.sin_port = htons(PORT);  // Convert port number to network byte order

    // Convert IP address from text to binary form and check for errors
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");  // Print error if IP address is invalid
        exit(EXIT_FAILURE);  // Exit the program
    }

    printf("Connecting to the server...\n");
    // Send a "Connect" message to the server to initiate the connection
    sendto(sockfd, "Connect", 7, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    bool playAgain = true;  // Flag to control the game loop
    while (playAgain) {
        // Main game loop
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);  // Clear the buffer
            // Receive a message from the server
            int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
            if (n <= 0) {
                // If no data is received or server disconnects, end the game
                printf("Server disconnected\n");
                playAgain = false;  // Exit the game loop
                break;
            }

            printf("%s", buffer);  // Print the received message from the server

            // If it's the player's turn, prompt them to enter a move
            if (strstr(buffer, "Your turn") || strstr(buffer, "Enter row and column")) {
                int row, col;
                do {
                    // Ask the player to enter the row and column for their move
                    printf("Enter row and column (0-2): ");
                    if (scanf("%d %d", &row, &col) != 2) {
                        // If invalid input (non-numeric), print error and prompt again
                        printf("Invalid input. Please enter two numbers.\n");
                        while (getchar() != '\n'); // Clear the input buffer
                        continue;
                    }
                    // Ensure that the row and column are valid (between 0 and 2)
                    if (row < 0 || row > 2 || col < 0 || col > 2) {
                        printf("Invalid move. Row and column must be between 0 and 2.\n");
                        continue;
                    }
                    break;  // Valid move, break the loop
                } while (1);

                // Send the player's move to the server
                char move[32];
                snprintf(move, sizeof(move), "%d %d", row, col);
                sendto(sockfd, move, strlen(move), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            }

            // Check if the game is over (win or draw)
            if (strstr(buffer, "Wins") || strstr(buffer, "Draw")) {
                break;  // Exit the inner game loop if game ends
            }
        }

        // Handle the "play again" prompt after a game finishes
        memset(buffer, 0, BUFFER_SIZE);  // Clear the buffer
        recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);  // Receive prompt
        printf("%s", buffer);  // Print the server's prompt

        // If server asks to play again, prompt the player for a response
        if (strstr(buffer, "Do you want to play again?")) {
            char response[10];  // Buffer for the player's response
            printf("Enter 'yes' to play again or 'no' to quit: ");
            scanf("%s", response);  // Get the player's response
            sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));  // Send response to server

            // Wait for server's decision on whether to start a new game or end
            memset(buffer, 0, BUFFER_SIZE);  // Clear the buffer
            recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);  // Receive response
            printf("%s", buffer);  // Print the server's message

            // If server decides to end the game, set the flag to false to exit the loop
            if (strstr(buffer, "Ending game")) {
                playAgain = false;  // Stop the game loop
            }
        }
    }

    close(sockfd);  // Close the socket connection
    return 0;  // Exit the program
}
