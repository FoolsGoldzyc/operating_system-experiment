
/*
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <inttypes.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <syscall.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "graphics.h"
#include <time.h>

#define PRI_USER_MIN 0
#define PRI_USER_MAX 127
#define SCREEN_DIVISION 4 // 屏幕分成4份
#define BUFFER_SIZE 100   // 每个缓冲区的大小

int tid1, tid2, tid_control;
int emp2 = 2;

// 缓冲区和信号量
int buffer[SCREEN_DIVISION][BUFFER_SIZE];  // 多个缓冲区
int in = 0, out = 0;                      // 生产者、消费者指针
int count = 0;                            // 当前缓冲区计数

int sem_full;  // 表示缓冲区满的信号量
int sem_empty; // 表示缓冲区空的信号量
int sem_mutex; // 用于保护缓冲区的互斥信号量

extern void *tlsf_create_with_pool(void* mem, size_t bytes);
extern void *g_heap;

// 绘制缓冲区内容
void draw_buffer(int buffer_id, int index, int value, COLORREF color) {
    int x_start = buffer_id * (g_graphic_dev.XResolution / SCREEN_DIVISION);
    int y_start = index * 6;
    line(x_start, y_start, x_start + value, y_start, color);
}

// 清除缓冲区内容
void clear_buffer(int buffer_id, int index, COLORREF color) {
    int x_start = buffer_id * (g_graphic_dev.XResolution / SCREEN_DIVISION);
    int y_start = index * 6;
    line(x_start, y_start, x_start + g_graphic_dev.XResolution / SCREEN_DIVISION, y_start, color); // 用黑色覆盖整个分区
}

// 绘制优先级进度条
void draw_priority_bar(int x_center, int prio, COLORREF color) {
    int i;
    for (i = 0; i <= prio; i++) {
        line(x_center - i * emp2 - 1, g_graphic_dev.YResolution * 3 / 4 - 25,
             x_center - i * emp2 - 1, g_graphic_dev.YResolution * 3 / 4 + 25, color);
    }
}

// 清除优先级进度条
void clear_priority_bar(int x_center, int prio) {
    int i;
    for (i = 0; i <= prio; i++) {
        line(x_center - i * emp2 - 1, g_graphic_dev.YResolution * 3 / 4 - 25,
             x_center - i * emp2 - 1, g_graphic_dev.YResolution * 3 / 4 + 25, RGB(0, 0, 0));
    }
}

// 快速排序（动态显示）
void quick_sort_dynamic(int buffer_id, int *arr, int low, int high, COLORREF color) {
    if (low >= high) {
        return;
    }

    int pivot = arr[low]; // 选择基准值
    int left = low + 1;
    int right = high;

    struct timespec req = { .tv_sec = 0, .tv_nsec = 10000000 };  // 延迟

    while (left <= right) {
        while (left <= right && arr[left] <= pivot) {
            left++;
        }
        while (left <= right && arr[right] >= pivot) {
            right--;
        }
        if (left < right) {
            // 交换元素，动态显示交换过程
            clear_buffer(buffer_id, left, RGB(0, 0, 0));
            clear_buffer(buffer_id, right, RGB(0, 0, 0));
            nanosleep(&req, NULL);

            int tmp = arr[left];
            arr[left] = arr[right];
            arr[right] = tmp;

            draw_buffer(buffer_id, left, arr[left], color);
            draw_buffer(buffer_id, right, arr[right], color);
            nanosleep(&req, NULL);
        }
    }

    // 交换基准值到正确位置
    clear_buffer(buffer_id, low, RGB(0, 0, 0));
    clear_buffer(buffer_id, right, RGB(0, 0, 0));
    nanosleep(&req, NULL);

    arr[low] = arr[right];
    arr[right] = pivot;

    draw_buffer(buffer_id, low, arr[low], color);
    draw_buffer(buffer_id, right, arr[right], color);
    nanosleep(&req, NULL);

    // 递归排序左右两部分
    quick_sort_dynamic(buffer_id, arr, low, right - 1, color);
    quick_sort_dynamic(buffer_id, arr, right + 1, high, color);
}

// 生产者线程（动态绘制）
void tsk_producer(void *pv) {
    int id = task_getid();
    printf("Producer thread started with tid=%d\r\n", id);

    srand(time(NULL));
    while (1) {
        sem_wait(sem_empty); // 等待缓冲区有空位
        sem_wait(sem_mutex); // 进入临界区

        int i;
        for (i = 0; i < BUFFER_SIZE; ++i) {
            buffer[in][i] = rand() % 200;

            // 动态绘制生产者生成数据
            draw_buffer(in, i, buffer[in][i], RGB(255, 0, 0)); // 红色条表示生产者数据
            struct timespec req = { .tv_sec = 0, .tv_nsec = 10000000 };
            nanosleep(&req, NULL);
        }
        in = (in + 1) % SCREEN_DIVISION;

        sem_signal(sem_mutex); // 退出临界区
        sem_signal(sem_full);  // 通知消费者缓冲区有数据
    }
    task_exit(0);
}

// 消费者线程（动态排序）
void tsk_consumer(void *pv) {
    int id = task_getid();
    printf("Consumer thread started with tid=%d\r\n", id);

    while (1) {
        sem_wait(sem_full); // 等待缓冲区有数据
        sem_wait(sem_mutex); // 进入临界区

        int i;
        int local_buffer[BUFFER_SIZE];
        for (i = 0; i < BUFFER_SIZE; ++i) {
            local_buffer[i] = buffer[out][i];
        }
        out = (out + 1) % SCREEN_DIVISION;

        sem_signal(sem_mutex); // 退出临界区
        sem_signal(sem_empty); // 通知生产者缓冲区空位增加

        // 对缓冲区数据进行动态快速排序
        quick_sort_dynamic(out, local_buffer, 0, BUFFER_SIZE - 1, RGB(0, 0, 255)); // 蓝色表示消费者排序数据

        // 清空缓冲区显示
        for (i = 0; i < BUFFER_SIZE; ++i) {
            clear_buffer(out, i, RGB(0, 0, 0));
        }
    }
    task_exit(0);
}

// 控制线程
void tsk_control(void *pv) {
    int prio_producer = getpriority(tid1);
    int prio_consumer = getpriority(tid2);

    draw_priority_bar(g_graphic_dev.XResolution / 4, prio_producer, RGB(255, 0, 255));
    draw_priority_bar(g_graphic_dev.XResolution * 3 / 4, prio_consumer, RGB(128, 0, 128));

    int keypress;
    while (1) {
        keypress = getchar();
        switch (keypress) {
            case 0x4800: // UP 键，增加生产者优先级
                clear_priority_bar(g_graphic_dev.XResolution / 4, prio_producer);
                prio_producer = (prio_producer < PRI_USER_MAX) ? prio_producer + 1 : PRI_USER_MAX;
                setpriority(tid1, prio_producer);
                draw_priority_bar(g_graphic_dev.XResolution / 4, prio_producer, RGB(255, 0, 255));
                break;
            case 0x5000: // DOWN 键，降低生产者优先级
                clear_priority_bar(g_graphic_dev.XResolution / 4, prio_producer);
                prio_producer = (prio_producer > PRI_USER_MIN) ? prio_producer - 1 : PRI_USER_MIN;
                setpriority(tid1, prio_producer);
                draw_priority_bar(g_graphic_dev.XResolution / 4, prio_producer, RGB(255, 0, 255));
                break;
            case 0x4d00: // RIGHT 键，增加消费者优先级
                clear_priority_bar(g_graphic_dev.XResolution * 3 / 4, prio_consumer);
                prio_consumer = (prio_consumer < PRI_USER_MAX) ? prio_consumer + 1 : PRI_USER_MAX;
                setpriority(tid2, prio_consumer);
                draw_priority_bar(g_graphic_dev.XResolution * 3 / 4, prio_consumer, RGB(128, 0, 128));
                break;
            case 0x4b00: // LEFT 键，降低消费者优先级
                clear_priority_bar(g_graphic_dev.XResolution * 3 / 4, prio_consumer);
                prio_consumer = (prio_consumer > PRI_USER_MIN) ? prio_consumer - 1 : PRI_USER_MIN;
                setpriority(tid2, prio_consumer);
                draw_priority_bar(g_graphic_dev.XResolution * 3 / 4, prio_consumer, RGB(128, 0, 128));
                break;
            default:
                break;
        }
    }
    task_exit(0);
}

void __main() {
    size_t heap_size = 32 * 1024 * 1024;
    void *heap_base = (void *)mmap(NULL, heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    g_heap = tlsf_create_with_pool(heap_base, heap_size);
}

void main(void *pv) {
    int mode = 0x143;
    init_graphic(mode);

    // 初始化信号量
    sem_full = sem_create(0);    // 缓冲区满信号量
    sem_empty = sem_create(SCREEN_DIVISION);  // 缓冲区空信号量
    sem_mutex = sem_create(1);  // 互斥信号量

    unsigned int stack_size = 1024 * 1024;
    unsigned char *stack_producer = malloc(stack_size);
    unsigned char *stack_consumer = malloc(stack_size);
    unsigned char *stack_control = malloc(stack_size);

    tid1 = task_create(stack_producer + stack_size, &tsk_producer, NULL);
    setpriority(tid1, 5);
    tid2 = task_create(stack_consumer + stack_size, &tsk_consumer, NULL);
    setpriority(tid2, 8);

    tid_control = task_create(stack_control + stack_size, &tsk_control, NULL);
    setpriority(tid_control, 2);

    task_wait(tid1, NULL);
    task_wait(tid2, NULL);
    task_wait(tid_control, NULL);

    free(stack_producer);
    free(stack_consumer);
    free(stack_control);

    while (1);

    task_exit(0);
}