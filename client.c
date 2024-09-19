#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

char* IP_address = NULL;
int port_num = -1; 
void parse_connect(int argc, char** argv, int* server_fd) {
    int option; 
    while((option = getopt(argc, argv, ":i:p:h")) != -1){
        switch(option){
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
                printf("  -i IP_address     Default to \"127.0.0.1\";\n");
                printf("  -p port_number    Default to 25555;\n");
                printf("  -h                Display this help info.\n");
                exit(EXIT_SUCCESS); 
            case '?':
                fprintf(stderr, "Error: Unknown option '-%c' received.", optopt);
                exit(EXIT_FAILURE);  
        }
    }

    if(IP_address == NULL){
        IP_address = "127.0.0.1";
    }
    if(port_num == -1){
        port_num = 25555; 
    }
}

int main(int argc, char* argv[]){
    int  server_fd;
    parse_connect(argc,argv, &server_fd);
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(in_addr);

    /* STEP 1
        Create and set up a socket
    */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1){
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port_num);
    server_addr.sin_addr.s_addr = inet_addr(IP_address);

    int connect1 = connect(server_fd, (struct sockaddr *) &server_addr, addr_size);
    if(connect1 == -1){
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }
    fd_set read_fds;
    char name[160];
    int bytes = recv(server_fd, name, 160, 0);
    if(bytes==0){
        printf("Server closed the connection");
        exit(EXIT_SUCCESS);
    } else if (bytes<0){
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }
    name[bytes] = 0; 
    printf("%s", name); fflush(stdout);
    char buffer[1024];
    scanf("%s", buffer);
    bytes = send(server_fd, buffer, strlen(buffer), 0);
    if(bytes==0){
        printf("Server closed the connection");
        exit(EXIT_SUCCESS);
    } else if (bytes<0){
        perror("send() failed");
        exit(EXIT_FAILURE);
    }       
    while(1){
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        int fail = select(server_fd+1, &read_fds, NULL, NULL, NULL);
        if(fail == -1){
            perror("client select() failed");
            exit(EXIT_FAILURE); 
        }
        if (FD_ISSET(STDIN_FILENO, &read_fds)){
            char correct_answer[1024];
            scanf("%s", correct_answer);
            fflush(stdin);
            int send_bytes = send(server_fd, correct_answer, strlen(correct_answer),0);
            if(send_bytes<0){
                perror("send() failed");
                exit(EXIT_FAILURE);
            }
        } 
        if(FD_ISSET(server_fd, &read_fds)){
            char correct_answer[2048];
            int bytes = recv(server_fd, correct_answer, 2048,0);
            if(bytes==0){
                printf("Server closed the connection");
                exit(EXIT_SUCCESS);
            } else if (bytes<0){
                perror("recv() failed");
                exit(EXIT_FAILURE);
            } else {
                correct_answer[bytes] = '\0';
                printf("%s", correct_answer); fflush(stdout);
            }            
        } 
    }
    return 0; 
}