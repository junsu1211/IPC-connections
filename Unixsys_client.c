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

void filtering_right_packet(int rcv_data_size, char *t_ip);
void filtering_TCPorUDP_packet_maindata();
void receive_packet();

unsigned char *buffer = NULL;//패킷을 받을 버퍼
unsigned char *packet_data = NULL;//버퍼로 받은 패킷을 받은 길이만큼으로 해서 저장
unsigned char *packet_Header_data = NULL;

int main(int argc, char *argv[]){

    if(argc != 2){//인자가 정상적으로 전달되지 않았다면 사용법을 알려주고 종료
        printf("사용법: %s \"ip주소\" \n",argv[0]);
        return 1;
    }

    int raw_socket;
    struct sockaddr_in saddr;
    socklen_t saddr_size = sizeof(saddr);
    buffer = (unsigned char *)malloc(PACKET_SIZE);//패킷의 최대사이즈만큼 동적할당
    int rcv_data_size;//받은 패킷의 크기
    char *t_ip = argv[1];

    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (raw_socket < 0) {
        perror("Socket 생성에 실패했습니다.");
        return 1;
    }

    while(1){
        rcv_data_size = recvfrom(raw_socket, buffer, PACKET_SIZE, 0, (struct sockaddr *)&saddr, &saddr_size);
        if (rcv_data_size < 0) {
            perror("수신에 실패했습니다.");
            return 1;
        }

    }
}
//정상적인 패킷의 길이여부, ip주소 일치여부 판단후 packet_data에 동적할당하여 저장
void filtering_right_packet(int rcv_data_size, char *t_ip){
    // IP 헤더 구조체
    struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    //정상적인 패킷의 최소 사이즈
    int minimum_size = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr);

    // source, dest ip 가져오기
    char dest_ipaddress[100];
    inet_ntop(AF_INET,&(iph->daddr),dest_ipaddress,INET_ADDRSTRLEN);
    char source_ipaddress[100];
    inet_ntop(AF_INET,&(iph->saddr),source_ipaddress,INET_ADDRSTRLEN);

    //정상적인 패킷의 길이이면서 ip주소가 설정한 ip주소와 일치여부 점검
    if((rcv_data_size > minimum_size) && ((dest_ipaddress == t_ip)||(source_ipaddress == t_ip))){
        //받은 패킷의 길이만큼 동적할당
        packet_data = (unsigned char *)malloc(rcv_data_size);
        if(packet_data == NULL){
            perror("메모리 할당 실패");
            exit(EXIT_FAILURE);
        }
        memcpy(packet_data, buffer, rcv_data_size);//받은 패킷의 길이만큼 동적할당한 배열에 복사
    }
    free(buffer);
    buffer = NULL;
}
//tcp or udp인지를 판단후 패킷의 헤더부분만 동적할당하여 저장
void filtering_TCPorUDP_packet_maindata(){
    struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));

    if(iph->protocol == 6){//TCP인 경우

        int tcp_main_size = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct tcphdr);
        packet_Header_data = (unsigned char*)malloc(tcp_main_size);
        if(packet_Header_data  == NULL){
            perror("메모리 할당 실패");
            exit(EXIT_FAILURE);
        }
        memcpy(packet_Header_data,packet_data,tcp_main_size);
        free(packet_data);
        packet_data = NULL;
    }else if(iph->protocol == 17){//UDP인 경우

        int udp_main_size = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr); 
        packet_Header_data  = (unsigned char*)malloc(udp_main_size);
        if(packet_Header_data  == NULL){
            perror("메모리 할당 실패");
            exit(EXIT_FAILURE);
        }
        memcpy(packet_Header_data ,packet_data,udp_main_size); 
        free(packet_data);
        packet_data = NULL;
    }
}
//서버에 패킷의 헤더 내용 전달
void receive_packet(){

    //IPC기법을 활용해 packet_Header_data를 서버로 전달?
    printf("서버로 헤더 내용 %zu 만큼 전달 완료", sizeof(packet_Header_data));

    //전달후 packet_Header_data 반환
    free(packet_Header_data);
    packet_Header_data = NULL;

}