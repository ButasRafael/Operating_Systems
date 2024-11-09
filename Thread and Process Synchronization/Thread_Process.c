#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "a2_helper.h"
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <semaphore.h>

#define NUM_THREADS_P5_AND_P8 4
#define NUM_THREADS_P9 43
#define SEM_NAME_1 "/sem_1"
#define SEM_NAME_2 "/sem_2"

pthread_mutex_t mutex, mutex1;
pthread_cond_t cond_start, cond_end;
int start = 0, end = 0;
int sem_id;
int th_no = 0;
int done = 0;
sem_t *sem_1, *sem_2;

void *thread_function_P5(void *arg)
{

    int thread_id = *((int *)arg);
    int process_no = 5;
    int thread_no = thread_id;

    pthread_mutex_lock(&mutex);

    if (thread_id == 2)
    {
        sem_wait(sem_1);
    }

    if (thread_id == 4)
    {
        start = 1;
        pthread_cond_signal(&cond_start);
    }
    else if (thread_id == 1)
    {
        while (!start)
        {
            pthread_cond_wait(&cond_start, &mutex);
        }
    }
    info(BEGIN, process_no, thread_no);
    if (thread_id == 1)
    {
        info(END, process_no, thread_no);
        end = 1;
        pthread_cond_signal(&cond_end);
    }
    else if (thread_id == 4)
    {
        while (!end)
        {
            pthread_cond_wait(&cond_end, &mutex);
        }
    }
    info(END, process_no, thread_no);
    if (thread_id == 2)
    {
        sem_post(sem_2);
    }
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

void *thread_function_P8(void *arg)
{

    int thread_id = *((int *)arg);
    int process_no = 8;
    int thread_no = thread_id;
    if (thread_id == 3)
    {
        sem_wait(sem_2);
    }
    info(BEGIN, process_no, thread_no);
    info(END, process_no, thread_no);
    if (thread_id == 4)
    {
        sem_post(sem_1);
    }
    pthread_exit(NULL);
}

void P(int sem_id, int sem_no)
{
    struct sembuf op = {sem_no, -1, 0};

    semop(sem_id, &op, 1);
}

void V(int sem_id, int sem_no)
{
    struct sembuf op = {sem_no, +1, 0};

    semop(sem_id, &op, 1);
}

void *thread_function_P9(void *arg)
{
    int thread_id = *((int *)arg);
    int process_no = 9;
    int thread_no = thread_id;

    P(sem_id, 0);
    info(BEGIN, process_no, thread_no);
    pthread_mutex_lock(&mutex1);
    th_no++;
    pthread_mutex_unlock(&mutex1);

    if (thread_id != 13)
    {
        while (!done)
        {
        }

        info(END, process_no, thread_no);
    }
    else
    {

        while (th_no != 6)
        {
        }
        info(END, process_no, 13);
        done = 1;
    }
    pthread_mutex_lock(&mutex1);
    th_no--;
    pthread_mutex_unlock(&mutex1);

    V(sem_id, 0);

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    init();
    pid_t pid;
    int status;
    sem_1 = sem_open(SEM_NAME_1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    sem_2 = sem_open(SEM_NAME_2, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    info(BEGIN, 1, 0);
    if ((pid = fork()) == 0)
    {
        info(BEGIN, 2, 0);
        if ((pid = fork()) == 0)
        {
            info(BEGIN, 3, 0);
            if ((pid = fork() == 0))
            {
                info(BEGIN, 4, 0);
                if ((pid = fork()) == 0)
                {
                    info(BEGIN, 5, 0);
                    if ((pid = fork()) == 0)
                    {
                        info(BEGIN, 8, 0);
                        pthread_mutex_init(&mutex, NULL);
                        pthread_cond_init(&cond_start, NULL);
                        pthread_cond_init(&cond_end, NULL);
                        pthread_t threads_P5[NUM_THREADS_P5_AND_P8];
                        pthread_t threads_P8[NUM_THREADS_P5_AND_P8];
                        for (int i = 1; i <= NUM_THREADS_P5_AND_P8; i++)
                        {
                            int *id = (int *)malloc(sizeof(i));
                            *id = i;
                            if (pthread_create(&threads_P5[i], NULL, thread_function_P5, (void *)id) != 0)
                            {
                                perror("Error creating a new thread");
                                exit(EXIT_FAILURE);
                            }
                            if (pthread_create(&threads_P8[i], NULL, thread_function_P8, (void *)id) != 0)
                            {
                                perror("Error creating a new thread");
                                exit(EXIT_FAILURE);
                            }
                        }

                        for (int i = 1; i <= NUM_THREADS_P5_AND_P8; i++)
                        {
                            pthread_join(*(threads_P5 + i), NULL);
                            pthread_join(*(threads_P8 + i), NULL);
                        }
                        info(END, 8, 0);
                        exit(EXIT_SUCCESS);
                    }
                    else
                    {
                        if (waitpid(pid, &status, 0) < 0)
                        {
                            perror("The error waitpid");
                            exit(EXIT_FAILURE);
                        }
                    }
                    pthread_mutex_destroy(&mutex);
                    pthread_cond_destroy(&cond_start);
                    pthread_cond_destroy(&cond_end);
                    info(END, 5, 0);
                    exit(EXIT_SUCCESS);
                }
                else
                {
                    if (waitpid(pid, &status, 0) < 0)
                    {
                        perror("The error waitpid");
                        exit(EXIT_FAILURE);
                    }
                }

                info(END, 4, 0);
                exit(EXIT_SUCCESS);
            }
            else
            {
                if (waitpid(pid, &status, 0) < 0)
                {
                    perror("The error waitpid");
                    exit(EXIT_FAILURE);
                }
            }
            info(END, 3, 0);
            exit(EXIT_SUCCESS);
        }
        else
        {
            if (waitpid(pid, &status, 0) < 0)
            {
                perror("The error waitpid");
                exit(EXIT_FAILURE);
            }
        }
        if ((pid = fork()) == 0)
        {
            info(BEGIN, 6, 0);
            info(END, 6, 0);
            exit(EXIT_SUCCESS);
        }
        else
        {
            if (waitpid(pid, &status, 0) < 0)
            {
                perror("The error waitpid");
                exit(EXIT_FAILURE);
            }
        }
        info(END, 2, 0);
        exit(EXIT_SUCCESS);
    }
    else
    {
        if (waitpid(pid, &status, 0) < 0)
        {
            perror("The error waitpid");
            exit(EXIT_FAILURE);
        }
    }
    if ((pid = fork()) == 0)
    {
        info(BEGIN, 7, 0);
        if ((pid = fork()) == 0)
        {
            info(BEGIN, 9, 0);
            pthread_t threads_P9[NUM_THREADS_P9];
            pthread_mutex_init(&mutex1, NULL);
            sem_id = semget(10000, 1, IPC_CREAT | 0600);
            if (sem_id < 0)
            {
                perror("Error creating the semaphore set");
                exit(EXIT_FAILURE);
            }
            semctl(sem_id, 0, SETVAL, 6);
            int i;
            int *id = (int *)malloc(sizeof(int));
            *id = 13;
            if (pthread_create(&threads_P9[13], NULL, thread_function_P9, (void *)id) != 0)
            {
                perror("Error creating a new thread");
                exit(EXIT_FAILURE);
            }
            while (th_no < 1)
            {
            }
            for (i = 1; i <= NUM_THREADS_P9; i++)
            {
                if (i != 13)
                {
                    int *id = (int *)malloc(sizeof(int));
                    *id = i;
                    if (pthread_create(&threads_P9[i], NULL, thread_function_P9, (void *)id) != 0)
                    {
                        perror("Error creating a new thread");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            for (int i = 1; i <= NUM_THREADS_P9; i++)
            {
                pthread_join(*(threads_P9 + i), NULL);
            }
            semctl(sem_id, 0, IPC_RMID);
            pthread_mutex_destroy(&mutex1);
            info(END, 9, 0);
            exit(EXIT_SUCCESS);
        }
        else
        {
            if (waitpid(pid, &status, 0) < 0)
            {
                perror("The error waitpid");
                exit(EXIT_FAILURE);
            }
        }
        info(END, 7, 0);
        exit(EXIT_SUCCESS);
    }
    else
    {
        if (waitpid(pid, &status, 0) < 0)
        {
            perror("The error waitpid");
            exit(EXIT_FAILURE);
        }
    }
    sem_close(sem_1);
    sem_close(sem_2);
    sem_unlink(SEM_NAME_1);
    sem_unlink(SEM_NAME_2);

    info(END, 1, 0);
    exit(EXIT_SUCCESS);

    return 0;
}
