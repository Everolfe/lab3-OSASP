/*
 * Файл: parent_functions.c
 * Реализация функций родительского процесса
 * Управление дочерними процессами через сигналы
 */
#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 199309L
#include "parent_functions.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#ifndef SA_RESTART
#define SA_RESTART 0x10000000
#endif

/* Глобальные переменные */
size_t num_child_processes = 0;
size_t max_child_processes = MAX_CHILDREN;
process_info_t* child_processes = NULL;

/*
* Обработчик сигнала таймера
* Принимает: sig - номер сигнала
* Возвращает: ничего
*/
void alarm_handler(int sig) {
    if (sig == SIGALRM) {
        enable_all_output();
        printf("Timeout expired. Enabled output for all children.\n");
    }
}

/*
* Обработчик сигналов от дочерних процессов
* Принимает: sig - номер сигнала
*            info - информация о сигнале
*            context - контекст (не используется)
* Возвращает: ничего
*/
void parent_signal_handler(int sig, siginfo_t* info, void* context) {
    (void)context;
    
    for (size_t i = 0; i < num_child_processes; i++) {
        if (child_processes[i].pid == info->si_pid) {
            if (sig == SIGUSR1) {
                if (child_processes[i].is_stopped) {
                    kill(info->si_pid, SIGUSR2);
                } else {
                    kill(info->si_pid, SIGUSR1);
                }
            }
            else if (sig == SIGUSR2) {
                printf("C_%d finished output\n", info->si_pid);
            }
            return;
        }
    }
}

/*
* Инициализация родительского процесса
* Принимает: ничего
* Возвращает: ничего
*/
void init_parent() {
    child_processes = (process_info_t*)malloc(max_child_processes * sizeof(process_info_t));
    if (!child_processes) {
        perror("Failed to allocate memory for child processes");
        exit(EXIT_FAILURE);
    }
    
    for (size_t i = 0; i < max_child_processes; i++) {
        child_processes[i].pid = 0;
        child_processes[i].is_stopped = false;
        memset(child_processes[i].name, 0, CHILD_NAME_LENGTH);
    }
    
    struct sigaction sa;
    sa.sa_sigaction = parent_signal_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1 ||
        sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Failed to set signal handlers");
        exit(EXIT_FAILURE);
    }
    
    signal(SIGALRM, alarm_handler);
}

/*
* Завершение работы родительского процесса
* Принимает: ничего
* Возвращает: ничего
*/
void cleanup_parent() {
    alarm(0);
    remove_all_children();
    free(child_processes);
    child_processes = NULL;
}


/*
* Создание нового дочернего процесса
* Принимает: ничего
* Возвращает: ничего
*/
void create_child() {
    if (num_child_processes >= max_child_processes) {
        printf("Maximum number of children (%zu) reached\n", max_child_processes);
        return;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        perror("Failed to fork");
        return;
    }
    
    if (pid == 0) {
        execl("./child", "./child", NULL);
        perror("Failed to exec child");
        _exit(EXIT_FAILURE);
    }
    else {
        child_processes[num_child_processes].pid = pid;
        child_processes[num_child_processes].is_stopped = false;
        snprintf(child_processes[num_child_processes].name, CHILD_NAME_LENGTH, 
                "C_%zu", num_child_processes + 1);
        
        num_child_processes++;
        printf("Created C_%zu (PID: %d)\n", num_child_processes, pid);
    }
}

/*
* Удаление последнего дочернего процесса
* Принимает: ничего
* Возвращает: ничего
*/
void remove_last_child() {
    if (num_child_processes == 0) {
        printf("No children to remove\n");
        return;
    }
    
    size_t index = num_child_processes - 1;
    pid_t pid = child_processes[index].pid;
    
    if (kill(pid, SIGTERM) == -1) {
        perror("Failed to send SIGTERM");
        return;
    }
    
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("Failed to wait for child");
    }
    
    num_child_processes--;
    printf("Removed C_%zu (PID: %d). %zu children remaining\n", 
        index, pid, num_child_processes);
}

/*
* Удаление всех дочерних процессов
* Принимает: ничего
* Возвращает: ничего
*/
void remove_all_children() {
    if (num_child_processes == 0) {
        printf("No children to remove\n");
        return;
    }
    
    printf("Removing all %zu C_XX...\n", num_child_processes);
    for (size_t i = 0; i < num_child_processes; i++) {
        if (child_processes[i].pid > 0) {
            kill(child_processes[i].pid, SIGTERM);
            waitpid(child_processes[i].pid, NULL, 0);
        }
    }
    
    num_child_processes = 0;
    printf("All children removed\n");
}

