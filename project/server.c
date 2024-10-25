#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
    /* 1. Create socket */
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                     // use IPv4  use UDP

    /* 2. Construct our address */
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET; // use IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY; // accept all connections
                            // same as inet_addr("0.0.0.0") 
                                     // "Address string to network bytes"
    // Set receiving port
    int PORT = atoi(argv[1]);
    servaddr.sin_port = htons(PORT); // Big endian

    /* 3. Let operating system know about our config */
    int did_bind = bind(sockfd, (struct sockaddr*) &servaddr, 
                        sizeof(servaddr));
    // Error if did_bind < 0 :(
    if (did_bind < 0) return errno;
    
    // 3a. Make socket nonblocking
   
    int flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) < 0) {
        return errno;
    }

    // 3b. Make STDIN nonblocking
    flags = fcntl(STDIN_FILENO, F_GETFL);
    flags |= O_NONBLOCK;
    if(fcntl(STDIN_FILENO, F_SETFL, flags) < 0) {
      return errno;
    }

    printf("Both socket and stdin are nonblocking \n");

    /* 4. Create buffer to store incoming data */
    int BUF_SIZE = 1024;    
    // keep operating
    int client_connected = 0;

    while(1){
         /* 5. Listen for data from clients */
      char client_buf[BUF_SIZE];
      struct sockaddr_in clientaddr; // Same information, but about client
      socklen_t clientsize = sizeof(clientaddr);

      int bytes_recvd = recvfrom(sockfd, client_buf, BUF_SIZE, 
                              // socket  store data  how much
                                 0, (struct sockaddr*) &clientaddr, 
                                 &clientsize);
      // Execution will stop here until `BUF_SIZE` is read or termination/error
      // Error if bytes_recvd < 0 :(
      if (bytes_recvd < 0){
         if((errno==EAGAIN || errno==EWOULDBLOCK)){
            if(!client_connected) continue;
         }else{
            perror("recv_from");
            return errno;
         }
      }

      client_connected = 1;

      if(bytes_recvd >= 0){
         // 6. Inspect data from client
         char* client_ip = inet_ntoa(clientaddr.sin_addr);
                        // "Network bytes to address string"
         int client_port = ntohs(clientaddr.sin_port); // Little endian
         // Print out data
         write(1, client_buf, bytes_recvd);
      }
      

      char server_buf[1024];

      int bytes_read = read(STDIN_FILENO, server_buf, 1024);
      if(bytes_read <= 0){
         if (errno==EAGAIN || errno==EWOULDBLOCK){
            continue;
         }else{
            perror("Reading from stdin");
            return errno;
         }
      }

      if(bytes_read > 0){
         int did_send = sendto(sockfd, server_buf, bytes_read, 
                        // socket  send data   how much to send
                           0, (struct sockaddr*) &clientaddr, 
                        // flags   where to send
                           sizeof(clientaddr));
         if (did_send < 0){
            if (errno==EAGAIN || errno==EWOULDBLOCK){
               continue;
            }else{
               perror("Sending to client");
               return errno;
            }
         }
         write(STDERR_FILENO,  "send to client:\n", 16);
         write(STDERR_FILENO, server_buf, bytes_read);
      }
   
/* 
      // 7. Send data back to client 
      char server_buf[] = "Hello world!";
      int did_send = sendto(sockfd, server_buf, strlen(server_buf), 
                        // socket  send data   how much to send
                           0, (struct sockaddr*) &clientaddr, 
                        // flags   where to send
                           sizeof(clientaddr));
      if (did_send < 0) return errno;
*/
    }
    

    /* 8. You're done! Terminate the connection */     
    close(sockfd);
    return 0;
}