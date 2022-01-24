#include <stdio.h>
#include <stdlib.h> 
#include <sys/socket.h>
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

#define BUFFER_TAILLE 500


/* definition des variables globales   */

pthread_t reception , envoi; 
struct sockaddr_in serveradress , client; 
int sockfd; 

pthread_mutex_t reservationSocket; 

/* def des fonctions   */
void *recvHandling(void *args){
   
    char message[BUFFER_TAILLE];
    int nbRecu; 
    while(1){
        nbRecu = recv(sockfd , message , BUFFER_TAILLE , 0); 
        printf("le message recu  %s " , message); 
    }
}

void *sendHandling(void *args){

    char message[BUFFER_TAILLE];
    int nbRecu;  
    while(1){
      
       puts(" entrez le message que vous souhaitez envoyer : "); 
       fgets(message , BUFFER_TAILLE , stdin);
       nbRecu = send(sockfd , message , BUFFER_TAILLE , 0);
    }

}



int main(void){

    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    int connection = 0; 

    pthread_mutex_unlock(&reservationSocket); 
    serveradress.sin_family = AF_INET; 
    serveradress.sin_port = htons(8080); 
    serveradress.sin_addr.s_addr = inet_addr("127.0.0.1");

    connection = connect(sockfd , (struct sockaddr*)&serveradress , sizeof(serveradress));

    if(connection == -1){
        printf("connection refusee ");
	exit(1); 
    } 

    pthread_create(&envoi , NULL ,   &sendHandling , NULL);
    pthread_create(&reception , NULL ,   &recvHandling , NULL);

    pthread_join(envoi , NULL); 
    pthread_join(reception , NULL);    

    printf(" on sort merci aurevoir "); 
}
