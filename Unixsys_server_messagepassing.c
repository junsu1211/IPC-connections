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

#define C1_SEND_QKEY (key_t)60040
#define C1_RCV_QKEY (key_t)60041
#define C2_SEND_QKEY (key_t)60042
#define C2_RCV_QKEY (key_t)60043
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
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msg_cond2 = PTHREAD_COND_INITIALIZER;

bool receive_massage_available = false;
int priority = 1;
int cbsize = 0;
unsigned char *comments_buffer;
bool send_new_message = false;

bool receive_massage_available2 = false;
int cbsize2 = 0;
unsigned char *comments_buffer2;
bool send_new_message2 = false;

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

int init_C2send_queue() {
    int qid;
    if((qid = msgget(C2_RCV_QKEY,IPC_CREAT|QPERM)) == -1){
        perror("msgget failed");
    }
    return qid;
}

int init_C2rcv_queue(){
    int qid;
    if((qid = msgget(C2_SEND_QKEY,IPC_CREAT|QPERM)) == -1){
        perror("msgget failed");
    }
    return qid;
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

void *send_message(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(!send_new_message){
            pthread_cond_wait(&msg_cond,&mutex);
        }   
        int s_qid = init_C1send_queue();
        struct send_message_entry send_entry;
        send_entry.data_type = (long)priority;
        //priority +=1;
        memcpy(send_entry.message, comments_buffer,cbsize);
        if(msgsnd(s_qid, &send_entry, cbsize, 0) == -1){
            perror("msgsnd failed");
        }
        send_new_message = false;
        pthread_cond_signal(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

void *recv_message2(void *arg){
    while(1){
        pthread_mutex_lock(&mutex2);
        while(receive_massage_available2){
            pthread_cond_wait(&msg_cond2,&mutex2);
        }
        int r_qid = init_C2rcv_queue();
        struct rcv_message_entry rcv_entry;
        ssize_t rcvsize;

        if((rcvsize = msgrcv(r_qid, &rcv_entry, 256, 0, 0)) == -1){
            pthread_cond_wait(&msg_cond2,&mutex2);
        }else{
            receive_massage_available2 = true;
            comments_buffer2 = realloc(comments_buffer2, strlen(comments_buffer2)
            + strlen(rcv_entry.message)+2);
            cbsize2 = cbsize2 + strlen(comments_buffer2) + strlen(rcv_entry.message) + 2;
            strcat(comments_buffer2, rcv_entry.message);
            send_new_message2= true;
        }
        pthread_cond_signal(&msg_cond2);
        pthread_mutex_unlock(&mutex2);
    }
}

void *send_message2(void *arg){
    while(1){
        pthread_mutex_lock(&mutex2);
        while(!send_new_message2){
            pthread_cond_wait(&msg_cond2,&mutex2);
        }   
        int s_qid = init_C2send_queue();
        struct send_message_entry send_entry;
        send_entry.data_type = (long)priority;
        memcpy(send_entry.message, comments_buffer2,cbsize2);
        if(msgsnd(s_qid, &send_entry, cbsize2, 0) == -1){
            perror("msgsnd failed");
        }
        send_new_message2 = false;
        pthread_cond_signal(&msg_cond2);
        pthread_mutex_unlock(&mutex2);
    }
}


void *print_comments(void *arg){

    while(1){
        pthread_mutex_lock(&mutex);
        if(receive_massage_available == true){
            printf("\nClient Comments\n");
            printf("%s", comments_buffer);
            receive_massage_available = false;
        }
        pthread_cond_signal(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

void *print_comments2(void *arg){

    while(1){
        pthread_mutex_lock(&mutex2);
        if(receive_massage_available2 == true){
            printf("\nClient2 Comments\n");
            printf("%s", comments_buffer2);
            receive_massage_available2 = false;
        }
        pthread_cond_signal(&msg_cond2);
        pthread_mutex_unlock(&mutex2);
    }
}


int main(){
    comments_buffer = malloc(1);
    cbsize +=1;
    comments_buffer[0] = '\0';

    comments_buffer2 = malloc(1);
    cbsize2 +=1;
    comments_buffer2[0] = '\0';

    pthread_t send_thread1, recv_thread1, comments_thread;
    pthread_t send_thread2,recv_thread2, comments_thread2;

    pthread_create(&send_thread1, NULL, send_message, NULL);
    pthread_create(&recv_thread1, NULL, recv_message, NULL);
    pthread_create(&comments_thread, NULL, print_comments, NULL); 
    pthread_create(&send_thread2, NULL, send_message2, NULL);
    pthread_create(&recv_thread2, NULL, recv_message2, NULL);
    pthread_create(&comments_thread2, NULL, print_comments2, NULL); 

    pthread_join(send_thread1,NULL);
    pthread_join(recv_thread1,NULL);
    pthread_join(comments_thread,NULL);
    pthread_join(send_thread2,NULL);
    pthread_join(recv_thread2,NULL);
    pthread_join(comments_thread2,NULL);
}