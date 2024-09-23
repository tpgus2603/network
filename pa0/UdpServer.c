// UdpEchoServer.c
// UDP Echo Server with Chat Functionality

#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ws2tcpip.h>


#define BUFSIZE 1024

// 동작 형식 코드 정의
#define opt_ECHO  0x0001
#define opt_CHAT  0x0002
#define opt_STAT  0x0003
#define opt_QUIT  0x0004

// 오류 처리 함수
void err_quit(const char *msg) {
    LPVOID lpMsgBuf;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
        NULL, 
        WSAGetLastError(), 
        0, 
        (LPSTR)&lpMsgBuf, 
        0, 
        NULL
    );
    fprintf(stderr, "%s: %s\n", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        printf("Usage: %s <Server IP> <Port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* server_ip = argv[1];
    int server_port = atoi(argv[2]);

    WSADATA wsa;
    SOCKET sock;
    SOCKADDR_IN serveraddr, clientaddr;
    char buf[BUFSIZE];
    int clientaddr_len = sizeof(clientaddr);
    int retval;

    // 윈속 초기화
    if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        err_quit("WSAStartup()");
    }

    // 소켓 생성
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == INVALID_SOCKET) {
        err_quit("socket()");
    }

    // 서버 주소 설정
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(server_port);
    serveraddr.sin_addr.s_addr = inet_addr(server_ip);

    // 바인딩
    if(bind(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR) {
        err_quit("bind()");
    }

    printf("UDP  Server is running on %s:%d\n", server_ip, server_port);

    // 통계 정보 초기화
    unsigned long total_bytes = 0;
    unsigned long total_messages = 0;

    while(1) {
        memset(buf, 0, BUFSIZE);
        retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&clientaddr, &clientaddr_len);
        if(retval == SOCKET_ERROR) {
            fprintf(stderr, "recvfrom() error: %d\n", WSAGetLastError());
            continue;
        }

        total_bytes += retval;
        total_messages += 1;

        // 클라이언트 정보 추출
        char client_ip[INET_ADDRSTRLEN];
        strcpy(client_ip, inet_ntoa(clientaddr.sin_addr));
        unsigned short client_port = ntohs(clientaddr.sin_port);

        // 동작 형식 추출
        unsigned short opt = ((unsigned char)buf[0] << 8) | (unsigned char)buf[1];
        char* message = buf + 2;
        int message_len = retval - 2;

        printf("(%s:%d)로부터 (%d) 바이트 메시지 수신: %.*s\n", 
               client_ip, client_port, 
               message_len, message_len, message);

        char send_buf[BUFSIZE];
        memset(send_buf, 0, BUFSIZE);

        switch(opt) {
            case 0x0001:
                // Echo 동작: 받은 메시지를 그대로 반환
                send_buf[0] = 0x00;
                send_buf[1] = 0x01;
                memcpy(send_buf + 2, message, message_len);
                retval = sendto(sock, send_buf, 2 + message_len, 0, (SOCKADDR*)&clientaddr, clientaddr_len);
                if(retval == SOCKET_ERROR) {
                    fprintf(stderr, "sendto() echo error: %d\n", WSAGetLastError());
                } else {
                    printf("[echo] %d 바이트 전송\n", retval);
                }
                break;

            case 0x0002:
                // Chat 동작: 서버 운영자가 입력한 메시지를 클라이언트에게 전송
                {
                    char server_reply[BUFSIZE - 2];
                    printf("클라이언트 (%s:%d)에게 보낼 chat 메시지를 입력: ", client_ip, client_port);
                    fgets(server_reply, sizeof(server_reply), stdin);
                    // 개행 문자 제거
                    int len = strlen(server_reply);
                    if(len > 0 && server_reply[len-1] == '\n') {
                        server_reply[len-1] = '\0';
                    }

                    int reply_len = strlen(server_reply);
                    send_buf[0] = 0x00;
                    send_buf[1] = 0x02;
                    memcpy(send_buf + 2, server_reply, reply_len);
                    retval = sendto(sock, send_buf, 2 + reply_len, 0, (SOCKADDR*)&clientaddr, clientaddr_len);
                    if(retval == SOCKET_ERROR) {
                        fprintf(stderr, "sendto() chat error: %d\n", WSAGetLastError());
                    } else {
                        printf("[chat] %d 바이트 전송: %s\n", retval, server_reply);
                    }
                }
                break;

            case 0x0003:
                // Stat 동작: 통계 정보 전송
                {
                    char* stat_request = message;
                    char stat_response[BUFSIZE];
                    memset(stat_response, 0, BUFSIZE);
                    if(strncmp(stat_request, "bytes", 5) == 0) {
                        sprintf(stat_response, "Total bytes received: %lu", total_bytes);
                    }
                    else if(strncmp(stat_request, "number", 6) == 0) {
                        sprintf(stat_response, "Total messages received: %lu", total_messages);
                    }
                    else if(strncmp(stat_request, "both", 4) == 0) {
                        sprintf(stat_response, "Total messages received: %lu, Total bytes received: %lu", total_messages, total_bytes);
                    }
                    else {
                        sprintf(stat_response, "Invalid stat request.");
                    }

                    int stat_len = strlen(stat_response);
                    send_buf[0] = 0x00;
                    send_buf[1] = 0x03;
                    memcpy(send_buf + 2, stat_response, stat_len);
                    retval = sendto(sock, send_buf, 2 + stat_len, 0, (SOCKADDR*)&clientaddr, clientaddr_len);
                    if(retval == SOCKET_ERROR) {
                        fprintf(stderr, "sendto() stat error: %d\n", WSAGetLastError());
                    } else {
                        printf("[stat] %d 바이트 전송: %s\n", retval, stat_response);
                    }
                }
                break;

            case 0x0004:
                // Quit 동작: 서버 종료
                printf(" 서버를 종료.\n");
                closesocket(sock);
                WSACleanup();
                return EXIT_SUCCESS;

            default:
                printf("알 수 없는 동작 형식: 0x%04X\n", opt);
                break;
        }
    }

    // 소켓 닫기 및 윈속 종료 (반드시 도달하지 않음)
    closesocket(sock);
    WSACleanup();
    return 0;
}
