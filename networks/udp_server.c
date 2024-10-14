#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define PORT 8080
#define BUFFER_SIZE 1024

char board[3][3]; // Tic-Tac-Toe board (3x3 grid)
int currentPlayer = 1; // 1 for 'X', 2 for 'O'

// Function to reset the board to an empty state
void resetBoard() {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            board[i][j] = ' ';
}

// Function to check if a player has won
// Returns 1 if a player has won, otherwise returns 0
int checkWin() {
    // Check rows and columns for a win
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return 1;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return 1;
    }
    // Check diagonals for a win
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ')
        return 1;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
        return 1;
    return 0;
}

// Function to check if the game is a draw (i.e., no empty spaces left)
// Returns 1 if it's a draw, otherwise returns 0
int checkDraw() {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (board[i][j] == ' ')
                return 0;
    return 1;
}

// Function to update the board with the player's move
void updateBoard(int row, int col, char symbol) {
    board[row][col] = symbol;
}

// Function to send the current board state to both players
void sendBoardState(int sockfd, struct sockaddr_in *player1, struct sockaddr_in *player2) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message),
             "\nCurrent board state:\n %c | %c | %c\n---|---|---\n %c | %c | %c\n---|---|---\n %c | %c | %c\n",
             board[0][0], board[0][1], board[0][2], board[1][0], board[1][1], board[1][2], board[2][0], board[2][1], board[2][2]);
    // Send the board state to both players
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)player1, sizeof(*player1));
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)player2, sizeof(*player2));
}

// Function to ask a player if they want to play again
// Returns true if the player chooses "yes", otherwise false
bool askPlayAgain(int sockfd, struct sockaddr_in *player) {
    char message[] = "Do you want to play again? (yes/no): ";
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)player, sizeof(*player));

    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(struct sockaddr_in);
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)player, &addr_len);

    return (strncmp(buffer, "yes", 3) == 0);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, player1_addr, player2_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address and bind the socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("UDP Server is running. Waiting for players...\n");

    // Wait for Player 1 to connect
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&player1_addr, &addr_len);
    printf("Player 1 connected\n");
    sendto(sockfd, "You are Player 1 (X). Waiting for Player 2...\n", 45, 0, (struct sockaddr *)&player1_addr, sizeof(player1_addr));

    // Wait for Player 2 to connect
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&player2_addr, &addr_len);
    printf("Player 2 connected\n");
    sendto(sockfd, "You are Player 2 (O). Game is starting...\n", 41, 0, (struct sockaddr *)&player2_addr, sizeof(player2_addr));
    sendto(sockfd, "Game is starting. You are X.\n", 29, 0, (struct sockaddr *)&player1_addr, sizeof(player1_addr));

    // Main game loop to handle multiple rounds
    bool playAgain = true;
    while (playAgain) {
        resetBoard(); // Initialize a new game board
        int playerTurn = 1; // Player 1 starts first

        while (1) {
            int row, col;
            char symbol = (playerTurn == 1) ? 'X' : 'O'; // Choose the correct symbol for the current player
            struct sockaddr_in *currentPlayer = (playerTurn == 1) ? &player1_addr : &player2_addr;
            struct sockaddr_in *otherPlayer = (playerTurn == 1) ? &player2_addr : &player1_addr;

            sendBoardState(sockfd, &player1_addr, &player2_addr); // Send board state to both players

            // Notify the current player to make a move
            char turnMessage[64];
            snprintf(turnMessage, sizeof(turnMessage), "Your turn (%c). Enter row and column (0-2):\n", symbol);
            sendto(sockfd, turnMessage, strlen(turnMessage), 0, (struct sockaddr *)currentPlayer, sizeof(*currentPlayer));
            // Notify the other player to wait
            sendto(sockfd, "Waiting for other player's move...\n", 35, 0, (struct sockaddr *)otherPlayer, sizeof(*otherPlayer));

            // Receive the current player's move (row and column)
            memset(buffer, 0, BUFFER_SIZE);
            recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)currentPlayer, &addr_len);
            sscanf(buffer, "%d %d", &row, &col);

            // Validate the move
            if (row < 0 || row > 2 || col < 0 || col > 2 || board[row][col] != ' ') {
                char *invalidMove = "Invalid move, try again.\n";
                sendto(sockfd, invalidMove, strlen(invalidMove), 0, (struct sockaddr *)currentPlayer, sizeof(*currentPlayer));
                continue; // Ask for another move
            }

            updateBoard(row, col, symbol); // Update the board with the player's move

            // Check if the current player has won
            if (checkWin()) {
                sendBoardState(sockfd, &player1_addr, &player2_addr);
                char winMessage[64];
                snprintf(winMessage, sizeof(winMessage), "Player %d Wins!\n", playerTurn);
                sendto(sockfd, winMessage, strlen(winMessage), 0, (struct sockaddr *)&player1_addr, sizeof(player1_addr));
                sendto(sockfd, winMessage, strlen(winMessage), 0, (struct sockaddr *)&player2_addr, sizeof(player2_addr));
                break; // End the game
            }

            // Check if the game is a draw
            if (checkDraw()) {
                sendBoardState(sockfd, &player1_addr, &player2_addr);
                char *drawMessage = "It's a Draw!\n";
                sendto(sockfd, drawMessage, strlen(drawMessage), 0, (struct sockaddr *)&player1_addr, sizeof(player1_addr));
                sendto(sockfd, drawMessage, strlen(drawMessage), 0, (struct sockaddr *)&player2_addr, sizeof(player2_addr));
                break; // End the game
            }

            // Switch turns to the other player
            playerTurn = (playerTurn == 1) ? 2 : 1;
        }

        // Ask both players if they want to play again
        bool player1Replay = askPlayAgain(sockfd, &player1_addr);
        bool player2Replay = askPlayAgain(sockfd, &player2_addr);

        // If Both players want to play again
        if (player1Replay && player2Replay) {
            char *replayMessage = "Both players agreed to play again. Starting new game...\n";
            sendto(sockfd, replayMessage, strlen(replayMessage), 0, (struct sockaddr *)&player1_addr, sizeof(player1_addr));
            sendto(sockfd, replayMessage, strlen(replayMessage), 0, (struct sockaddr *)&player2_addr, sizeof(player2_addr));
            playAgain = true;
        } 
        //If both players don't want to play again.
        else if (!player1Replay && !player2Replay) {
            char *endMessage = "Both players chose not to play again. Ending game...\n";
            sendto(sockfd, endMessage, strlen(endMessage), 0, (struct sockaddr *)&player1_addr, sizeof(player1_addr));
            sendto(sockfd, endMessage, strlen(endMessage), 0, (struct sockaddr *)&player2_addr, sizeof(player2_addr));
            playAgain = false;
        } 
        //If one player wants to play and other player doesn't
        else {
            char *mismatchMessage = "One player wanted to continue, but the other did not. Ending game...\n";
            sendto(sockfd, mismatchMessage, strlen(mismatchMessage), 0, (struct sockaddr *)&player1_addr, sizeof(player1_addr));
            sendto(sockfd, mismatchMessage, strlen(mismatchMessage), 0, (struct sockaddr *)&player2_addr, sizeof(player2_addr));
            playAgain = false;
        }
    }
    // close the server socket
    close(sockfd);
    return 0;
}