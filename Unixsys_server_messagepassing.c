//이게 서버 최종본
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

#define C1_SEND_QKEY (key_t)0321
#define C1_RCV_QKEY (key_t)0322
#define C2_SEND_QKEY (key_t)0323
#define C2_RCV_QKEY (key_t)0324
#define QPERM 0777

struct send_message_entry{
    long data_type;
    char message[10000];
};

struct rcv_message_entry{
    long data_type;
    char message[256];
};


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msg_cond = PTHREAD_COND_INITIALIZER;

//char *message = NULL;
//bool message_available = false;

//char *receive_message = NULL;
bool receive_massage_available = false;
int priority = 1;
int cbsize = 0;
unsigned char *comments_buffer;
//bool new_message = false;
bool send_new_message = false;

int init_C1send_queue() {
    int qid;
    if((qid = msgget(C1_RCV_QKEY,IPC_CREAT|QPERM)) == -1){
        perror("msgget failed");
    }
    return qid;
}

int init_C1rcv_queue(){
    int qid;
    if((qid = msgget(C1_SEND_QKEY,IPC_CREAT|QPERM)) == -1){
        perror("msgget failed");
    }
    return qid;
}

int enter(){
    int s_qid = init_C1send_queue();
    struct send_message_entry send_entry;

    send_entry.data_type = (long)priority;
    //priority +=1;
    memcpy(send_entry.message, comments_buffer,cbsize);

    if(msgsnd(s_qid, &send_entry, cbsize, 0) == -1){
        perror("msgsnd failed");
        return -1;
    } else {
        return 0;
    }
}

void *recv_message(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(receive_massage_available){
            pthread_cond_wait(&msg_cond,&mutex);
        }
        int r_qid = init_C1rcv_queue();
        struct rcv_message_entry rcv_entry;
        ssize_t rcvsize;

        if((rcvsize = msgrcv(r_qid, &rcv_entry, 256, 0, 0)) == -1){
            pthread_cond_wait(&msg_cond,&mutex);
        }else{
            receive_massage_available = true;
            comments_buffer = realloc(comments_buffer, strlen(comments_buffer)
            + strlen(rcv_entry.message)+2);
            cbsize = cbsize + strlen(comments_buffer) + strlen(rcv_entry.message) + 2;
            strcat(comments_buffer, rcv_entry.message);
            send_new_message = true;
        }
        pthread_cond_signal(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

void *print_comments(void *arg){

    while(1){
        pthread_mutex_lock(&mutex);
        if(receive_massage_available==true){
            printf("Client Comments\n");
            printf("%s",comments_buffer);
            //free(receive_message);
            //receive_message = NULL;
            receive_massage_available = false;
        }
        pthread_cond_signal(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}
void *send_message(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(!send_new_message){
            pthread_cond_wait(&msg_cond,&mutex);
        }   

        int send_ok = enter();
        if(send_ok != 0)
            perror("enter failed");
        

        send_new_message = false;
        pthread_cond_signal(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

int main(){
    comments_buffer = malloc(1);
    cbsize +=1;
    comments_buffer[0] = '\0';
    pthread_t send_thread, recv_thread, comments_thread;

    pthread_create(&send_thread, NULL, send_message, NULL);
    pthread_create(&recv_thread, NULL, recv_message, NULL);
    pthread_create(&comments_thread, NULL, print_comments, NULL); 

    pthread_join(send_thread,NULL);
    pthread_join(recv_thread,NULL);
    pthread_join(comments_thread,NULL);
}