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

int tid1, tid2;
int emp2 = 2;

extern void *tlsf_create_with_pool(void* mem, size_t bytes);
extern void *g_heap;

struct timespec req = { .tv_sec = 0, .tv_nsec = 100000000 };

void draw_priority_bar(int x_center, int prio, COLORREF color) {
    int i;
    for (i = 0; i <= prio; i++) {
        line(x_center - i * emp2 - 1, g_graphic_dev.YResolution * 3 / 4 - 25,
             x_center - i * emp2 - 1, g_graphic_dev.YResolution * 3 / 4 + 25, color);
    }
}

void clear_priority_bar(int x_center, int prio) {
    int i;
    for (i = 0; i <= prio; i++) {
        line(x_center - i * emp2 - 1, g_graphic_dev.YResolution * 3 / 4 - 25,
             x_center - i * emp2 - 1, g_graphic_dev.YResolution * 3 / 4 + 25, RGB(0, 0, 0));
    }
}

void draw_sort_progress(int x_pos, int index, int value, COLORREF color, int direction) {
    if (direction == 0) {
        line(x_pos, index * 6, x_pos - value, index * 6, color);
    } else {
        line(x_pos, index * 6, x_pos + value, index * 6, color);
    }
}

void b_sort(int *arr, int len, int x_pos, COLORREF color, int direction) {
    int i, j, tmp;
    for (i = 0; i < len - 1; ++i) {  
        for (j = 0; j < len - i - 1; ++j) {  
            if (arr[j] > arr[j + 1]) {            
                draw_sort_progress(x_pos, j, arr[j], RGB(0, 0, 0), direction);
                draw_sort_progress(x_pos, j + 1, arr[j + 1], RGB(0, 0, 0), direction);               
                tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
                draw_sort_progress(x_pos, j, arr[j], color, direction);
                nanosleep(&req, NULL);
                draw_sort_progress(x_pos, j + 1, arr[j + 1], color, direction);
                nanosleep(&req, NULL);
            }
        }
    }
}

void tsk_sort(void *pv) {
    int id = task_getid();
    printf("This is Bubble Sort with tid=%d\r\n", id);
    int i;
    int arr[100];
    srand(time(NULL));
    for (i = 0; i < 100; i++) {
        arr[i] = rand() % 200;
        draw_sort_progress((id == tid1) ? 350 : 450, i, arr[i], (id == tid1) ? RGB(0, 255, 0) : RGB(0, 0, 255), (id == tid1) ? 0 : 1);
    }
    b_sort(arr, 100, (id == tid1) ? 350 : 450, (id == tid1) ? RGB(0, 255, 0) : RGB(0, 0, 255), (id == tid1) ? 0 : 1);
    task_exit(0);
}

void tsk_control(void *pv) {
    int prio_l = getpriority(tid1);
    int prio_r = getpriority(tid2);

    draw_priority_bar(g_graphic_dev.XResolution / 4, prio_l, RGB(0, 255, 255));
    draw_priority_bar(g_graphic_dev.XResolution * 3 / 4, prio_r, RGB(255, 0, 255));

    int mykeypress;
    while (1) {
        mykeypress = getchar();  
        switch (mykeypress) {
            case 0x4800:
                clear_priority_bar(g_graphic_dev.XResolution / 4, prio_l);
                prio_l = (prio_l < PRI_USER_MAX) ? prio_l + 1 : PRI_USER_MAX;
                setpriority(tid1, prio_l);
                break;
            case 0x5000:
                clear_priority_bar(g_graphic_dev.XResolution / 4, prio_l);
                prio_l = (prio_l > PRI_USER_MIN) ? prio_l - 1 : PRI_USER_MIN;
                setpriority(tid1, prio_l);
                break;
            case 0x4d00:
                clear_priority_bar(g_graphic_dev.XResolution * 3 / 4, prio_r);
                prio_r = (prio_r < PRI_USER_MAX) ? prio_r + 1 : PRI_USER_MAX;
                setpriority(tid2, prio_r);
                break;
            case 0x4b00:
                clear_priority_bar(g_graphic_dev.XResolution * 3 / 4, prio_r);
                prio_r = (prio_r > PRI_USER_MIN) ? prio_r - 1 : PRI_USER_MIN;
                setpriority(tid2, prio_r);
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

    unsigned int stack_size = 1024 * 1024;
    unsigned char *stack_foo1 = malloc(stack_size);
    unsigned char *stack_foo2 = malloc(stack_size);
    unsigned char *stack_foo3 = malloc(stack_size);

    tid1 = task_create(stack_foo1 + stack_size, &tsk_sort, NULL);
    setpriority(tid1, 5);
    tid2 = task_create(stack_foo2 + stack_size, &tsk_sort, NULL);
    setpriority(tid2, 8);

    int tid_control = task_create(stack_foo3 + stack_size, &tsk_control, NULL);
    setpriority(tid_control, 2);

    task_wait(tid1, NULL);
    task_wait(tid2, NULL);
    task_wait(tid_control, NULL);

    free(stack_foo1);
    free(stack_foo2);
    free(stack_foo3);

    while (1);

    task_exit(0);
}