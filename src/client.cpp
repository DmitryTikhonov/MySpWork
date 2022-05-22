#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <pthread.h>

#define SERVERPORT 8080
#define TRUE 1
#define BUF_SIZE 1024

typedef struct
{
    int socketClient;
    struct sockaddr_in socketAddress;
} PthreadData;

pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;

int createSocket()
{ // создание IPv4 сокета
    int newSocket;
    int opt = TRUE;
    if ((newSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << "ошибка создания сокета";
        exit(EXIT_FAILURE);
    }
    else
    {
        if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
        {
            std::cout << "произошла ошибка изменения параметров сокета" << std::endl;
            exit(EXIT_FAILURE);
        }
        return newSocket;
    }
}

struct sockaddr_in defineAddress(int PORTNUMBER)
{ // определяет адрес в заданном порту
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORTNUMBER);
    // inet_aton(ip, &address.sin_addr);
    return address;
}

void connectToServer(int anySocket, struct sockaddr_in anyAddress)
{
    if (connect(anySocket, (struct sockaddr *)&anyAddress, sizeof(anyAddress)) < 0)
    {
        std::cerr << "ошибка подключения №2";
        exit(EXIT_FAILURE);
    }

    std::cout << "Направление на сервер ..." << std::endl;
}

void *write(void *inputData)
{
    int socketClient;
    PthreadData *data = (PthreadData *)inputData;
    socketClient = data->socketClient;
    for (;;)
    {
        char buffer[BUF_SIZE];

        fflush(stdout);
        ssize_t s = read(0, buffer, sizeof(buffer) - 1);

        buffer[s - 1] = '\0';
        write(socketClient, buffer, strlen(buffer));

        // send(socketClient, buffer, strlen(buffer), 0);
    }
    close(socketClient);
}

void *read(void *inputData)
{
    int socketClient;
    PthreadData *data = (PthreadData *)inputData;
    socketClient = data->socketClient;
    char buffer[BUF_SIZE];
    bzero(buffer, BUF_SIZE);
    bool loop = false;

    while (!loop)
    {
        bzero(buffer, BUF_SIZE);

        int rc = read(socketClient, buffer, BUF_SIZE);
        if (rc > 0)
        {
            std::string tester(buffer);
            std::cout << ":" << tester << std::endl;

            if (tester == "exit_server")
                break;
        }
    }
    std::cout << "\nClosing thread and conn" << std::endl;
    close(socketClient);
    pthread_exit(NULL);
}

int main()
{
    char buffer[BUF_SIZE];
    pthread_t thread[2];
    PthreadData data;
    int socketClient = createSocket();
    struct sockaddr_in socketAddress = defineAddress(SERVERPORT);

    connectToServer(socketClient, socketAddress);

    while (1)
    {
        data.socketAddress = socketAddress;
        data.socketClient = socketClient;

        pthread_create(&thread[0], NULL, write, &data);
        pthread_create(&thread[1], NULL, read, &data);

        pthread_join(thread[1], NULL);
        pthread_join(thread[0], NULL);
    }
}