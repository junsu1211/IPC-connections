#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <time.h>

#define SHM_SIZE 1024
#define SHM_KEY 0x60044
#define SEM_KEY 0x60046
#define BILLION 1000000000L
#define SHM_KEY2 0x60045
#define SEM_KEY2 0x60047

char buf[SHM_SIZE];
char buf2[SHM_SIZE];

char filename[50] = "shared file";
char filename2[50] = "shared file"; 

pthread_mutex_t Mutex;
pthread_mutex_t Mutex2;

sem_t *semaphore; // 세마포
sem_t *semaphore2;
//
int flag = 0;
int flag2 = 0;
// 1번 클라이언트 전역 변수
pthread_cond_t Cond;
pthread_cond_t Cond2;
pthread_cond_t Cond3;
// 2번 클라이언트 전역 변수
pthread_cond_t Ccond;
pthread_cond_t Ccond2;
pthread_cond_t Ccond3;

struct timespec start, stop, start2, stop2;
double accum, accum2;

struct shared_data {
    int flag;
    char message[SHM_SIZE];
};

void sem_change(sem_t *sem, int value) {
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = value;
    sem_b.sem_flg = SEM_UNDO;
    semop(semaphore, &sem_b, 1);
}

// 1번 클라이언트 쓰레드
void *printThread(struct shared_data *shared_memory){
    while(1){
        pthread_mutex_lock(&Mutex);
        while (flag != 2) {
            // savedThread를 대기 상태로 만듦
            pthread_cond_wait(&Cond3, &Mutex);
        }
        //printf("<<  %s.txt  >>\n",filename);
        /*FILE *file = fopen(filename, "r");
        if (file == NULL) {
        perror("fopen");
        return 1;
        }*/
    
        printf("%s", buf);
        flag = 0;
        pthread_cond_signal(&Cond);
        pthread_mutex_unlock(&Mutex);
    }
}

