#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
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
    printf("\n[+]Which file do you want to send(only under same diretory)? ");
    scanf("%s", filename);
    send(sockfd, filename, sizeof(filename), 0);

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



int main(){
    while(1){

        int n;
        int PORT = 8080;
        int server_sock, client_sock;
        char buffer[1024];
        char server_ip[16];
        char *client_ip;
        char *filename = "hello.txt";
        struct pollfd pfds[2];
        struct sockaddr_in server_addr, client_addr;
        FILE *fp;
        socklen_t addr_size;
        addr_size = sizeof(client_addr);
        
        printf("*********************************************************************\n");
        printf("\t\t\tServer IP: ");
        scanf("%s", server_ip);
        printf("*********************************************************************\n");

        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock < 0){
            system("clear");
            perror("[-]Socket error");
            continue;
        }
        printf("[+]TCP server socket created.\n");

        memset(&server_addr, '\0', sizeof(server_addr)); //getrid of last end character
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = PORT;
        server_addr.sin_addr.s_addr = inet_addr(server_ip);
        
        n = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (n < 0){
            system("clear");
            perror("[-]Bind error");
            continue;
        }
        printf("[+]Bind to the port number: %d\n", PORT);

        listen(server_sock, 5);
        printf("[+]Listening...\n");
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        client_ip = inet_ntoa(client_addr.sin_addr);
        printf("[+]Client connected.\n");
        
        printf("+=============================Chat Room=============================+\n");
        printf("*********************************************************************\n");
        printf("**     User Guide:   [!x]->exit chat room      [!f]->send file     **\n");
        printf("*********************************************************************\n");
        // chat mode
        pfds[0].fd = STDIN_FILENO;
        pfds[0].events = POLLIN;
        pfds[1].fd = client_sock;
        pfds[1].events = POLLRDNORM;
        while(poll(pfds, 2, 10) != -1) { /* error handling elided */
            if(pfds[0].revents & POLLIN) {
                // read data from stdin and send it over the socket
                bzero(buffer, 1024);
                fgets(buffer, 1024, stdin);
                if(strncmp(buffer,"!x",2)==0){
                    send(client_sock, buffer, strlen(buffer), 0);
                    shutdown(client_sock, SHUT_RDWR);
                    break;
                }
                else if(strncmp(buffer,"!f",2)==0){
                    send(client_sock, buffer, strlen(buffer), 0);
                    bzero(buffer, 1024);
                    printf("\n[+]Ready to send file...\n");
                    send_file(client_sock);
                    send(client_sock, 0, sizeof(0), 0);
                }
                send(client_sock, buffer, strlen(buffer), 0); // repeat as necessary.
            }
            if(pfds[1].revents & POLLRDNORM) {
                // chat data received
                bzero(buffer, 1024);
                n = recv(client_sock, buffer, sizeof(buffer), 0);
                if(strncmp(buffer,"!x",2) == 0){
                    printf("\n[-]Client disconnect!\n");
                    // printf("[Client %s]: ***Client leave the room.***\n", client_ip);
                    shutdown(client_sock, SHUT_RDWR);
                    break;
                }
                else if(strncmp(buffer,"!f",2) == 0){
                    printf("\n[+]Ready to recieve file...\n");
                    write_file(client_sock);
                    printf("[+]Data written in the file successfully.\n\n");
                }
                else if(n>0){
                    printf("[Client %s]: ", client_ip);
                    printf("%s", buffer);
                }
            }
            if(pfds[1].revents & (POLLERR | POLLHUP)) {
                // socket was closed
            }
        }
        printf("+===========================Disconnected!===========================+\n\n\n\n\n");
        shutdown(client_sock, SHUT_RDWR);
    }
    return 0;
}

// void write_file(int sockfd){
//     int n;
//     FILE *fp;
//     char filename[SIZE];
//     char buffer[SIZE];
//     bzero(buffer, SIZE);
//     bzero(filename, SIZE);
//     recv(sockfd, filename, SIZE, 0);
//     fp = fopen(filename, "w");
//     while (1) {
//         n = recv(sockfd, buffer, SIZE, 0);
//         if (n <= 0) return;
//         if(strncmp(buffer,"!fileEnd",9)==0)
//             break;
//         fprintf(fp, "%s", buffer);
//         bzero(buffer, SIZE);
//     }
//     fclose(fp);
//     return;
// }