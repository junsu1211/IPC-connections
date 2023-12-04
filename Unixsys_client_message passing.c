#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define QKEY (key_t)1230
#define QPERM 0777

typedef struct{
    long data_type;
    unsigned char message[1024];
} message_entry;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msg_cond = PTHREAD_COND_INITIALIZER;

unsigned char *message = NULL;
unsigned char *receive_message = NULL;
bool message_available = false;
bool receive_message_available = false;
bool new_message = true;

int enter(){
    int s_qid;
    struct message_entry send_entry;
    send_entry.data_type = (long)0;
    strcpy(send_entry.message, message);

    //if((s_qid = init_queue())==-1)
        //return (-1);

    if(msgsnd(s_qid,&send_entry,1024,0)==-1){
        perror("msgsnd failed");
        return(-1);
    }else{
        return (0);
    }

}

void *get_message(void *arg){

    while(1){
        pthread_mutex_lock(&mutex);
        message = (unsigned char *)malloc(1024);
        if(receive_message_available == true){
            printf("Receive Server Message: %s\n",receive_message);
            free(receive_message);
            receive_message = NULL;
            receive_message_available = false;
        }else{
            printf("No New Message\n");
        }
        new_message = false;
        printf("Enter message: ");
        fgets(message, 1024, stdin);
        message_available = true;
        pthread_cond_signal(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}
void *send_message(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(!message_available){
            pthread_cond_wait(&msg_cond,&mutex);
        }   

        //이 부분에 ipc 기법을 이용한 message 배열을 보내기
        int send_ok = enter();
        if(send_ok == 0)
            printf("Send message: %s",message);
        
        free(message);
        message = NULL;
        message_available = false;
        pthread_mutex_unlock(&mutex);
    }
}
void *recv_message(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(receive_message_available){
            pthread_cond_wait(&msg_cond,&mutex);
        }
        receive_message = (unsigned char *)malloc(1024);

        //이 부분에 ipc 기법을 이용해 receive_message에 저장
        strcpy(receive_message, "1234");

        receive_message_available = true;
        pthread_cond_signal(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

int main(){
    pthread_t send_thread, recv_thread, get_thread;

    pthread_create(&send_thread, NULL, send_message, NULL);
    pthread_create(&recv_thread, NULL, recv_message, NULL);
    pthread_create(&get_thread, NULL, get_message, NULL); 

    pthread_join(send_thread,NULL);
    pthread_join(recv_thread,NULL);
    pthread_join(get_thread,NULL);
}