/*
* Просмотр дочерних процессов
* Принимает: ничего
* Возвращает: ничего
*/
void list_processes() {
    printf(SEPARATE);
    printf("Parent PID: %d\n", getpid());
    printf("C_processes (%zu):\n", num_child_processes);
    
    for (size_t i = 0; i < num_child_processes; i++) {
        printf("%zu. %s (PID: %d, %s)\n", 
            i + 1, 
            child_processes[i].name, 
            child_processes[i].pid,
            child_processes[i].is_stopped ? "stopped" : "running");
    }
    printf(SEPARATE);
}

/*
* Остановка работы всех дочерних процессов
* Принимает: ничего
* Возвращает: ничего
*/
void disable_all_output() {
    for (size_t i = 0; i < num_child_processes; i++) {
        child_processes[i].is_stopped = true;
    }
    printf("Disabled output for all children\n");
}

/*
* Включение работы всех дочерних процессов
* Принимает: ничего
* Возвращает: ничего
*/
void enable_all_output() {
    for (size_t i = 0; i < num_child_processes; i++) {
        child_processes[i].is_stopped = false;
    }
    printf("Enabled output for all children\n");
}

/*
* Отключение работы конкретного дочернего процесса
* Принимает: номер процесса
* Возвращает: ничего
*/
void disable_child_output(int child_num) {
    if (child_num > 0 && child_num <= (int)num_child_processes) {
        child_processes[child_num-1].is_stopped = true;
        printf("Disabled output for child %d (PID: %d)\n", 
            child_num, child_processes[child_num-1].pid);
    } else {
        printf("Invalid child number: %d\n", child_num);
    }
}

/*
* Включение работы конкретного дочернего процесса
* Принимает: номер процесса
* Возвращает: ничего
*/
void enable_child_output(int child_num) {
    if (child_num > 0 && child_num <= (int)num_child_processes) {
        child_processes[child_num-1].is_stopped = false;
        printf("Enabled output for child %d (PID: %d)\n", 
            child_num, child_processes[child_num-1].pid);
    } else {
        printf("Invalid child number: %d\n", child_num);
    }
}

void request_child_output(int child_num) {
    if (child_num > 0 && child_num <= (int)num_child_processes) {
        disable_all_output();
        enable_child_output(child_num);
        
        alarm(20);
        printf("Requested output from child %d. You have 20 seconds...\n", child_num);
    } else {
        printf("Invalid child number: %d\n", child_num);
    }
}

/*
* Обработка ввода пользователя
* Принимает: ввденную строку
* Возвращает: ничего
*/
void handle_user_input(const char* input) {
    if (strlen(input) == 0) return;
    
    switch (input[0]) {
        case '+':
            create_child();
            break;
        case '-':
            remove_last_child();
            break;
        case 'l':
            list_processes();
            break;
        case 'k':
            remove_all_children();
            break;
        case 's':
            if (strlen(input) > 1 && isdigit(input[1])) {
                disable_child_output(atoi(&input[1]));
            } else {
                disable_all_output();
            }
            break;
        case 'g':
            if (strlen(input) > 1 && isdigit(input[1])) {
                enable_child_output(atoi(&input[1]));
            } else {
                enable_all_output();
            }
            break;
        case 'p':
            if (strlen(input) > 1 && isdigit(input[1])) {
                request_child_output(atoi(&input[1]));
            } else {
                printf("Usage: p<num> (e.g. p1)\n");
            }
            break;
        case 'q':
            cleanup_parent();
            exit(EXIT_SUCCESS);
        default:
            printf("Unknown command: %s\n", input);
            printf("Available commands:\n");
            printf("  + : Create child\n  - : Remove last child\n");
            printf("  l : List processes\n  k : Kill all children\n");
            printf("  s : Disable all output\n  g : Enable all output\n");
            printf("  s<num> : Disable output for child <num>\n");
            printf("  g<num> : Enable output for child <num>\n");
            printf("  p<num> : Request output from child <num>\n");
            printf("  q : Quit\n");
    }
}

/*
* Основной цикл программы
* Принимает: ничего
* Возвращает: ничего
*/
void parent_main_loop() {
    char input[32];
    
    printf("Parent process started. PID: %d\n", getpid());
    printf("Type 'help' for available commands\n");
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin)) {
            input[strcspn(input, "\n")] = '\0';
            
            if (strcmp(input, "help") == 0) {
                handle_user_input("?");
            } else {
                handle_user_input(input);
            }
        } else {
            clearerr(stdin);
            printf("\n");
        }
    }
}

void print_status(const char* message) {
    printf("[PARENT %d] %s\n", getpid(), message);
}