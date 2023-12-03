#include <stdio.h>            //basic
#include <time.h>             //저장한 file 이름을 현재 날짜로 하기 위해
#include <stdlib.h>           //malloc 동적할당
#include <string.h>           //memset 초기화
#include <netinet/if_ether.h> //etherrnet 구조체
#include <netinet/ip.h>       //ip header 구조체
#include <netinet/tcp.h>      //tcp header 구조체
#include <netinet/udp.h>      //udp header 구조체
#include <netinet/ip_icmp.h>  //icmp header 구조체
#include <sys/socket.h>       //소켓의 주소 설정 (sockaddr 구조체)
#include <arpa/inet.h>        //network 정보 변환
#include <pthread.h>          //thread
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define PACKET_SIZE 65536//패킷의 최대 사이즈

void filtering_right_packet(unsigned char *buffer, int rcv_data_size);

int main(int argc, char *argc){

    if(argc != 2){//인자가 정상적으로 전달되지 않았다면 사용법을 알려주고 종료
        printf("사용법: %s \"ip주소\" \n",argc[0]);
        return 1
    }

    printf("전달된 IP주소: %s\n",argc[1]);

    int raw_socket;
    struct sockaddr_in saddr;
    socklen_t saddr_size = sizeof(saddr);
    unsigned char *buffer = (unsigned char *)malloc(PACKET_SIZE);
    int rcv_data_size;//받은 패킷의 크기

    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (raw_socket < 0) {
        perror("Socket 생성에 실패했습니다.");
        return NULL;
    }

    while(1){
        rcv_data_size = recvfrom(raw_socket, buffer, PACKET_SIZE, 0, (struct sockaddr *)&saddr, &saddr_size);
        if (rcv_data_size < 0) {
            perror("수신에 실패했습니다.");
            return NULL;
        }
    }
}

void filtering_right_packet(unsigned char *buffer, int rcv_data_size){
    // IP 헤더 구조체
    struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));

}