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
#define QKEY (key_t)60040
#define QPERM 0777
#define BILLION 1000000000L

struct message_entry{
    long data_type;
    char message[1024];
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msg_cond = PTHREAD_COND_INITIALIZER;

unsigned char *get_message = NULL;
unsigned char *send_message = NULL;
int priority = 1;

bool get_message_available = true;
bool make_send_message_available = false;
bool send_message_available = false;

int init_queue() {
    int qid;
    if((qid = msgget(QKEY,IPC_CREAT|QPERM)) == -1){
        perror("msgget failed");
    }
    return qid;
}

int enter(){
    int qid = init_queue();
    struct message_entry send_entry;
    send_entry.data_type = (long)priority;
    memcpy(send_entry.message, send_message,strlen(send_message));
    if(msgsnd(qid, &send_entry, strlen(send_message), 0) == -1){
        perror("msgsnd failed");
        return -1;
    } else {
        return 0;
    }
}

void *get_user_message(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(!get_message_available){
            pthread_cond_wait(&msg_cond,&mutex);
        }
        get_message = (unsigned char *)malloc(1024);
        printf("Enter message: ");
        fgets(get_message, 1024, stdin);

        make_send_message_available = true;
        get_message_available = false;
        pthread_cond_broadcast(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

void *make_send_message(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(!(make_send_message_available)){
            pthread_cond_wait(&msg_cond,&mutex);
        }
        char *name = (char*)arg;
        send_message = malloc(1);
        send_message[0] = '\0'; 
        send_message = realloc(send_message, strlen(name) + strlen(get_message)+5);
        strcat(send_message, name);
        strcat(send_message,": ");
        strcat(send_message,get_message);
        free(get_message);
        get_message = NULL;

        make_send_message_available = false;
        send_message_available = true;
        pthread_cond_broadcast(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

void *send_make_message(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(!send_message_available){
            pthread_cond_wait(&msg_cond,&mutex);
        }
        int sig = enter();
        free(send_message);
        send_message = NULL;
        send_message_available = false;
        get_message_available = true;
        pthread_cond_broadcast(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

int main(int argc, char *argv[]){
    //argv[1] = "김성민";argc+=1; // 테스트를 위한 코드
    if(argc != 2){
        printf("사용법 : ./%s \"User Name\"",argv[0]);
        return -1;
    }

    pthread_t send_thread, make_thread, get_thread;

    pthread_create(&get_thread, NULL, get_user_message, NULL);
    pthread_create(&make_thread, NULL, make_send_message, (void*)argv[1]);
    pthread_create(&send_thread, NULL, send_make_message, NULL);   

    pthread_join(get_thread,NULL);
    pthread_join(make_thread,NULL);
    pthread_join(send_thread,NULL);

    return 0;
}