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
#include <signal.h>

#include <time.h>
#define BILLION 1000000000L

#define SEND_QKEY (key_t)60042
#define RCV_QKEY (key_t)60043
#define QPERM 0777

struct send_message_entry{
    long data_type;
    char message[256];
};

struct rcv_message_entry{
    long data_type;
    char message[10000];
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msg_cond = PTHREAD_COND_INITIALIZER;

char *message = NULL;
bool message_available = false;

unsigned char *comments_buffer;
bool comments_buffer_available = false;
int priority = 1;
int mi = 0;

int init_send_queue() {
    int qid;
    if((qid = msgget(SEND_QKEY,IPC_CREAT|QPERM)) == -1){
        perror("msgget failed");
    }
    return qid;
}

int init_rcv_queue() {
    int qid;
    if((qid = msgget(RCV_QKEY,IPC_CREAT|QPERM)) == -1){
        perror("msgget failed");
    }
    return qid;
}

int enter(){
    int s_qid = init_send_queue();
    struct send_message_entry send_entry;

    send_entry.data_type = (long)priority;
    //priority +=1;
    strcpy(send_entry.message, message);

    if(msgsnd(s_qid, &send_entry, 256, 0) == -1){
        perror("msgsnd failed");
        return -1;
    } else {
        return 0;
    }
}

void *get_message(void *arg){

    while(1){
        pthread_mutex_lock(&mutex);
        message = (unsigned char *)malloc(256);
        printf("Comments\n");
        if(comments_buffer_available==true){
            printf("%s",comments_buffer);
            free(comments_buffer);
            comments_buffer = NULL;
            comments_buffer_available=false;
        }
        printf("Enter message: ");
        fgets(message, 256, stdin);
        printf("\n");
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
        struct timespec start,stop;
        double accum;
        clock_gettime(CLOCK_MONOTONIC,&start);
        int send_ok = enter();  //이 부분에 ipc 기법을 이용한 message 배열을 보내기
        if (send_ok != 0) {
            perror("enter failed");
        }
        clock_gettime(CLOCK_MONOTONIC,&stop);
        accum = (stop.tv_sec-start.tv_sec)+(double)(stop.tv_nsec-start.tv_nsec)/(double)BILLION;
        printf("send time: %.9f\n",accum);
        free(message);
        message = NULL;
        message_available = false;
        pthread_cond_signal(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}
void *recv_message(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(comments_buffer_available){
            pthread_cond_wait(&msg_cond,&mutex);
        }

        int r_qid = init_rcv_queue();int mlen;
        struct rcv_message_entry rcv_entry;
        ssize_t rcvsize;

        struct timespec start,stop;
        double accum;
        clock_gettime(CLOCK_MONOTONIC,&start);
        if((rcvsize = msgrcv(r_qid, &rcv_entry, sizeof(rcv_entry), 0, IPC_NOWAIT)) == -1){
            pthread_cond_wait(&msg_cond,&mutex);
        }else{
            clock_gettime(CLOCK_MONOTONIC,&stop);
            accum = (stop.tv_sec-start.tv_sec)+(double)(stop.tv_nsec-start.tv_nsec)/(double)BILLION;
            printf("receive time: %.9f\n",accum);
            comments_buffer = malloc(1);
            comments_buffer[0] = '\0';
            comments_buffer = realloc(comments_buffer, strlen(comments_buffer)
            + strlen(rcv_entry.message)+2);
            strcat(comments_buffer, rcv_entry.message);
            comments_buffer_available=true;
        }
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

    return 0;
}