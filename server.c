#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#define MAX_CONN 3

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

int read_questions(struct Entry* arr, char* filename) {
    int num_questions=0; 
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Cannot open file");
        exit(EXIT_FAILURE);
    }
    char question[1024];
    char option[160];
    char ans[50]; 
    char empty[10];
    while(fgets(question, 1024, file)){
        num_questions++; 
        if(question[strlen(question)-1] == '\n'){
            question[strlen(question)-1] = 0; 
        }
        strcpy(arr[num_questions-1].prompt, question);
        fgets(option, 160, file);
        if(option[strlen(option)-1] == '\n'){
            option[strlen(option)-1] = 0; 
        }
        int i = 0; 
        for(char* token = strtok(option, " "); token != NULL; token = strtok(NULL, " ")){
            strcpy((arr[num_questions-1].options)[i], token);
            i++; 
        }
        fgets(ans, sizeof(ans), file); 
        if(ans[strlen(ans)-1] == '\n'){
            ans[strlen(ans)-1] = 0; 
        }
        int j; 
        for(j=0; j<3; j++){
            if(strcmp((arr[num_questions-1].options)[j],ans)==0){
                arr[num_questions-1].answer_idx = j; 
            }
        }
        if(fgets(empty,sizeof(empty), file) == NULL){
            break; 
        }
    }
    return num_questions; 
}

int prep_fds(fd_set* active_fds, int server_fd, int* client_fds ) {
    FD_ZERO(active_fds);
    FD_SET(server_fd, active_fds);
    int max_fd = server_fd;
    for (int i = 0; i < MAX_CONN; i ++) {
        if (client_fds[i] > -1)
            FD_SET(client_fds[i], active_fds);
        if (client_fds[i] > max_fd)
            max_fd = client_fds[i];
    }
    return max_fd;
}

