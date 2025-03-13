#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <sys/types.h> 
#include <pthread.h>
#include <cstdlib>
#include <queue>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")

char response1[] =
"HTTP/1.1 200 OK\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html><html><head><title>Bye-bye baby bye-bye</title>"
"<style>body { background-color: #111 }"
"h1 { font-size:2cm; text-align: center; color: white;"
" text-shadow: 0 0 2mm red}</style></head>"
"<body><h1>Request number ";
char response2[] = "</h1></body></html>\r\n";
static int request_number = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
std::queue<int> client_queue;

void err_exit(int code, const char buff[]) {
    cout << endl << buff;
    exit(code);
}

void* serve_client(int client_fd) {
    request_number += 1;
    int iResult = send(client_fd, response1, sizeof(response1) - 1, NULL); /*-1:'\0'*/
    if (iResult == SOCKET_ERROR) {
        wprintf(L"send failed with error: %d, socket: %d\n", WSAGetLastError(), client_fd);
        closesocket(client_fd);
        return NULL;
    }
    std::string s = std::to_string(request_number);
    s.append(" has been processed");
    FILE* pipe = _popen("php version.php", "r");
    if (!pipe) {
        std::cerr << "Error occured when starting PHP" << std::endl;
        return NULL;
    }
    char result[128];
    std::string phpVersion;
    while (fgets(result, sizeof(result), pipe) != nullptr) {
        phpVersion += result;
    }
    _pclose(pipe);
    s.append("<br>PHP version: ");
    s += phpVersion;
    const char* pchar = s.c_str();
    iResult = send(client_fd, pchar, strlen(pchar), NULL); /*-1:'\0'*/
    if (iResult == SOCKET_ERROR) {
        wprintf(L"send failed with error: %d, socket: %d\n", WSAGetLastError(), client_fd);
        closesocket(client_fd);
        return NULL;
    }
    //Sleep(1);
    iResult = send(client_fd, response2, sizeof(response2) - 1, NULL); /*-1:'\0'*/
    if (iResult == SOCKET_ERROR) {
        wprintf(L"send failed with error: %d, socket: %d\n", WSAGetLastError(), client_fd);
        closesocket(client_fd);
        return NULL;
    }
    s.clear();
    //phpVersion.clear();
    Sleep(1);
    shutdown(client_fd, SD_SEND);
    closesocket(client_fd);
    return NULL;
}

void* thread_job(void* arg)
{
    while (true) {
        int thread_n = (int)arg;
        // Ожидаем сигнал о пуступлении клиента
        int err = pthread_mutex_lock(&mutex);
        if (err != 0)
            err_exit(err, "Cannot lock mutex");

        err = pthread_cond_wait(&cond, &mutex);
        if (err != 0)
            err_exit(err, "Cannot wait on condition variable");
        // Получаем дескриптор клиента из очереди
        int client_fd = client_queue.front();
        client_queue.pop();
        // Разблокируем мьютекс
        err = pthread_mutex_unlock(&mutex);
        if (err != 0)
            err_exit(err, "Cannot lock mutex");
        // Обслуживаем клиента
        serve_client(client_fd);
    }
    return NULL;
}


int main()
{
    int retval, err;
    int one = 1;
    struct sockaddr_in svr_addr, cli_addr;
    int sin_len = sizeof(cli_addr);

    err = pthread_mutex_init(&mutex, NULL);
    if (err != 0)
        err_exit(err, "Cannot initialize mutex");
    err = pthread_cond_init(&cond, NULL);
    if (err != 0)
        err_exit(err, "Cannot initialize condition variable");

    //---------------------------------------
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"Error at WSAStartup()\n");
        return 1;
    }
    //---------------------------------------
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        err_exit(1, "can't open socket");
    cout << "Listen socket created" << endl;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(char*));

    int port = 12435;
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&svr_addr, sizeof(svr_addr)) == -1) {
        closesocket(sock);
        WSACleanup();
        err_exit(1, "Can't bind");
    }

    listen(sock, 5);
    cout << "Listening..." << endl;
    SOCKET client_fd = INVALID_SOCKET;

    // Thread pull
    int threads_number = 100;
    pthread_t* thread_stack = new pthread_t[threads_number];
    int threads_end = 0;
    for (size_t n = 0; n < threads_number; n++) {
        // Создание потока
        int err = pthread_create(&thread_stack[n], NULL, thread_job, (int*)n);
        // Если при создании потока произошла ошибка, выводим
        // сообщение об ошибке и прекращаем работу программы
        if (err != 0) {
            cout << "Не получилось создать поток: " << err << endl;
            exit(-1);
        }
    }

    while (1) {
        // Принимаем клиента 
        client_fd = accept(sock, (struct sockaddr*)&cli_addr, &sin_len);
        //printf("got connection\n");
        if (client_fd < 1) {
            wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
            closesocket(client_fd);
            continue;
        }
        // Захватываем поток для синхронного доступа к очереди клиентов
        int err = pthread_mutex_lock(&mutex);
        if (err != 0)
            err_exit(err, "Cannot lock mutex");
        
        client_queue.push(client_fd);

        // Дадим сигнал о поступлении клиента и разблокируем мьютекс
        err = pthread_cond_signal(&cond);
        if (err != 0)
            err_exit(err, "Cannot send signal");
        err = pthread_mutex_unlock(&mutex);
        if (err != 0)
            err_exit(err, "Cannot unlock mutex");
    }
    pthread_exit(NULL);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    closesocket(sock);
    WSACleanup();
}