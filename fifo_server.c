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
#include <fcntl.h>
#include <errno.h>

#define FPERM 0777
#define BILLION 1000000000L

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msg_cond = PTHREAD_COND_INITIALIZER;

char *fifo = "team4fifo";
unsigned char *message = NULL;
int fd;

bool receive_message_available = true;
bool print_message_available = false;
bool save_message_available = false;

int serve(){

    message = malloc(1024);
    memset(message, 0, 1024);

    struct timespec start,stop;
    double accum;
    clock_gettime(CLOCK_MONOTONIC,&start);
    if(read(fd,message,1024)<0){
        free(message);
        message = NULL;
        return -1;
    }else{
        clock_gettime(CLOCK_MONOTONIC,&stop);
        accum = (stop.tv_sec-start.tv_sec)+(double)(stop.tv_nsec-start.tv_nsec)/(double)BILLION;
        printf("receive time: %.9f\n",accum);
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

        int sig = serve();

        if(message != NULL){
            print_message_available = true;
            save_message_available = true;
        }
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

        fprintf(comments_file, "%s", message);
        fclose(comments_file);

        save_message_available = false;
        pthread_cond_broadcast(&msg_cond);
        pthread_mutex_unlock(&mutex);
    }
}

int main(){
    if(mkfifo(fifo,FPERM)==-1){
        if(errno != EEXIST)
            perror("receiver: mkfifo");
    }
    
    if((fd = open(fifo, O_RDWR | O_NONBLOCK))<0)
        perror("fifo open failed");

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