#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define PORT 8080

char board[3][3];  // Tic-Tac-Toe board
int currentPlayer = 1; // Keeps track of the current player (1 = 'X', 2 = 'O')

// Resets the board by initializing all positions to an empty space ' '
void resetBoard() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
}

// Checks if a player has won
// Returns 1 if there is a win, 0 otherwise
int checkWin() {
    // Check rows and columns
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return 1;  // Row match
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return 1;  // Column match
    }
    // Check diagonals
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ')
        return 1;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
        return 1;

    return 0;  // No win detected
}

// Checks if the game is a draw by verifying if all positions are filled
int checkDraw() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == ' ') {
                return 0;  // Empty position found, not a draw yet
            }
        }
    }
    return 1;  // All positions filled, it's a draw
}

// Updates the board by placing the player's symbol ('X' or 'O') in the specified row and column
void updateBoard(int row, int col, char symbol) {
    board[row][col] = symbol;
}

// Sends the current state of the board to both players
void sendBoardState(int player1, int player2) {
    char message[1024];
    snprintf(message, sizeof(message),
             "\nCurrent board state:\n %c | %c | %c\n---|---|---\n %c | %c | %c\n---|---|---\n %c | %c | %c\n",
             board[0][0], board[0][1], board[0][2], board[1][0], board[1][1], board[1][2], board[2][0], board[2][1], board[2][2]);
    // Send the board state to both players
    send(player1, message, strlen(message), 0);
    send(player2, message, strlen(message), 0);
}

// Asks a player if they want to play again, based on their response (yes/no)
bool askPlayAgain(int player) {
    char message[] = "Do you want to play again? (yes/no): ";
    send(player, message, strlen(message), 0);

    char buffer[1024] = {0};
    int valread = recv(player, buffer, 1024, 0);
    
    if (valread > 0) {
        buffer[valread] = '\0';  // Null-terminate the response
        if (strstr(buffer, "yes") != NULL) {
            return true;  // Player wants to play again
        } else if (strstr(buffer, "no") != NULL) {
            return false;  // Player does not want to play again
        }
    }

    // If no valid response or connection error, assume they don't want to play again
    return false;
}

int main() {
    int server_fd, new_socket1, new_socket2;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Create a socket (IPv4, TCP)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Accept connections from any IP address
    address.sin_port = htons(PORT);

    // Bind the socket to the server address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections, up to 2 players
    if (listen(server_fd, 2) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");
    // Accept the connection for Player 1
    if ((new_socket1 = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed for player 1");
        exit(EXIT_FAILURE);
    }
    printf("Player 1 connected!\n");

    // Accept the connection for Player 2
    if ((new_socket2 = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed for player 2");
        exit(EXIT_FAILURE);
    }
    printf("Player 2 connected!\n");

    char buffer[1024] = {0};
    bool playAgain = true;  // Variable to control the loop for replaying games

    while (playAgain) {
        resetBoard();  // Reset the board before each new game
        int playerTurn = 1;  // Player 1 starts first

        // Main game loop
        while (1) {
            int row, col;
            char symbol = (playerTurn == 1) ? 'X' : 'O';  // Current player's symbol
            int currentSocket = (playerTurn == 1) ? new_socket1 : new_socket2;  // Socket for the current player
            int otherSocket = (playerTurn == 1) ? new_socket2 : new_socket1;  // Socket for the opponent

            sendBoardState(new_socket1, new_socket2);  // Display the board to both players

            // Notify the current player to make a move
            char turnMessage[64];
            snprintf(turnMessage, sizeof(turnMessage), "Player %d's turn (%c)\n", playerTurn, symbol);
            send(currentSocket, turnMessage, strlen(turnMessage), 0);
            send(otherSocket, "Waiting for other player's move...\n", 35, 0);

            // Receive the move from the current player
            memset(buffer, 0, sizeof(buffer));
            recv(currentSocket, buffer, sizeof(buffer), 0);
            sscanf(buffer, "%d %d", &row, &col);  // Parse the row and column values

            // Validate the move (check if the position is valid and not occupied)
            if (row < 0 || row > 2 || col < 0 || col > 2 || board[row][col] != ' ') {
                char *invalidMove = "Invalid move, try again.\n";
                send(currentSocket, invalidMove, strlen(invalidMove), 0);
                continue;  // Invalid move, ask the player to try again
            }

            // Update the board with the valid move
            updateBoard(row, col, symbol);

            // Check if the current move resulted in a win
            if (checkWin()) {
                sendBoardState(new_socket1, new_socket2);  // Update both players on the final board state
                char winMessage[64];
                snprintf(winMessage, sizeof(winMessage), "Player %d Wins!\n", playerTurn);
                send(new_socket1, winMessage, strlen(winMessage), 0);
                send(new_socket2, winMessage, strlen(winMessage), 0);
                break;  // End the game loop after a win
            }

            // Check for a draw
            if (checkDraw()) {
                sendBoardState(new_socket1, new_socket2);
                char *drawMessage = "It's a Draw!\n";
                send(new_socket1, drawMessage, strlen(drawMessage), 0);
                send(new_socket2, drawMessage, strlen(drawMessage), 0);
                break;  // End the game loop after a draw
            }

            // Switch turns between Player 1 and Player 2
            playerTurn = (playerTurn == 1) ? 2 : 1;
        }

        // Ask both players if they want to play again
        bool player1Replay = askPlayAgain(new_socket1);
        bool player2Replay = askPlayAgain(new_socket2);

        // If both players want to replay, start a new game
        if (player1Replay && player2Replay) {
            char *replayMessage = "Both players agreed to play again. Starting new game...\n";
            send(new_socket1, replayMessage, strlen(replayMessage), 0);
            send(new_socket2, replayMessage, strlen(replayMessage), 0);
            playAgain = true;
        }
        // Both players don't want to play agian. 
         else if (!player1Replay && !player2Replay) {
            char *endMessage = "Both players chose not to play again. Ending game...\n";
            send(new_socket1, endMessage, strlen(endMessage), 0);
            send(new_socket2, endMessage, strlen(endMessage), 0);
            playAgain = false;
        } 
        // One player agrees but other one doesn't
        else {
            char *mismatchMessage = "One player wanted to continue, but the other did not. Ending game...\n";
            send(new_socket1, mismatchMessage, strlen(mismatchMessage), 0);
            send(new_socket2, mismatchMessage, strlen(mismatchMessage), 0);
            playAgain = false;
        }
    }
    // close the sockets
    close(new_socket1);
    close(new_socket2);
    // close the server socket
    close(server_fd);
    return 0;
}