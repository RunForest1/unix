#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/select.h>
#include <string.h>

#define PORT 3457

volatile sig_atomic_t signal_flag = 0;

void signal_handler(int signum)
{
    signal_flag = 1;
}

int main()
{
    int client_fd = -1;
    struct sockaddr_in server_address;
    fd_set read_fds;
    sigset_t blocked_mask, original_mask, empty_mask;
    struct sigaction signal_action;
    char buffer[256];

    // Создание сокета
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int option = 1;
    // Переиспользование порта
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
    {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d (PID=%d)\n", PORT, getpid());

    // Подготовка маски сигналов
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);

    // БЛОКИРУЕМ SIGHUP перед установкой обработчика
    if (sigprocmask(SIG_BLOCK, &blocked_mask, &original_mask) == -1)
    {
        perror("sigprocmask");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Настройка обработчика сигнала (теперь безопасно)
    memset(&signal_action, 0, sizeof(signal_action));
    signal_action.sa_handler = signal_handler;
    signal_action.sa_flags = 0;
    sigemptyset(&signal_action.sa_mask);

    if (sigaction(SIGHUP, &signal_action, NULL) == -1)
    {
        perror("sigaction");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (sigprocmask(SIG_SETMASK, &original_mask, NULL) == -1)
    {
        perror("sigprocmask");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    sigemptyset(&empty_mask);

    signal_action.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &signal_action, NULL);

    while (1)
    {

        if (signal_flag)
        {
            printf("Received SIGHUP signal\n");
            signal_flag = 0;
        }

        // Подготовка множества дескрипторов
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        if (client_fd != -1)
        {
            FD_SET(client_fd, &read_fds);
            if (client_fd > max_fd)
            {
                max_fd = client_fd;
            }
        }

        // pselect с временной разблокировкой ВСЕХ сигналов
        if (pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &empty_mask) == -1)
        {
            if (errno == EINTR)
            {
                if (signal_flag)
                {
                    printf("Received SIGHUP signal\n");
                    signal_flag = 0;
                }
                continue;
            }
            perror("pselect");
            break;
        }

        // Обработка новых подключений
        if (FD_ISSET(server_fd, &read_fds))
        {
            int new_client = accept(server_fd, NULL, NULL);
            if (new_client == -1)
            {
                if (errno != EINTR)
                {
                    perror("accept");
                }
                continue;
            }

            printf("New connection accepted (fd=%d)\n", new_client);

            // Закрываем предыдущего клиента, если он есть
            if (client_fd != -1)
            {
                printf("Closing previous connection (fd=%d)\n", client_fd);
                close(client_fd);
            }
            client_fd = new_client;
        }

        if (client_fd != -1 && FD_ISSET(client_fd, &read_fds))
        {
            ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_read > 0)
            {
                printf("Received %zd bytes\n", bytes_read);
            }
            else if (bytes_read == 0)
            {
                printf("Client disconnected\n");
                close(client_fd);
                client_fd = -1;
            }
            else
            {
                // Ошибка при чтении
                perror("recv");
                close(client_fd);
                client_fd = -1;
            }
        }
    }

    // Очистка ресурсов при завершении
    if (client_fd != -1)
    {
        close(client_fd);
    }
    close(server_fd);

    return 0;
}