#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sys/select.h>
#include <cstring>

#define N (sizeof(fd_set) * 8)
#define TRUE 1
#define SERVERPORT 8080
#define MAXCLIENTS 30
#define BUF_SIZE 1024

int fd_array[N];
unsigned int clientCount = 0;

struct myClient
{ // Структура клиентов для хранения информации
    char username[BUF_SIZE];
    int socketID;
    int index;
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
};

struct myClient Client[MAXCLIENTS];

int SearchByLine(char buffer[BUF_SIZE])
{
    int n, k = strlen(buffer);
    for (int i = 0; i < k; i++)
    {
        if (buffer[i] == ':')
        {
            n = i + 1;
            break;
        }
    }
    return n;
}

int createSocket()
{ // создание сокета
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

void bindSocket(int anySocket, struct sockaddr_in anyAddress)
{ // привязывает сокет к заданному порту
    if (bind(anySocket, (struct sockaddr *)&anyAddress, sizeof(anyAddress)) < 0)
    {
        std::cerr << "bind";
        exit(EXIT_FAILURE);
    }
}

void listenForConnections(int anySocket)
{ // прослушивание подключений из данного сокета
    if (listen(anySocket, SOMAXCONN) < 0)
    {
        std::cerr << "listen";
        exit(EXIT_FAILURE);
    }
}

void exitserver(int fd_array[N])
{
    for (unsigned int i; i < N; i++)
    {
        if (fd_array[i] > 0)
        {
            close(fd_array[i]);
            fd_array[i] = -1;
        }
    }
}

int main()
{

    int socketServer = createSocket();
    struct sockaddr_in serverAddress = defineAddress(SERVERPORT);
    bindSocket(socketServer, serverAddress);
    listenForConnections(socketServer);

    unsigned int index = 0;
    for (; index < N; ++index)
    {
        fd_array[index] = -1;
    }
    fd_array[0] = socketServer;
    int max_fd = -1;

    fd_set my_fd_set;

    std::cout << "Ожидание подключений..." << std::endl;

    while (1)
    {
        FD_ZERO(&my_fd_set);

        unsigned int i = 0;
        for (; i < N; i++)
        {
            if (fd_array[i] > 0)
            {
                FD_SET(fd_array[i], &my_fd_set);
                if (fd_array[i] > max_fd)
                {
                    max_fd = fd_array[i];
                }
            }
        }
        int flag = select(max_fd + 1, &my_fd_set, NULL, NULL, 0);

        switch (flag)
        {
        case 0:
            printf("timeout...\n");
            break;
        case -1:
            printf("select error...\n");
            break;
        default:
        {
            for (i = 0; i < N; i++)
            {
                if (fd_array[i] < 0)
                {
                    continue;
                }

                // Find a valid FD to operate
                if (i == 0 && FD_ISSET(socketServer, &my_fd_set))
                {
                    int new_fd = accept(socketServer, (struct sockaddr *)&Client[clientCount].clientAddr, &Client->len);
                    if (new_fd < 0)
                    {
                        continue;
                    }
                    char *ip = inet_ntoa(Client[clientCount].clientAddr.sin_addr);
                    int port = ntohs(Client[clientCount].clientAddr.sin_port);

                    printf("Подключился новый клиент %s:%d\n", ip, port);
                    clientCount++;

                    unsigned int j = 1;
                    for (; j < N; j++)
                    {
                        if (fd_array[j] == -1)
                            break;
                    }
                    if (j == N)
                    {
                        printf("server is full\n");
                        close(new_fd);
                    }
                    else
                    {
                        fd_array[j] = new_fd;
                        Client[i].socketID = new_fd;
                    }
                }
                else if (i > 0 && FD_ISSET(fd_array[i], &my_fd_set)) // If it is a normal socket
                {
                    // Read Client Data
                    char buf[BUF_SIZE];
                    //char username[BUF_SIZE];
                    char massage[BUF_SIZE];

                    bzero(buf, BUF_SIZE);
                    bzero(Client[i - 1].username, BUF_SIZE);
                    bzero(massage, BUF_SIZE);

                    ssize_t s = read(fd_array[i], buf, sizeof(buf) - 1);
                    if (s > 0)
                    {
                        buf[s] = '\0';
                        int count = SearchByLine(buf) - 1;

                        strncpy(Client[i - 1].username, buf, count);
                        strcpy(massage, &buf[count + 1]);

                        std::string tester(massage);

                        printf("%s[%s:%d] - %s\n", Client[i - 1].username, inet_ntoa(Client[i - 1].clientAddr.sin_addr), ntohs(Client[i - 1].clientAddr.sin_port), massage);

                        if (tester == "exit_server")
                        {
                            for (unsigned int h = 1; h <= clientCount; h++)
                            {
                                write(fd_array[h], buf, strlen(buf));
                            }
                            exitserver(fd_array);
                        }
                        for (unsigned int h = 1; h <= clientCount; h++)
                        {
                            if (h != i)
                                write(fd_array[h], buf, strlen(buf));
                        }
                    }
                    else if (s == 0)
                    {
                        printf("%s quits\n", Client[i - 1].username);
                        clientCount--;
                        close(fd_array[i]);
                        fd_array[i] = -1;
                    }
                    else
                    {
                        fprintf(stderr, "read error\n");
                    }
                }
                else
                {
                }
            }

        } // default
        break;
        }
    }

    return 0;
}
