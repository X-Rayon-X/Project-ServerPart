#pragma comment(lib, "Ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <time.h>

struct ThreadArgs {
    int* array;
    int array_size;
    int max_array;
};

DWORD WINAPI MaxArray(LPVOID lpParam) {
    struct ThreadArgs* threadArgs = (struct ThreadArgs*)lpParam;
    int* array = threadArgs->array;
    int size = threadArgs->array_size;
    int maxArray = array[0];

    for (int i = 0; i < size; i++) {
        if (array[i] > maxArray) {
            maxArray = array[i];
        }
    }

    threadArgs->max_array = maxArray;

    return 0;
}

int main(void)
{
    srand(time(NULL));
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    printf("TCP Сервер \n");
    printf("___________\n\n");
    WSADATA wsaData;
    SOCKET ListeningSocket;
    SOCKET NewConnection;
    struct sockaddr_in ServerAddr;
    struct sockaddr_in ClientAddr;
    int ClientAddrLen;
    u_short Port = 5150;
    int Ret;
    char DataBuffer[1024];

    if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        printf("Помилка WSAStartup, номер помилки %d\n", Ret);
        return -1;
    }

    if ((ListeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    {
        printf("Помилка socket, номер помилки %d\n", WSAGetLastError());
        WSACleanup();
        return -2;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(ListeningSocket, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
    {
        printf("Помилка bind, номер помилки %d\n", WSAGetLastError());
        closesocket(ListeningSocket);
        WSACleanup();
        return -3;
    }

    if (listen(ListeningSocket, 5) == SOCKET_ERROR)
    {
        printf("Помилка listen, номер помилки %d\n", WSAGetLastError());
        closesocket(ListeningSocket);
        WSACleanup();
        return -4;
    }

    printf("Чекаємо з'єднання на порту %d.\n", Port);
    ClientAddrLen = sizeof(ClientAddr);

    if ((NewConnection = accept(ListeningSocket, (struct sockaddr*)&ClientAddr,
        &ClientAddrLen)) == INVALID_SOCKET)
    {
        printf("Помилка accept, номер помилки %d\n", WSAGetLastError());
        closesocket(ListeningSocket);
        WSACleanup();
        return -5;
    }

    printf("Успішно з'єдналися з %s:%d.\n", inet_ntoa(ClientAddr.sin_addr),
        ntohs(ClientAddr.sin_port));

    closesocket(ListeningSocket);
    printf("Чекаємо дані для отримання...\n");

    if ((Ret = recv(NewConnection, DataBuffer, sizeof(DataBuffer), 0)) == SOCKET_ERROR)
    {
        printf("Помилка recv, номер помилки %d\n", WSAGetLastError());
        closesocket(NewConnection);
        WSACleanup();
        return -6;
    }

    if (Ret <= 1023)
        DataBuffer[Ret] = '\0';
    else
    {
        printf("Повідомлення задовге!\n");
        return -7;
    }
    DataBuffer[Ret] = '\0';
    printf("Успішно отримано %d байтів в повідомленні, кількість значень \"%s\".\n", Ret, DataBuffer);

    int* dynamicArray = (int*)malloc((int)atoi(DataBuffer) * sizeof(int));
    for (int i = 0; i < (int)atoi(DataBuffer); i++) {
        dynamicArray[i] = rand() % 100;
    }

    printf("\nOriginal array:\n");
    char msg[1024];
    char tempBuffer[32];

    // Починаємо формувати повідомлення
    strcpy(msg, "\nOriginal array: ");

    for (int i = 0; i < (int)atoi(DataBuffer); i++) {
        printf("%d ", dynamicArray[i]);
        sprintf(tempBuffer, "%d ", dynamicArray[i]);
        strcat(msg, tempBuffer);
    }

    strcat(msg, "\n");

    struct ThreadArgs threadArgs;
    threadArgs.array = dynamicArray;
    threadArgs.array_size = (int)atoi(DataBuffer);
    threadArgs.max_array = dynamicArray[0];

    HANDLE hThread;
    DWORD dwThreadId;

    hThread = CreateThread(
        NULL,
        0,
        MaxArray,
        &threadArgs,
        0,
        &dwThreadId);

    if (hThread == NULL) {
        fprintf(stderr, "Error creating thread (%lu).\n", GetLastError());
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);

    printf("\nMax array: %d\n", threadArgs.max_array);

    printf("\nThread ID: %lu\n", dwThreadId);
    printf("Thread Handle: %p\n\n", hThread);
    printf("Main process ID: %lu\n", GetCurrentProcessId());
    printf("Main thread ID: %lu\n", GetCurrentThreadId());
    printf("Main process Handle: %p\n", GetCurrentProcess());
    printf("Main thread Handle: %p\n", GetCurrentThread());

    char sendBuffer[1024];
    sprintf_s(sendBuffer, "%s\nMax array: %d\nThread ID: %lu\nThread Handle: %p\nMain process ID: %lu\nMain thread ID: %lu\nMain process Handle: %p\nMain thread Handle: %p\n",
        msg, threadArgs.max_array, dwThreadId, hThread, GetCurrentProcessId(), GetCurrentThreadId(), GetCurrentProcess(), GetCurrentThread());

    if ((Ret = send(NewConnection, sendBuffer, strlen(sendBuffer), 0)) == SOCKET_ERROR)
    {
        printf("Помилка send, номер помилки %d\n", WSAGetLastError());
        closesocket(NewConnection);
        WSACleanup();
        return -8;
    }

    printf("Успішно передано %d байт повідомлення: %s.\n\n", Ret, sendBuffer);
    printf("Закриваємо з'єднання з клієнтом.\n");
    closesocket(NewConnection);
    WSACleanup();
    printf("Натисніть Enter для завершення.\n");
    getchar();

    CloseHandle(hThread);
    free(dynamicArray);

    getchar();
    getchar();
    return 0;
}
