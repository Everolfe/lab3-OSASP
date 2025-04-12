#define _POSIX_C_SOURCE 199309L
#include "child_functions.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Константы для настройки работы */
#define STAT_INTERVAL_NS 100000000  /* Интервал между сигналами (100 мс) */
#define STAT_REPEAT 101             /* Частота вывода статистики */

/* Статистические данные */
pair_t stats = {0, 0};
size_t c00 = 0, c01 = 0, c10 = 0, c11 = 0;
size_t iteration_count = 0;
char child_name[CHILD_NAME_LENGTH] = {0};
volatile sig_atomic_t received_signal = false;
volatile sig_atomic_t state = WAITING;

/*
* Обработчик сигналов
*/
void child_signal_handler(int sig) 
{
    switch (sig) {
        case SIGALRM:
            received_signal = true;
            break;
        case SIGUSR1:
            state = PRINT_ALLOWED;
            break;
        case SIGUSR2:
            state = PRINT_FORBIDDEN;
            break;
    }
}

/*
* Обновление статистики на основе текущих значений
*/
void update_statistics(void) 
{
    if (stats.first == 0 && stats.second == 0) c00++;
    else if (stats.first == 0 && stats.second == 1) c01++;
    else if (stats.first == 1 && stats.second == 0) c10++;
    else if (stats.first == 1 && stats.second == 1) c11++;
}

/*
* Обновление пар статистики по циклу 00->01->10->11
*/
void update_stats_cycle(void) 
{
    static int cycle_pos = 0;
    
    switch (cycle_pos) {
        case 0: stats.first = 0; stats.second = 0; break;
        case 1: stats.first = 0; stats.second = 1; break;
        case 2: stats.first = 1; stats.second = 0; break;
        case 3: stats.first = 1; stats.second = 1; break;
    }
    
    cycle_pos = (cycle_pos + 1) % 4;
}

/*
* Безопасный вывод строки посимвольно
*/
void print_safe(const char* str) 
{
    for (; *str; str++) {
        if (putchar(*str) == EOF) {
            perror("putchar failed");
            break;
        }
    }
    fflush(stdout);
}

/*
* Формирование и вывод статистики
*/
void print_statistics(void) 
{
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer),
        "[%s pid: %d ppid: %d] stats: 00=%zu 01=%zu 10=%zu 11=%zu\n",
        child_name, getpid(), getppid(), c00, c01, c10, c11);
    
    if (len > 0 && len < (int)sizeof(buffer)) {
        print_safe(buffer);
    }

    /* Сброс статистики */
    c00 = c01 = c10 = c11 = 0;
    
    /* Уведомление родителя */
    if (kill(getppid(), SIGUSR2) == -1) {
        perror("failed to send SIGUSR2 to parent");
    }
}

/*
* Запрос разрешения на вывод у родителя
*/
void request_output_permission(void) 
{
    if (kill(getppid(), SIGUSR1) == -1) {
        perror("failed to request output permission");
        return;
    }
    state = WAITING;
}

/*
* Инициализация дочернего процесса
*/
void init_child(void) 
{
    /* Формирование имени процесса */
    if (snprintf(child_name, CHILD_NAME_LENGTH, "child_%d", getpid()) < 0) {
        perror("failed to create child name");
        _exit(EXIT_FAILURE);
    }
    
    /* Настройка обработчиков сигналов */
    struct sigaction sa;
    sa.sa_handler = child_signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGALRM, &sa, NULL) == -1 ||
        sigaction(SIGUSR1, &sa, NULL) == -1 ||
        sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("failed to set signal handlers");
        _exit(EXIT_FAILURE);
    }
}

/*
* Главный рабочий цикл дочернего процесса
*/
void child_main_loop(void) 
{
    const struct timespec interval = {0, STAT_INTERVAL_NS};
    
    while (1) {
        /* Задержка перед следующим циклом */
        if (nanosleep(&interval, NULL) == -1) {
            if (errno != EINTR) {
                perror("nanosleep failed");
                break;
            }
            continue;
        }
        
        /* Обновление статистики */
        update_stats_cycle();
        update_statistics();
        iteration_count++;
        
        /* Периодический запрос на вывод */
        if (iteration_count % STAT_REPEAT == 0) {
            request_output_permission();
            
            /* Ожидание разрешения */
            while (state == WAITING) {
                pause();
            }
            
            /* Вывод если разрешено */
            if (state == PRINT_ALLOWED) {
                print_statistics();
            }
            
            state = WAITING;
        }
    }
}