int main(int argc, char* argv[]){
    char* question_file = NULL;
    char* IP_address = NULL;
    int port_num = -1; 
    int option; 
    while((option = getopt(argc, argv, ":f:i:p:h")) != -1){
        switch(option){
            case 'f': 
                if(optarg == NULL){
                    fprintf(stderr, "Option '-%c' requires a file argument.\n", optopt);
                    exit(EXIT_FAILURE); 
                }
                question_file = optarg; 
                break; 
            case 'i': 
                if(optarg == NULL){
                    fprintf(stderr, "Option '-%c' requires a file argument.\n", optopt);
                    exit(EXIT_FAILURE);  
                }
                IP_address = optarg; 
                break;
            case 'p': 
                if(optarg == NULL){
                    fprintf(stderr, "Option '-%c' requires a file argument.\n", optopt);
                    exit(EXIT_FAILURE); 
                }
                port_num = atoi(optarg);
                if (port_num == 0 || port_num > 49151 || port_num < 1024) {
                    fprintf(stderr, "Invalid port number.\n");
                    exit(EXIT_FAILURE); 
                }
                break; 
            case 'h': 
                printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n", argv[0]);
                printf("  -f question_file  Default to \"questions.txt\";\n");
                printf("  -i IP_address     Default to \"127.0.0.1\";\n");
                printf("  -p port_number    Default to 25555;\n");
                printf("  -h                Display this help info.\n");
                exit(EXIT_SUCCESS); 
            case '?':
                fprintf(stderr, "Error: Unknown option '-%c' received.", optopt);
                exit(EXIT_FAILURE); 
        }
    }
    if(question_file == NULL){
        question_file = "questions.txt";
    }
    if(IP_address == NULL){
        IP_address = "127.0.0.1";
    }
    if(port_num == -1){
        port_num = 25555; 
    }

    struct Entry array[50]; 
    int array_length = read_questions(array, question_file);  
    int question_index = 0; 

    int    server_fd;
    int    client_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(in_addr);

    /* STEP 1
        Create and set up a socket
    */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd==-1){
        perror("socket() failed");
        exit(EXIT_FAILURE); 
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port_num);
    server_addr.sin_addr.s_addr = inet_addr(IP_address);

    /* STEP 2
        Bind the file descriptor with address structure
        so that clients can find the address
    */
    int bind1 = bind(server_fd,
            (struct sockaddr *) &server_addr,
            sizeof(server_addr));
    
    if (bind1 == -1){
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    /* STEP 3
        Listen to at most 3 incoming connections
    */
    if (listen(server_fd, MAX_CONN) == 0)
        printf("Welcome to 392 Trivia!\n");
    else perror("listen");

    fd_set active_fds, write_fds; 
    int client_fds[MAX_CONN]; 
    int nconn = 0;
    for (size_t i = 0; i < MAX_CONN; i ++) client_fds[i] = -1;

    /* STEP 4
        Accept connections from clients
        to enable communication
    */
    /*client_fd  =   accept(server_fd,
                            (struct sockaddr*)&in_addr,
                            &addr_size);*/

    char* receipt   = "Read\n";
    int   recvbytes = 0;
    char  buffer[1024];
    struct Player contestants[MAX_CONN]; 
    for(int i=0; i<MAX_CONN;i++){
        contestants[i].fd = client_fds[i];
        contestants[i].score = 0; 
    }
    int max_conn_iters=1;
    while(1) {
        int max_fd = prep_fds(&active_fds, server_fd, client_fds);
        int fail = select(max_fd + 1, &active_fds, NULL, NULL, NULL);
        if(fail==-1){
            perror("select() failed");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(server_fd, &active_fds)) {
            int in_fd = accept(server_fd, (struct sockaddr*) &in_addr, &addr_size);
            if(in_fd==-1){
                perror("accept() failed");
                continue; 
            }
            if (nconn == MAX_CONN) {
                close(in_fd);
                fprintf(stderr, "Max connection reached!\n");
            }
            else {
                int i;
                for (i = 0; i < MAX_CONN; i++) {
                    if (client_fds[i] == -1) {
                        client_fds[i] = in_fd;
                        contestants[i].fd = client_fds[i];
                        break;
                    }
                }
                char* intro = "Please type your name: ";
                char name[128]; 
                int recvbytes;
                printf("New connection detected!\n");
                nconn++;
                int send_bytes;
                send_bytes = send(in_fd, intro, strlen(intro),0);
                if(send_bytes == -1){
                    perror("send() from server failed"); 
                    exit(EXIT_FAILURE);
                }
                if((recvbytes = recv(in_fd, name, 128, 0)) == -1){
                    perror("recv() from server failed"); 
                    exit(EXIT_FAILURE);                    
                }
                name[recvbytes] = 0; 
                strcpy(contestants[nconn-1].name, name); 
                printf("Hi %s!\n", name); 

            }
        } else if (nconn == MAX_CONN){
            //select(max_fd + 1, NULL, &write_fds, NULL, NULL);
            int answer_index = array[question_index].answer_idx;
            //select(max_fd + 1, &active_fds, &write_fds, NULL, NULL);
            for (int i = 0; i < MAX_CONN; i ++) {
                if (FD_ISSET(client_fds[i], &active_fds)) {
                    char buffer[1024];
                    int bytes = recv(client_fds[i], buffer, 1024,0);
                    if (bytes == 0) {
                        close(client_fds[i]);
                        FD_CLR(client_fds[i], &active_fds);
                        client_fds[i] = -1;
                        printf("Lost connection!\n");
                        nconn--;
                        fprintf(stderr, "game interrupted");
                        for(int i=0; i<MAX_CONN; i++){
                            if(client_fds[i] != -1){
                                close(client_fds[i]);
                            }
                        }
                        close(server_fd);
                        exit(EXIT_FAILURE);
                    } else {
                        buffer[bytes] = 0; 
                        int response= atoi(buffer); 
                        if(response==0){
                            char* error = "Send an integer";
                            puts(error);
                        } else if ((response-1)==answer_index){
                            contestants[i].score++;
                        } else {
                            contestants[i].score--; 
                        }
                        char correct_answer[2048]; 
                        sprintf(correct_answer, "Correct Answer is: %s\n", array[question_index].options[(array[question_index].answer_idx)]);
                        printf("Score: ");
                        for (int i = 0; i < MAX_CONN; i ++) {
                            printf("%s: %d/", contestants[i].name, contestants[i].score);
                            int s_bytes = send(client_fds[i],correct_answer,strlen(correct_answer),0);
                            if(s_bytes == -1){
                                perror("send() failed");
                                exit(EXIT_FAILURE);
                            }
                        }
                        printf("\n");
                        fflush(stdout);
                    }
                }
            }
            if((question_index+1) == array_length){
                int max_score = -1;
                for(int i=0; i<MAX_CONN; i++){
                    if(contestants[i].score>max_score){
                        max_score = contestants[i].score; 
                    } 
                }
                for(int i=0; i<MAX_CONN; i++){
                    if(contestants[i].score == max_score){
                        printf("Congrats, %s!\n", contestants[i].name);
                    }
                }
                break; 
            }
            question_index++;
            if (max_conn_iters>1){
                if(question_index==0){
                    printf("The game starts now!\n");
                }
                char client_message[2048]; 
                sprintf(client_message, "Question %d: %s\nPress 1: %s\nPress 2: %s\nPress 3: %s\n", question_index+1, array[question_index].prompt, array[question_index].options[0],array[question_index].options[1],array[question_index].options[2]);
                printf("Question %d: %s\n1: %s\n2: %s\n3: %s\n",  question_index+1, array[question_index].prompt, array[question_index].options[0],array[question_index].options[1],array[question_index].options[2]);
                fflush(stdout);
                for(int i=0; i<MAX_CONN; i++){
                    int s_bytes = send(client_fds[i],client_message, strlen(client_message),0);
                    if(s_bytes == -1){
                        perror("send() failed");
                        exit(EXIT_FAILURE);
                    }
                }
                max_conn_iters++; 
            }
        }
        if (nconn == MAX_CONN && max_conn_iters==1){
            if(question_index==0){
                printf("The game starts now!\n");
            }
            char client_message[2048]; 
            sprintf(client_message, "Question %d: %s\nPress 1: %s\nPress 2: %s\nPress 3: %s\n", question_index+1, array[question_index].prompt, array[question_index].options[0],array[question_index].options[1],array[question_index].options[2]);
            printf("Question %d: %s\n1: %s\n2: %s\n3: %s\n",  question_index+1, array[question_index].prompt, array[question_index].options[0],array[question_index].options[1],array[question_index].options[2]);
            fflush(stdout);
            for(int i=0; i<MAX_CONN; i++){
                int s_bytes = send(client_fds[i],client_message, strlen(client_message),0);
                if(s_bytes==-1){
                    perror("send() failed");
                    exit(EXIT_FAILURE);
                }
            }
            max_conn_iters++; 
        }
    }

    for(int i=0; i<MAX_CONN; i++){
        if(client_fds[i] != -1){
            close(client_fds[i]);
        }
    }

    close(server_fd);

    return 0;
}