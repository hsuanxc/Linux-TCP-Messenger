#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <sys/stat.h>
#define SIZE 1024

void send_file(int sockfd){
    int n, numbytes;
    char data[1000000] = {0};
    char end[0] = {};
    char filename[SIZE];
    int format;
    FILE *fp;
    struct stat filestat;
    // // char *filename = "hello.txt";
    printf("\n[+]Which file do you want to send(only under same diretory)? ");
    scanf("%s", filename);
    send(sockfd, filename, sizeof(filename), 0);
    // printf("\n[+]Which format of file (Enter the number 1.txt  2.mp3)? ");
    // scanf("%d", format);
    // if(format==1)
    //     fp = fopen(filename, "r");
    // else if(format==2)
    //     fp = fopen(filename, "r");
    // if (fp == NULL){
    //     printf("[-]Error in reading file!\n"); 
    //     send(sockfd, "[-]Server failed to send file.", sizeof(data), 0);
    //     return;
    // }
    // send(sockfd, filename, sizeof(filename), 0);
    // while(fgets(data, SIZE, fp) != NULL){
    //     if (send(sockfd, data, sizeof(data), 0) == -1) {
    //         printf("[-]Error in sending file.");
    //         return;
    //     }
    //     bzero(data, SIZE);
    // }
    
    //Get file stat
	if ( lstat(filename, &filestat) < 0){
		return;
	}
	printf("The file size is %lu\n", filestat.st_size);
	fp = fopen(filename, "rb");
	//Sending file
	while(!feof(fp)){
        bzero(data, 1024);
        strcpy(data, "");
        send(sockfd, data, strlen(data), 0);
        // printf("data= %s\n", data);
		numbytes = fread(data, sizeof(char), sizeof(data), fp);
		// printf("fread %d bytes, ", numbytes);
		numbytes = write(sockfd, data, numbytes);
		// printf("Sending %d bytesn\n",numbytes);
	}
    
    printf("[+]Data send to client successfully.\n");
    printf("[+]Please enter Enter to continue...\n\n");
}


void write_file(int sockfd){
    int numbytes;
    int n;
    FILE *fp;
    char filename[SIZE];
    char buffer[1000000];
    bzero(buffer, SIZE);
    bzero(filename, SIZE);
    recv(sockfd, filename, SIZE, 0);
    printf("filename: %s\n",filename);
 
    //Open file
	if ( (fp = fopen(filename, "wb")) == NULL){
		perror("fopen");
		return;
	}
	//Receive file from server
	while(1){
            numbytes = read(sockfd, buffer, sizeof(buffer));
            // printf("read %d bytes, ", numbytes);
            if(numbytes == 1){
                break;
            }
            numbytes = fwrite(buffer, sizeof(char), numbytes, fp);
            // printf("fwrite %d bytes\n", numbytes);
	}
    fclose(fp);
    return;
}

char* findmyip(){
	char hostbuffer[256];
    char *ip;
    struct hostent *host_entry;
    int hostname;
    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
	if (hostname == -1){
        perror("gethostname");
        exit(1);
    }
    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
	if (host_entry == NULL){
        perror("gethostbyname");
        exit(1);
    }
    // To convert an Internet network address into ASCII string
    ip = inet_ntoa(*((struct in_addr*)
                           host_entry->h_addr_list[0]));
	if (NULL == ip){
        perror("inet_ntoa");
        exit(1);
    }
	return ip;
}

