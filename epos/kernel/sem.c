/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <stddef.h>
#include "kernel.h"
uint32_t flags;
struct semaphore {
    int value;               // 信号量的值
    int semid;               // 唯一标识符
    struct wait_queue *wq;   // 等待队列
};
#define MAX_SEMAPHORES 128

static struct semaphore *semaphores[MAX_SEMAPHORES];
static int next_semid = 0;

int sys_sem_create(int value) {
    struct semaphore *sem;
    int i;

    save_flags_cli(flags);  

    // 在信号量数组中找到一个空位
    for (i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i] == NULL) {
            break;
        }
    }

    if (i == MAX_SEMAPHORES) {
        restore_flags(flags); 
        return -1;        // 没有空位
    }

    // 分配信号量结构
    sem = (struct semaphore *)kmalloc(sizeof(struct semaphore));
    if (!sem) {
        restore_flags(flags);  
        return -1;        // 分配失败
    }

    // 初始化信号量
    sem->value = value;
    sem->semid = next_semid++;
    sem->wq = NULL;
    semaphores[i] = sem;

    restore_flags(flags);  
    return sem->semid;  // 返回信号量标识符
}
int sys_sem_destroy(int semid) {
    int i;
    struct semaphore *sem;

    save_flags_cli(flags);  

    // 查找信号量
    for (i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i] && semaphores[i]->semid == semid) {
            sem = semaphores[i];
            semaphores[i] = NULL;
            kfree(sem);  // 释放信号量内存
            restore_flags(flags);  
            return 0;  // 成功
        }
    }

    restore_flags(flags);  // 恢复中断
    return -1;  // 信号量不存在
}
int sys_sem_wait(int semid) {
    int i;
    struct semaphore *sem;

    save_flags_cli(flags);  

    // 查找信号量
    for (i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i] && semaphores[i]->semid == semid) {
            sem = semaphores[i];
            sem->value--;

            if (sem->value < 0) {
                sleep_on(&sem->wq);  // 当前线程睡眠
            }

            restore_flags(flags);  
            return 0;  // 成功
        }
    }

    restore_flags(flags);  // 恢复中断
    return -1;  // 信号量不存在
}

int sys_sem_signal(int semid) {
    int i;
    struct semaphore *sem;

    save_flags_cli(flags);  // 关闭中断，保证原子性

    // 查找信号量
    for (i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i] && semaphores[i]->semid == semid) {
            sem = semaphores[i];
            sem->value++;

            if (sem->value <= 0) {
                wake_up(&sem->wq,1);  // 唤醒等待线程
            }

            restore_flags(flags);  // 恢复中断
            return 0;  // 成功
        }
    }

    restore_flags(flags);  // 恢复中断
    return -1;  // 信号量不存在
}
