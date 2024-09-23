// UdpEchoClient.c
// UDP Echo Client with Chat Functionality

#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ws2tcpip.h>

#define BUFSIZE 1024



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
    SOCKADDR_IN serveraddr;
    char send_buf[BUFSIZE];
    char recv_buf[BUFSIZE];
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

    printf("UDP Echo Client is running. Server: %s:%d\n", server_ip, server_port);

    while(1) {
        char opt_input[10];
        unsigned short opt;
        char message[BUFSIZE - 2];

        // 동작 선택
        printf("\n opt선택 (echo,chat,stat,quit): ");
        scanf("%s", opt_input);

        // 동작 형식 코드 가져오기
        if(strcmp(opt_input, "echo") == 0) {
            opt = 0x0001;
        }
        else if(strcmp(opt_input, "chat") == 0) {
            opt = 0x0002;
        }
        else if(strcmp(opt_input, "stat") == 0) {
            opt = 0x0003;
        }
        else if(strcmp(opt_input, "quit") == 0) {
            opt = 0x0004;
        }
        else {
            printf("wrong opt. Please try again.\n");
            // 입력 버퍼 정리
            while(getchar() != '\n');
            continue;
        }

        // 메시지 입력 (quit은 메시지가 필요 없음)
        if(strcmp(opt_input, "quit") != 0) {
            printf("Enter message: ");
            getchar(); // 이전 입력 버퍼 제거
            fgets(message, sizeof(message), stdin);
            // 개행 문자 제거
            int len = strlen(message);
            if(len > 0 && message[len-1] == '\n') {
                message[len-1] = '\0';
            }
        }

        // 전송 버퍼 구성
        memset(send_buf, 0, BUFSIZE);
        send_buf[0] = (opt >> 8) & 0xFF; // 상위 바이트
        send_buf[1] = opt & 0xFF;        // 하위 바이트

        if(strcmp(opt_input, "quit") != 0) {
            memcpy(send_buf + 2, message, strlen(message));
        }

        // 메시지 전송
        int send_len = (strcmp(opt_input, "quit") == 0) ? 2 : 2 + strlen(message);
        retval = sendto(sock, send_buf, send_len, 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
        if(retval == SOCKET_ERROR) {
            fprintf(stderr, "sendto() error: %d\n", WSAGetLastError());
            continue;
        }
        printf("Sent %d bytes to server.\n", retval);

        // 'quit' 동작 시 클라이언트 종료
        if(strcmp(opt_input, "quit") == 0) {
            printf("Quit command sent. Exiting client.\n");
            break;
        }

        // 응답 수신
        memset(recv_buf, 0, BUFSIZE);
        retval = recvfrom(sock, recv_buf, BUFSIZE, 0, NULL, NULL);
        if(retval == SOCKET_ERROR) {
            fprintf(stderr, "recvfrom() error: %d\n", WSAGetLastError());
            continue;
        }

        // 응답 메시지 처리
        unsigned short response_opt = ((unsigned char)recv_buf[0] << 8) | (unsigned char)recv_buf[1];
        char* response_message = recv_buf + 2;
        int response_len = retval - 2;

      
        printf("(%s:%d)로부터 (%d) 바이트 메시지 수신: %.*s\n", 
              server_ip, server_port, 
               response_len, response_len, response_message);
    }

    // 소켓 닫기 및 윈속 종료
    closesocket(sock);
    WSACleanup();
    return 0;
}