int main(){

    while(1){

        int n, sock;
        char buffer[1024];
        char server_ip[16];
        char *client_ip = findmyip();
        char *leave_msg = "***Client leave the room.***";
        struct pollfd pfds[2];
        struct sockaddr_in addr;
        socklen_t addr_size;
        
        
        printf("*********************************************************************\n");
        printf("\t\t\tClient IP: %s\n", client_ip);
        printf("\t\t\tServer IP: ");
        scanf("%s", server_ip);
        printf("*********************************************************************\n");

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0){
            system("clear");
            perror("[-]Socket error");
            continue;
        }
        printf("[+]TCP server socket created.\n");

        memset(&addr, '\0', sizeof(addr)); //getrid of last end character
        addr.sin_family = AF_INET;
        addr.sin_port = 8080;
        addr.sin_addr.s_addr = inet_addr(server_ip);

        n = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
        if(n == -1){
            system("clear");
            printf("[-]Error in connection!\n");
            continue;
        }
        
        printf("[+]Connected to the server.\n");
        printf("+=============================Chat Room=============================+\n");
        printf("*********************************************************************\n");
        printf("**     User Guide:   [!x]->exit chat room      [!f]->send file     **\n");
        printf("*********************************************************************\n");
        // chat mode
        pfds[0].fd = STDIN_FILENO;
        pfds[0].events = POLLIN;
        pfds[1].fd = sock;
        pfds[1].events = POLLRDNORM;
        while(poll(pfds, 2, 1000) != -1) { /* error handling elided */
            if(pfds[0].revents & POLLIN) {
                // read data from stdin and send it over the socket
                bzero(buffer, 1024);
                // printf("[Server %s]: %s", server_ip, buffer);
                fgets(buffer, 1024, stdin);
                if(strncmp(buffer,"!x",2)==0){
                    send(sock, buffer, strlen(buffer), 0);
                    shutdown(sock, SHUT_RDWR);
                    break;
                }
                else if(strncmp(buffer,"!f",2)==0){
                    send(sock, buffer, strlen(buffer), 0);
                    bzero(buffer, 1024);
                    printf("\n[+]Ready to send file...\n");
                    send_file(sock);
                }
                send(sock, buffer, strlen(buffer), 0); // repeat as necessary.
            }
            if(pfds[1].revents & POLLRDNORM) {
                // chat data received
                bzero(buffer, 1024);
                n = recv(sock, buffer, sizeof(buffer), 0);
                if(strncmp(buffer,"!x",2) == 0){
                    printf("\n[-]Server disconnect!\n");
                    // printf("[Server %s]: ***Server leave the room.***\n\n", client_ip);
                    shutdown(sock, SHUT_RDWR);
                    break;
                }
                else if(strncmp(buffer,"!f",2) == 0){
                    printf("\n[+]Ready to recieve file...\n");
                    write_file(sock);
                    printf("[+]Data written in the file successfully.\n\n");
                }
                else if(n>0){
                        printf("[Server %s]: ", client_ip);
                        printf("%s", buffer);
                }
            }
            if(pfds[1].revents & (POLLERR | POLLHUP)) {
                // socket was closed
            }
        }
        printf("+===========================Disconnected!===========================+\n\n\n\n\n");
        shutdown(sock, SHUT_RDWR);
    }
    return 0;
}


// void send_file(int sockfd){
//     int n;
//     char data[SIZE] = {0};
//     char filename[SIZE];
//     FILE *fp;
//     // char *filename = "hello.txt";
//     printf("\n[+]Which file do you want to send(only under same diretory)? ");
//     scanf("%s", filename);
//     fp = fopen(filename, "r");
//     if (fp == NULL){
//         printf("[-]Error in reading file!\n"); 
//         send(sockfd, "[-]Server failed to send file.", sizeof(data), 0);
//         return;
//     }
//     send(sockfd, filename, sizeof(filename), 0);
//     while(fgets(data, SIZE, fp) != NULL){
//         if (send(sockfd, data, sizeof(data), 0) == -1) {
//             printf("[-]Error in sending file.");
//             return;
//         }
//         bzero(data, SIZE);
//     }
//     bzero(data, 1024);
//     strcpy(data, "!fileEnd");
//     send(sockfd, data, strlen(data), 0);
//     printf("[+]Data send to client successfully.\n\n");
// }