#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>

#define MAX_CONN 3
#define BUFFER_SIZE 2048

struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

struct Player {
    int fd;
    int score;
    char name[128];
};

// Global variables to track questions and player data
struct Entry array[50];
int array_length;
pthread_mutex_t lock;  // Mutex for safe access to global resources
int completed_clients = 0;  // Track how many clients have finished the game
struct Player* winner = NULL;  // Pointer to track the player with the highest score

// Function to read questions from the file
int read_questions(struct Entry* arr, char* filename) {
    int num_questions = 0;
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Cannot open file");
        exit(EXIT_FAILURE);
    }
    char question[1024];
    char option[160];
    char ans[50];
    char empty[10];
    while (fgets(question, 1024, file)) {
        num_questions++;
        if (question[strlen(question) - 1] == '\n') {
            question[strlen(question) - 1] = 0;
        }
        strcpy(arr[num_questions - 1].prompt, question);
        fgets(option, 160, file);
        if (option[strlen(option) - 1] == '\n') {
            option[strlen(option) - 1] = 0;
        }
        int i = 0;
        for (char* token = strtok(option, " "); token != NULL; token = strtok(NULL, " ")) {
            strcpy((arr[num_questions - 1].options)[i], token);
            i++;
        }
        fgets(ans, sizeof(ans), file);
        if (ans[strlen(ans) - 1] == '\n') {
            ans[strlen(ans) - 1] = 0;
        }
        for (int j = 0; j < 3; j++) {
            if (strcmp((arr[num_questions - 1].options)[j], ans) == 0) {
                arr[num_questions - 1].answer_idx = j;
            }
        }
        if (fgets(empty, sizeof(empty), file) == NULL) {
            break;
        }
    }
    fclose(file);
    return num_questions;
}

// Function to handle each client in a separate thread (independent game for each client)
void* handle_client(void* arg) {
    struct Player* player = (struct Player*)arg;
    int client_fd = player->fd;
    char buffer[BUFFER_SIZE];
    int recvbytes;

    // Get the player's name
    char* intro = "Please type your name: ";
    send(client_fd, intro, strlen(intro), 0);
    if ((recvbytes = recv(client_fd, player->name, 128, 0)) == -1) {
        perror("recv() from client failed");
        close(client_fd);
        pthread_exit(NULL);
    }
    player->name[recvbytes] = 0;
    printf("Hi %s!\n", player->name);

    // Loop through all questions for this client
    for (int current_question_index = 0; current_question_index < array_length; current_question_index++) {
        // Send the current question to the client
        char client_message[BUFFER_SIZE];
        sprintf(client_message, "Question %d: %s\nPress 1: %s\nPress 2: %s\nPress 3: %s\n",
                current_question_index + 1, array[current_question_index].prompt,
                array[current_question_index].options[0],
                array[current_question_index].options[1],
                array[current_question_index].options[2]);

        send(client_fd, client_message, strlen(client_message), 0);

        // Receive the client's answer
        if ((recvbytes = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
            buffer[recvbytes] = 0;
            int response = atoi(buffer);

            // Check if the response is correct and update the player's score
            if ((response - 1) == array[current_question_index].answer_idx) {
                player->score++;
            } else {
                player->score--;
            }

            // Send the correct answer back to the client
            char correct_answer[BUFFER_SIZE];
            sprintf(correct_answer, "Correct Answer is: %s\n", array[current_question_index].options[array[current_question_index].answer_idx]);
            send(client_fd, correct_answer, strlen(correct_answer), 0);

            // Display the player's score to them
            char score_message[BUFFER_SIZE];
            sprintf(score_message, "Your score: %d\n", player->score);
            send(client_fd, score_message, strlen(score_message), 0);
        }
    }

    // Once all questions are answered, print final score to the client
    char final_score[BUFFER_SIZE];
    sprintf(final_score, "Game over! Your final score: %d\n", player->score);
    send(client_fd, final_score, strlen(final_score), 0);

    // Update the winner after the game ends
    pthread_mutex_lock(&lock);
    completed_clients++;
    if (winner == NULL || player->score > winner->score) {
        winner = player;
    }
    pthread_mutex_unlock(&lock);

    // Close the connection
    close(client_fd);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    char* question_file = NULL;
    char* IP_address = NULL;
    int port_num = -1;
    int option;
    
    // getopt logic remains the same
    while ((option = getopt(argc, argv, ":f:i:p:h")) != -1) {
        switch (option) {
            case 'f':
                question_file = optarg;
                break;
            case 'i':
                IP_address = optarg;
                break;
            case 'p':
                port_num = atoi(optarg);
                break;
            case 'h':
                printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n", argv[0]);
                exit(EXIT_SUCCESS);
        }
    }
    if (question_file == NULL) question_file = "questions.txt";
    if (IP_address == NULL) IP_address = "127.0.0.1";
    if (port_num == -1) port_num = 25555;

    // Read questions from the file
    array_length = read_questions(array, question_file);
    
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    // Create the server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
    server_addr.sin_addr.s_addr = inet_addr(IP_address);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CONN) == -1) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on %s:%d...\n", IP_address, port_num);

    pthread_mutex_init(&lock, NULL);
    
    // Accept clients and create threads for each client
    struct Player players[MAX_CONN];
    int nconn = 0;
    while (nconn < MAX_CONN) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
        if (client_fd == -1) {
            perror("accept() failed");
            continue;
        }
        printf("New client connected\n");

        players[nconn].fd = client_fd;
        players[nconn].score = 0;

        // Create a new thread to handle the client
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, (void*)&players[nconn]);
        pthread_detach(thread);

        nconn++;
    }

    // Wait for all clients to finish their game
    while (completed_clients < MAX_CONN) {
        sleep(1);  // Check periodically
    }

    // After all clients are done, print the winner and end the server
    pthread_mutex_lock(&lock);
    if (winner != NULL) {
        printf("The winner is %s with a score of %d!\n", winner->name, winner->score);
    } else {
        printf("No winner.\n");
    }
    pthread_mutex_unlock(&lock);

    close(server_fd);
    pthread_mutex_destroy(&lock);

    return 0;
}
