#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include "list.h"

#define BUFFSIZE 4100

typedef struct {
    int sockfd;
    struct sockaddr* dst_addr;
    socklen_t addrlen;
} destination;

pthread_mutex_t sendMutex;
pthread_mutex_t recMutex;

bool inputTyped = false;
bool messageReceived = false;
bool newInput = true;
bool clientRunning = false;

struct List_s* list1; //list for input from client
struct List_s* list2; //list for message from other client

void* writeMssg();
void* sendMssg(void * args);
void* recvMssg(void *args);
void* printMssg();
void threadInitalizer(destination dest);

int main(int argc, char* argv[]) {

    list1 = List_create();
    list2 = List_create();

    //Get source address info from the args

    int sockfd;
    struct addrinfo hintsSrc;
    memset(&hintsSrc, 0, sizeof(struct addrinfo));
    struct addrinfo *resultsSrc, *record;

    hintsSrc.ai_family = AF_UNSPEC;
    hintsSrc.ai_socktype = SOCK_DGRAM;
    hintsSrc.ai_flags = AI_PASSIVE;
    //hintsSrc.ai_protocol = 0;

    if(argc < 3){
        perror("Usage: ./lets-talk <local port> <remote host> <remote port>\n");
        exit(EXIT_FAILURE);
    }

    if( (getaddrinfo(NULL, argv[1], &hintsSrc, &resultsSrc)) != 0 ){
        perror("Couldn't get source address info");
        exit(EXIT_FAILURE);
    }

    //Get destination address info from the args

    struct addrinfo hintsDst;
    memset(&hintsDst, 0, sizeof(struct addrinfo));
    struct addrinfo *resultsDst;
    destination dest;

    hintsDst.ai_family = AF_INET;
    hintsDst.ai_socktype = SOCK_DGRAM;
    //hintsDst.ai_protocol = 0;

    if( (getaddrinfo(argv[2], argv[3], &hintsDst, &resultsDst)) != 0 ){
        perror("Couldn't get destination info");
        exit(EXIT_FAILURE);
    }

    //Create and bind socket

    for(record = resultsSrc; record != NULL; record = record->ai_next){
        if((sockfd = socket(record->ai_family, record->ai_socktype, record->ai_protocol)) == -1){
            continue;
        }
        if(bind(sockfd, record->ai_addr, record->ai_addrlen) != -1){
            printf("Welcome to the chatroom!\n");
            break;
        }
        close(sockfd);
    }

    if(record == NULL){
        perror("Couldn't create socket");
        exit(EXIT_FAILURE);
    }

    dest.sockfd = sockfd;
    dest.dst_addr = resultsDst->ai_addr;
    dest.addrlen = resultsDst->ai_addrlen;

    freeaddrinfo(resultsSrc);

    clientRunning = true;

    //Spawn threads

    threadInitalizer(dest);

    while (clientRunning){
    }

    printf("Thank you for using our chat service! The count of lists is: %d and %d\n", List_count(list1), List_count(list2));

    pthread_mutex_destroy(&sendMutex);
    pthread_mutex_destroy(&recMutex);
    free(resultsDst);

    return 0;
}

void* writeMssg(){

    char buff[BUFFSIZE];

    while(clientRunning){

        if(buff == NULL){
            fprintf(stderr, "error allocating input");
        }
        if(newInput){
            bzero(buff, BUFFSIZE);
            int i;
            if (fgets(buff, BUFFSIZE, stdin)) {
                if (1 == sscanf(buff, "%d", &i)) {
                }
            }
            //encrypt data with cipher and key is 2
            for(i = 0; i<sizeof(buff);i++){
                if(buff[i] != ' ' && buff[i] != '\n'){
                    buff[i] = buff[i] + 2;
                }
                if(buff[i] == '\n' || buff[i] == '\0'){
                    break;
                }
            } 
            //put input in the list
            pthread_mutex_lock(&sendMutex);

            if(List_add(list1, &buff) != 0){
                printf("couldn't add item to list");
            };
            pthread_mutex_unlock(&sendMutex);

            inputTyped = true;
            newInput = false;
            
        }
    }
}

void* sendMssg(void *args){

    destination* dest = (destination *) args;

    sleep(1);

    while(clientRunning){

        if(inputTyped){
            pthread_mutex_lock(&sendMutex);

            if(sendto(dest->sockfd,(char *) List_curr(list1), BUFFSIZE, MSG_CONFIRM, dest->dst_addr, dest->addrlen) == -1){
                printf("Couldnt send message\n");
            }

            if(strncmp((char *) List_curr(list1), "#gzkv\n", 6) == 0){
                clientRunning = false;
            }

            List_trim(list1);
            pthread_mutex_unlock(&sendMutex);

            newInput = true;
            inputTyped = false;
        }
    }
}

void* recvMssg(void *args){

    destination* dest = (destination *) args;
    char buff[BUFFSIZE];
    int i;
    sleep(1);

    while(clientRunning){
        //receive input from server

        if(!messageReceived){
            bzero(buff, BUFFSIZE);
            if(recvfrom(dest->sockfd, buff, BUFFSIZE, 0, dest->dst_addr, &dest->addrlen) != -1){
                //decrypt message with cipher key 2
                for(i = 0; i<sizeof(buff);i++){
                    if(buff[i] != ' ' && buff[i] != '\n'){
                        buff[i] = buff[i] - 2;
                    }
                    if(buff[i] == '\n' || buff[i] == '\0'){
                        break;
                    }
                }

                if(strncmp(buff, "!status\n", 8) == 0){

                    if(sendto(dest->sockfd,"Qpnkpg\n", 8, MSG_CONFIRM, dest->dst_addr, dest->addrlen) == -1){
                        printf("couldnt send message\n");
                    }
                }
                else{
                    pthread_mutex_lock(&recMutex);
                    List_add(list2, &buff);
                    messageReceived = true;
                    pthread_mutex_unlock(&recMutex);
                }
            }
            else{
                printf("message not received\n");
            }
        }
    }
}

void* printMssg(){

    sleep(1);

    while(clientRunning){
        if(messageReceived){
            pthread_mutex_lock(&recMutex);    
            printf("%s",(char *) List_curr(list2));
            
            if(strncmp((char *) List_curr(list2), "!exit\n", 6) == 0){
                clientRunning = false;
            }
            messageReceived = false;
            List_trim(list2);
            pthread_mutex_unlock(&recMutex);
        }
    }
}

void threadInitalizer(destination dest){

    pthread_t th[4];
    pthread_mutex_init(&sendMutex, NULL);
    pthread_mutex_init(&recMutex, NULL);

    if(pthread_create(&th[0], NULL, &writeMssg, NULL) != 0){
        perror("failed to create thread \n");
    }

    if(pthread_create(&th[1], NULL, &sendMssg, &dest) != 0){
        perror("failed to create thread \n");
    }
    
    if(pthread_create(&th[2], NULL, &recvMssg, &dest) != 0){
        perror("failed to create thread \n");
    }
    
    if(pthread_create(&th[3], NULL, &printMssg, NULL) != 0){
        perror("failed to create thread \n");
    }
}