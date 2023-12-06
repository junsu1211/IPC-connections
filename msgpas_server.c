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

int priority = 1;
unsigned char *message = NULL;

bool receive_message_available = true;
bool print_message_available = false;
bool save_message_available = false;

int init_queue() {
    int qid;
    if((qid = msgget(QKEY,IPC_CREAT|QPERM)) == -1){
        perror("msgget failed");
    }
    return qid;
}

int serve(){

    int qid = init_queue();
    struct message_entry recv_entry;
    ssize_t recv_size;

    if((recv_size = msgrcv(qid, &recv_entry, 1024, 0, 0))==-1){
        perror("msgrcv fail");
        return -1;
    }else{
        message = malloc(1);
        message[0] = '\0';
        message = realloc(message, strlen(recv_entry.message)+2);
        strcat(message,recv_entry.message);
        return 0;
    }
}

void *recv_message(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(print_message_available || save_message_available){
            pthread_cond_wait(&msg_cond,&mutex);
        }
        if(message != NULL){
            free(message);
            message = NULL;
        }
        int sig;
        if((sig=serve())==-1){
            perror("receive fail");
        }

        print_message_available = true;
        save_message_available = true;
        pthread_cond_broadcast(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

void *print_comments(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(!print_message_available){
            pthread_cond_wait(&msg_cond,&mutex);
        }

        printf("%s",message);

        print_message_available = false;
        pthread_cond_broadcast(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

void *save_comments(void *arg){
    while(1){
        pthread_mutex_lock(&mutex);
        while(!save_message_available){
            pthread_cond_wait(&msg_cond,&mutex);
        }

        FILE *comments_file;
        comments_file = fopen("comments.txt", "a");
        char save_message[1024];
        strcat(save_message,"\n");
        strcat(save_message,message);
        fprintf(comments_file, "%s", message);
        fclose(comments_file);

        save_message_available = false;
        pthread_cond_broadcast(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

int main(){
    FILE *comments_file;
    comments_file = fopen("comments.txt", "w");
    fprintf(comments_file, "- - - - - - - Comments - - - - - - -\n\n");
    fclose(comments_file);
    printf("- - - - - - - Comments - - - - - - -\n");

    pthread_t recv_thread, print_thread, save_thread;

    pthread_create(&recv_thread, NULL, recv_message, NULL);
    pthread_create(&print_thread, NULL, print_comments, NULL); 
    pthread_create(&save_thread, NULL, save_comments, NULL);

    pthread_join(recv_thread,NULL);
    pthread_join(print_thread,NULL);
    pthread_join(save_thread,NULL);
}