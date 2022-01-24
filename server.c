#include <stdio.h>
#include <stdlib.h> 
#include <sys/socket.h>
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <signal.h> 

#define MAX_CLIENT 10
#define BUFFER_TAILLE  2040
#define NAME_LEN 1000

static  unsigned int cli_count = 0; 
static int uid = 10; 

static int leave_flag = 0; 

typedef struct{

    struct sockaddr_in address; 
    int sockfd; 
    int uid; 
    char name[NAME_LEN]; 
}client_t; 

client_t *clients[MAX_CLIENT]; 

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout(){
    printf("   ");
    fflush(stdout); 
}

void trim(char *arr , int length){
    for(int i = 0 ; i < length ; i++){
        if(arr[i] == '\n'){
	    arr[i] = ' ';
	    break;  
	}
    }
}

void queue_add(client_t *cl){

    pthread_mutex_lock(&clients_mutex);
    for(int i = 0 ; i < MAX_CLIENT ; i++){
        if(clients[i] == NULL){
	    clients[i] = cl;
	    fflush(stdout); 
	    printf(" APRES L AFFECTATION %p " , clients[i]);  
            break; 	
	}
    }
    pthread_mutex_unlock(&clients_mutex);  
}

void queue_remove(int uid){

    pthread_mutex_lock(&clients_mutex);
    for(int i = 0 ; i < MAX_CLIENT ; i++){
        if(clients[i]){
	    if(clients[i]->uid == uid){
	        clients[i] = NULL; 
	        break;  
	    }
	}
    }

    pthread_mutex_unlock(&clients_mutex);
}

void send_message(char *s , int uid){

    pthread_mutex_lock(&clients_mutex); 	

    for(int i = 0 ; i < MAX_CLIENT ; i++){
        if(clients[i] != NULL &&  clients[i]->uid != uid){
	    if(write(clients[i]->sockfd , s , strlen(s)) < 0){
	        printf(" ERREUR AU NIVEAU DE L ENVOI DE LA CHAINE %s " , s);
	        break; 	
	    } 
	}
    
    }
    pthread_mutex_unlock(&clients_mutex); 	
}

void *handle_client(void *arg){

    char buffer[BUFFER_TAILLE]; 
    char name[NAME_LEN]; 
    int leave , flag = 0;
    cli_count++;
    
    client_t *cli = (client_t*)arg;  
    
    if(recv(cli->sockfd , name , NAME_LEN , 0) <= 0 || strlen(name) < 2 || strlen(name) >= NAME_LEN - 1){
        printf(" Entrez le nom correctement \n");
	leave_flag = 1; 
    }else{
        fflush(stdout); 	
        //trim(name , NAME_LEN);	
        strcpy(cli->name , name);

	sprintf(buffer , " %s a joint le chat \n " , cli->name);
        
        send_message(buffer , cli->uid); 
    }

    bzero(buffer ,  BUFFER_TAILLE);

    while(1){
        
	bzero(buffer , BUFFER_TAILLE); 
	
        if(leave_flag){
	    break; 
	}

        int receive = recv(cli->sockfd , buffer , BUFFER_TAILLE , 0);
        //trim(buffer , BUFFER_TAILLE);  
        if(receive > 0){
            if(strlen(buffer) > 0){
	        send_message(buffer , cli->uid); 
	        printf(" %s -> %s " , buffer , cli->name);
            }	
        }else if(receive == 0 || strcmp(buffer , "exit") == 0){
            sprintf(buffer , " %s has left \n" , cli->name);
            printf("%s" , buffer); 
            send_message(buffer , cli->uid); 
            leave_flag = 1; 	
        }else{
            printf(" ERROR \n"); 
            leave_flag = 1; 
        }

	bzero(buffer , BUFFER_TAILLE); 
    }

    close(cli->sockfd); 
    queue_remove(cli->uid); 
    free(cli); 
    cli_count--; 
    pthread_detach(pthread_self()); 

    return NULL; 
}



int main(void){

   char *ip = "127.0.0.1"; 
   char *tropCli = " Y A TROP DE CLIENT "; 
   int port = 8080; 
   
   int listenfd = 0 , connfd = 0 , option = 0; 
   struct sockaddr_in serv_addr; 
   struct sockaddr_in cli_addr; 
   pthread_t tid; 

   listenfd = socket(AF_INET , SOCK_STREAM , 0); 
   serv_addr.sin_family = AF_INET; 
   serv_addr.sin_addr.s_addr = inet_addr(ip); 
   serv_addr.sin_port = htons(port);

   signal(SIGPIPE , SIG_IGN);  

   if(setsockopt(listenfd , SOL_SOCKET , (SO_REUSEPORT | SO_REUSEADDR) , (char *)&option , sizeof(option)) < 0){
   
       printf(" ERROR : setsockopt "); 
       return EXIT_FAILURE; 
   
   }

   if(bind(listenfd , (struct sockaddr*) &serv_addr , sizeof(serv_addr)) < 0){
       printf(" on peut pas bind "); 
       exit(1);     
   } 

   puts(" ----- BIENVENUE DANS LA ZONE DE CHAT ----- "); 

   if(listen(listenfd , 10) < 0){
       printf(" ERROR : \n "); 
       exit(1); 
   }

   while(1){
   
       socklen_t clilen = sizeof(cli_addr); 
       connfd = accept(listenfd , (struct sockaddr*) &cli_addr , &clilen);

       if( (cli_count + 1) == MAX_CLIENT){
           send(connfd , tropCli , strlen(tropCli)  , 0); 
       }
	
       
       client_t *cli =  malloc(sizeof(client_t));
       if(cli == NULL){
           printf(" ERREUR FATAL AU NIVEAU DE L ALLOCATION DE LA MEMOIRE "); 
           exit(1);  
       }

       cli->address = cli_addr; 
       cli->sockfd = connfd; 
       cli->uid = uid++;

       fflush(stdout); 
       queue_add(cli);
       sleep(5);  
       pthread_create(&tid , NULL , &handle_client , (void*)cli);   
   }

   return EXIT_SUCCESS;
}
