#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

#define SHM_SIZE 1024
#define SHM_KEY 0x60044
#define SEM_KEY 0x60046
#define BILLION 1000000000L
#define IFLAGS (IPC_CREAT |IPC_EXCL) 

pthread_mutex_t Mutex;
char inputBuffer[SHM_SIZE]; // 입력을 저장할 변수
char combinedMessage[2 * SHM_SIZE]; // 배열 키우기 and 최종 배열
sem_t *semaphore; // 세마포
pthread_cond_t Cond;
pthread_cond_t Cond2;
pthread_cond_t Cond3;

struct timespec start, stop;
double accum;

struct shared_data {
    int flag;
    char message[SHM_SIZE];
};
int flag = 0;

void sem_change(sem_t* sem, int value) {
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = value;
    sem_b.sem_flg = SEM_UNDO;
    semop(semaphore, &sem_b, 1);
}
//
void *writerThread(struct shared_data *arg){ // 3번 전송 쓰레드
    while(1){
        pthread_mutex_lock(&Mutex);
        while (flag != 2) {
            // savedThread를 대기 상태로 만듦
            pthread_cond_wait(&Cond3, &Mutex);
        }
        //printf("Thread 3 공유 메모리 쓰기 작업\n");
        sem_change(semaphore, -1); // 세마포어 감소 연산
        //clock_gettime( CLOCK_MONOTONIC, &start); // 시간 시작
        //printf("클라이언트 세마포어 획득\n");
        strcpy(arg->message, combinedMessage); // read buf
        //clock_gettime( CLOCK_MONOTONIC, &stop); // 종료
        sem_change(semaphore, 1); // 세마포어 증가 연산
        //printf("클라이언트 세마포어 해제\n");
        //printf("Thread 3 공유 메모리 쓰기 작업 끝\n");
        //accum = ( stop.tv_sec - start.tv_sec) + (double)(stop.tv_nsec - start.tv_nsec) / (double)BILLION;
        //printf("send time: %.9f\n", accum);
        arg->flag = 1;
        flag = 0;
        pthread_cond_signal(&Cond);
        pthread_mutex_unlock(&Mutex);
    }
}

void *savedThread(char* argv[]){ // 뮤텍스 이용 2번 쓰레드
        while(1){
            pthread_mutex_lock(&Mutex);
            while (flag != 1) {
                // savedThread를 대기 상태로 만듦
                pthread_cond_wait(&Cond2, &Mutex);
            }
            //printf("Thread 2 Saving 작업 시작\n");
            // Concatenate argv[1] and inputBuffer into combinedMessage
            snprintf(combinedMessage, sizeof(combinedMessage), "%s:%s", argv[1], inputBuffer);
            printf("send to server - %s", combinedMessage);
            //printf("Thread 2 Saving 작업 끝\n");
            flag = 2;
            pthread_cond_signal(&Cond3);
            pthread_mutex_unlock(&Mutex);
        }
}

void *inputThread(struct shared_data *arg) { // 뮤텍스 이용 1번 쓰레드
    while(1){
        pthread_mutex_lock(&Mutex);
        while (flag != 0) {
            // inputThread를 대기 상태로 만듦
            pthread_cond_wait(&Cond, &Mutex);
        }  
        //printf("Thread 1 inputbuffer 쓰기 작업\n");
        printf("Enter message: ");
        fgets(inputBuffer, sizeof(inputBuffer), stdin); // buffer set
        //printf("Thread 1 inputbuffer 쓰기 작업 끝\n");
        flag = 1; // 플래그 상승으로 실행 후 대기상태
        pthread_cond_signal(&Cond2);
        pthread_mutex_unlock(&Mutex);
    }
}

int main(int argc, char* argv[]) {
    int shmid;
    struct shared_data *shared_memory;
    pthread_t writerThreadId, inputThreadId, savedThreadId;
    semaphore = sem_open("/my_semaphore", O_CREAT, 0666, 1);
    pthread_mutex_init(&Mutex, NULL); // 뮤텍스 초기화

    pthread_cond_init(&Cond, NULL);   // condition variable 초기화
    pthread_cond_init(&Cond2, NULL);   // condition variable 초기화
    pthread_cond_init(&Cond3, NULL);   // condition variable 초기화

    // 공유 메모리에 접근
    shmid = shmget(SHM_KEY, sizeof(struct shared_data), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // 공유 메모리 연결
    shared_memory = (struct shared_data *)shmat(shmid, NULL, 0);
    if ((void *)shared_memory == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    semaphore = semget(SEM_KEY, 1, 0);
    if (semaphore == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    semctl(semaphore, 0, SETVAL, 1); // 세마포어 초기화

    pthread_create(&inputThreadId, NULL, inputThread, (void*)shared_memory);
    pthread_create(&writerThreadId, NULL, writerThread, (void *)shared_memory);
    pthread_create(&savedThreadId, NULL, savedThread, (void *)argv);
    pthread_join(savedThreadId, NULL);
    pthread_join(inputThreadId, NULL);
    pthread_join(writerThreadId, NULL);

    pthread_mutex_destroy(&Mutex); // 뮤텍스 제거
    pthread_cond_destroy(&Cond);   // condition variable 제거
    pthread_cond_destroy(&Cond2);   // condition variable 제거
    pthread_cond_destroy(&Cond3);   // condition variable 제거

    if (shmctl(shmid, IPC_RMID, NULL) == -1) { // 공유메모리 제거
        printf("shmctl");
        exit(EXIT_FAILURE);
    }

    if (semctl(semaphore, 0, IPC_RMID, 0) == -1) { // 세마포어 제거
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    
    shmdt(shared_memory); // 공유메모리 해제 
}
