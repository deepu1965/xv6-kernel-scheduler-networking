#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define PORT 8080            // Define the port number for the server connection
#define BUFFER_SIZE 1024      // Define the buffer size for communication

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};   // Buffer for sending and receiving data

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    // Set up server address structure
    serv_addr.sin_family = AF_INET;           // IPv4
    serv_addr.sin_port = htons(PORT);         // Server port (convert to network byte order)

    // Convert the IP address from text to binary form and set it
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address\n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }

    printf("Connected to the server. Waiting for the game to start...\n");

    bool playAgain = true;  // Flag to keep track if the player wants to play again

    while (playAgain) {
        // Main loop for receiving and handling server messages during the game
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);   // Clear the buffer
            int valread = recv(sock, buffer, BUFFER_SIZE, 0);  // Receive data from the server

            // If the server disconnects, break out of the game loop
            if (valread <= 0) {
                printf("Server disconnected\n");
                playAgain = false;
                break;
            }

            printf("%s", buffer);  // Print the server message

            // Check if it's the player's turn to make a move
            if (strstr(buffer, "Your turn") || strstr(buffer, "turn (")) {
                int row, col;

                // Loop to get valid input from the player
                do {
                    printf("Enter row and column (0-2): ");
                    if (scanf("%d %d", &row, &col) != 2) {  // Ensure valid input format
                        printf("Invalid input. Please enter two numbers.\n");
                        while (getchar() != '\n');  // Clear input buffer
                        continue;
                    }

                    // Check if the input is within the valid range (0-2)
                    if (row < 0 || row > 2 || col < 0 || col > 2) {
                        printf("Invalid move. Row and column must be between 0 and 2.\n");
                        continue;
                    }

                    break;  // Input is valid, exit loop
                } while (1);

                // Send the player's move to the server
                char move[32];
                snprintf(move, sizeof(move), "%d %d", row, col);
                send(sock, move, strlen(move), 0);
            }

            // Check if the game has ended (someone won or the game is a draw)
            if (strstr(buffer, "Wins") || strstr(buffer, "Draw")) {
                break;
            }
        }

        // Handle "play again" prompt from the server
        memset(buffer, 0, BUFFER_SIZE);  // Clear buffer
        recv(sock, buffer, BUFFER_SIZE, 0);  // Receive the play again prompt from the server
        printf("%s", buffer);

        // If the server asks if the player wants to play again
        if (strstr(buffer, "Do you want to play again?")) {
            char response[10];
            printf("Enter 'yes' to play again or 'no' to quit: ");
            scanf("%s", response);  // Get the player's decision

            // Send the player's decision to the server
            send(sock, response, strlen(response), 0);

            // Wait for the server's response on whether the game will end or restart
            memset(buffer, 0, BUFFER_SIZE);  // Clear buffer
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("%s", buffer);

            // If the server indicates that the game is ending, set playAgain to false
            if (strstr(buffer, "Ending game")) {
                playAgain = false;
            }
        }
    }

    // Close the socket connection when done
    close(sock);
    return 0;
}
