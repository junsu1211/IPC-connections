# UnixsystemPrg
ICPC 프로세스간 통신기법을 이용한 공유 메모장 프로그램
- Massage Passing => msgpas_client.c, msgpas_server.c
- FIFO => fifo_client, fifo_server
- Shared Memory => clientS.c, serverS.c

- 읽기와 쓰기작업은 뮤텍스와 세마포어를 사용하여 교착상태가 일어나지 않도록 처리함