void *makefileThread(struct shared_data *shared_memory){
    while(1){
        pthread_mutex_lock(&Mutex);
        while (flag != 1) {
            pthread_cond_wait(&Cond2, &Mutex);
        }
        /*char *delimiter = strchr(buf,':'); // : 이전까지의 내용인 파라미터를 파일의 제목으로 사용
        if (delimiter == NULL) {
            fprintf(stderr, "Invalid message format: %s\n", buf);
            continue;
        }
        // 파일 이름을 저장할 버퍼 할당
        size_t filename_length = delimiter - buf;
        strncpy(filename, buf, filename_length);
        filename[filename_length] = '\0';
        */
        FILE *file = fopen(filename, "a");
        if (file == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        //printf("파일 쓰기 작업 완료\n");
        fprintf(file, "%s", buf); // 버퍼의 내용 파일에 쓰기
        fclose(file);
        flag = 2;
        pthread_cond_signal(&Cond3);
        pthread_mutex_unlock(&Mutex);
    }
}

void *sharedThread(struct shared_data *shared_memory){ // 클라이언트로부터 계속해서 메시지 읽고잇음
        while(1){
            pthread_mutex_lock(&Mutex);
            while (flag != 0) {
                pthread_cond_wait(&Cond, &Mutex);
            } 
            if(shared_memory->flag == 1){
                //printf("서버 공유메모리 읽기 작업\n");
                sem_change(semaphore, -1);
                //printf("서버 세마포어 획득\n");
                //clock_gettime( CLOCK_MONOTONIC, &start); // 시간 시작
                strcpy(buf, shared_memory->message); // 공유 메모리의 내용을 서버의 버퍼로 복사
                //printf("%s",buf);
                //clock_gettime( CLOCK_MONOTONIC, &stop); // 종료
                sem_change(semaphore, 1);
                //accum = ( stop.tv_sec - start.tv_sec) + (double)(stop.tv_nsec - start.tv_nsec) / (double)BILLION;
                //printf("recieve time: %.9f\n", accum);
                //printf("서버 세마포어 해제\n");
                //printf("서버 공유 메모리 읽기 작업 끝\n");
                shared_memory->flag = 0;
                flag = 1; // 플래그 상승으로 실행 후 대기상태
            }
            pthread_cond_signal(&Cond2);
            pthread_mutex_unlock(&Mutex);
        }
}


// 2번 클라이언트 쓰레드
void *printThread2(struct shared_data *shared_memory2){
    while(1){
        pthread_mutex_lock(&Mutex2);
        while (flag2 != 2) {
            // savedThread를 대기 상태로 만듦
            pthread_cond_wait(&Ccond3, &Mutex2);
        }
        //printf("<<  %s.txt  >>\n",filename2);
        FILE *file2 = fopen(filename2, "r");
        if (file2 == NULL) {
        perror("fopen");
        return 1;
        }

        int character;
        //printf("- - - - - - - - Coment - - - - - - - - \n");
        /*while ((character = fgetc(file2)) != EOF) { // 파일 출력
            putchar(character);
        }*/
        printf("%s", buf2);
        flag2 = 0;
        pthread_cond_signal(&Ccond);
        pthread_mutex_unlock(&Mutex2);
    }
}

void *makefileThread2(struct shared_data *shared_memory2){
  
    while(1){
        pthread_mutex_lock(&Mutex2);
        while (flag2 != 1) {
            pthread_cond_wait(&Ccond2, &Mutex2);
        }
        /*char *delimiter = strchr(buf2,':'); // : 이전까지의 내용인 파라미터를 파일의 제목으로 사용
        if (delimiter == NULL) {
            fprintf(stderr, "Invalid message format: %s\n", buf2);
            continue;
        }
        // 파일 이름을 저장할 버퍼 할당
        size_t filename2_length = delimiter - buf2;
        strncpy(filename2, buf2, filename2_length);
        filename2[filename2_length] = '\0';
        */
        FILE *file2 = fopen(filename2, "a");
        if (file2 == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        //printf("파일 쓰기 작업 완료\n");
        fprintf(file2, "%s", buf2); // 버퍼의 내용 파일에 쓰기
        fclose(file2);
        flag2 = 2;
        pthread_cond_signal(&Ccond3);
        pthread_mutex_unlock(&Mutex2);
    }
}

void *sharedThread2(struct shared_data *shared_memory2){ // 클라이언트로부터 계속해서 메시지 읽고잇음
        while(1){
            pthread_mutex_lock(&Mutex2);
            while (flag2 != 0) {
                pthread_cond_wait(&Ccond, &Mutex2);
            } 
            if(shared_memory2->flag == 1){
                //printf("서버 공유메모리 읽기 작업\n");
                sem_change(semaphore2, -1);
               //clock_gettime( CLOCK_MONOTONIC, &start2); // 시간 시작
                //printf("서버 세마포어 획득\n");
                strcpy(buf2, shared_memory2->message); // 공유 메모리의 내용을 서버의 버퍼로 복사
                //printf("%s",buf);
                //clock_gettime( CLOCK_MONOTONIC, &stop2); // 종료
                sem_change(semaphore2, 1);
                //accum2 = ( stop2.tv_sec - start2.tv_sec) + (double)(stop2.tv_nsec - start2.tv_nsec) / (double)BILLION;
                //printf("recieve time: %.9f\n", accum2);
                //printf("서버 세마포어 해제\n");
                //printf("서버 공유 메모리 읽기 작업 끝\n");
                shared_memory2->flag = 0;
                flag2 = 1; // 플래그 상승으로 실행 후 대기상태
            }
            pthread_cond_signal(&Ccond2);
            pthread_mutex_unlock(&Mutex2);
        }
}

int main() {


    // 1번 클라이언트
    int shmid;
    pthread_t sharedThreadId, printThreadId, makefileThreadId;
    struct shared_data *shared_memory;
    pthread_mutex_init(&Mutex, NULL); // 뮤텍스 초기화
    pthread_cond_init(&Cond, NULL);   // condition variable 초기화
    pthread_cond_init(&Cond2, NULL);   // condition variable 초기화
    pthread_cond_init(&Cond3, NULL);   // condition variable 초기화
    semaphore = sem_open("/my_semaphore", O_CREAT, 0666, 1);
    semaphore2 = sem_open("/my_semaphore2", O_CREAT, 0666, 1);
    // 공유 메모리 생성
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

    semaphore = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semaphore == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // 2번 클라이언트


    int shmid2;
    struct shared_data *shared_memory2;
    pthread_t sharedThreadId2, printThreadId2, makefileThreadId2;
    pthread_mutex_init(&Mutex2, NULL); // 뮤텍스 초기화
    pthread_cond_init(&Ccond, NULL);   // condition variable 초기화
    pthread_cond_init(&Ccond2, NULL);   // condition variable 초기화
    pthread_cond_init(&Ccond3, NULL);   // condition variable 초기화

     // 공유 메모리 생성
    shmid2 = shmget(SHM_KEY2, sizeof(struct shared_data), IPC_CREAT | 0666);
    if (shmid2 == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // 공유 메모리 연결
    shared_memory2 = (struct shared_data *)shmat(shmid2, NULL, 0);
    if ((void *)shared_memory2 == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    semaphore2 = semget(SEM_KEY2, 1, IPC_CREAT | 0666);
    if (semaphore2 == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    printf("- - - - - - - - - coment - - - - - - - - - \n\n");
    pthread_create(&sharedThreadId, NULL, sharedThread, (void*)shared_memory);
    pthread_create(&printThreadId, NULL, printThread, (void*)shared_memory);
    pthread_create(&makefileThreadId, NULL, makefileThread, (void*)shared_memory);
    pthread_create(&sharedThreadId2, NULL, sharedThread2, (void*)shared_memory2);
    pthread_create(&printThreadId2, NULL, printThread2, (void*)shared_memory2);
    pthread_create(&makefileThreadId2, NULL, makefileThread2, (void*)shared_memory2);
    pthread_join(makefileThreadId2, NULL);
    pthread_join(printThreadId2, NULL);
    pthread_join(sharedThreadId2, NULL);
    pthread_join(makefileThreadId, NULL);
    pthread_join(printThreadId, NULL);
    pthread_join(sharedThreadId, NULL);

    if (semctl(semaphore, 0, IPC_RMID, 0) == -1) { // 세마포어 제거
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    if (semctl(semaphore2, 0, IPC_RMID, 0) == -1) { // 세마포어 제거
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    shmdt(shared_memory2);
    shmdt(shared_memory);
    shmctl(shmid, IPC_RMID, NULL); 
    shmctl(shmid2, IPC_RMID, NULL);
}
