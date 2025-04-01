/*
 * Файл: globals.h
 * Общие определения и структуры данных для родительского и дочернего процессов
 * Содержит константы, типы данных и объявления глобальных переменных
 */

 #ifndef COMMON_H
 #define COMMON_H
 
 #include <stdbool.h>
 #include <sys/types.h>
 #include <stdio.h>
 #include <signal.h>
 /* Максимальное количество дочерних процессов */
 #define MAX_CHILDREN 8
 
 /* Длина имени дочернего процесса */
 #define CHILD_NAME_LENGTH 16
 
 /* Разделитель для вывода информации */
 #define SEPARATE "-------------------------------------------------------\n"
 
 /* Структура для хранения информации о процессе */
 typedef struct process_info_s {
     pid_t pid;                /* Идентификатор процесса */
     bool is_stopped;          /* Флаг остановки вывода */
     char name[CHILD_NAME_LENGTH]; /* Имя процесса */
 } process_info_t;
 
 /* Структура для хранения пары значений */
 typedef struct pair_s {
     int first;                /* Первое значение */
     int second;               /* Второе значение */
 } pair_t;
 
 /* Состояния дочернего процесса */
 typedef enum {
     WAITING,                  /* Ожидание разрешения */
     PRINT_ALLOWED,            /* Вывод разрешен */
     PRINT_FORBIDDEN           /* Вывод запрещен */
 } child_state_t;
 

 extern pair_t stats;          /* Текущие значения пары */
 extern size_t c00;            /* Счетчик комбинации 00 */
 extern size_t c01;            /* Счетчик комбинации 01 */
 extern size_t c10;            /* Счетчик комбинации 10 */
 extern size_t c11;            /* Счетчик комбинации 11 */
 extern volatile sig_atomic_t received_signal; /* Флаг получения сигнала */
 extern volatile sig_atomic_t state;          /* Текущее состояние */
 extern size_t num_child_processes; /* Количество дочерних процессов */
 extern size_t max_child_processes; /* Максимальное количество процессов */
 extern process_info_t *child_processes; /* Массив информации о процессах */
 extern char child_name[CHILD_NAME_LENGTH]; /* Имя текущего процесса */
 
 #endif /* COMMON_H */