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

extern void *tlsf_create_with_pool(void* mem, size_t bytes);
extern void *g_heap;
struct timespec req = { .tv_sec = 0, .tv_nsec = 100000000 };

/* 排序数据结构（新增颜色字段） */
struct sort_data {
    int *array;
    int region;       /* 显示区域索引 0-3 */
    int num_elements; /* 元素数量 */
    COLORREF color;   /* 新增：排序算法专属颜色 */
};

/* 函数声明 */
void swap(int *a, int *b);
void draw_array(struct sort_data *data);
int partition(int arr[], int l, int r, struct sort_data *data);
void q_sort(int arr[], int l, int r, struct sort_data *data);
void insert_sort(int *arr, int len, struct sort_data *data);
void bubble_sort(int *arr, int len, struct sort_data *data);
void shell_sort(int *arr, int len, struct sort_data *data);

void __main()
{
    size_t heap_size = 32*1024*1024;
    void  *heap_base = mmap(NULL, heap_size, PROT_READ|PROT_WRITE, 
                          MAP_PRIVATE|MAP_ANON, -1, 0);
    g_heap = tlsf_create_with_pool(heap_base, heap_size);
}

void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

/* 修改后的绘制函数（使用line绘制柱状图） */
void draw_array(struct sort_data *data)
{
    int screen_width = g_graphic_dev.XResolution;
    int screen_height = g_graphic_dev.YResolution;
    int region_width = screen_width / 4;
    int start_x = data->region * region_width;
    int max_val = 0;
    int i, x, y;

    /* 计算最大值 */
    for (i = 0; i < data->num_elements; ++i) {
        if (data->array[i] > max_val) {
            max_val = data->array[i];
        }
    }
    if (max_val == 0) max_val = 1;

    /* 清空当前区域 */
    for (x = start_x; x < start_x + region_width; ++x) {
        for (y = 0; y < screen_height; ++y) {
            setPixel(x, y, RGB(0, 0, 0));
        }
    }

    /* 绘制柱状图（使用line和data->color） */
    {
        int bar_width = region_width / data->num_elements;
        int bar_height, x_start;
        for (i = 0; i < data->num_elements; ++i) {
            bar_height = (data->array[i] * screen_height) / max_val;
            x_start = start_x + i * bar_width;
            line(x_start, screen_height, x_start, screen_height - bar_height, data->color); // 使用专属颜色
        }
    }
}

/* 分区函数（保持不变） */
int partition(int arr[], int l, int r, struct sort_data *data)
{
    int pivot = arr[r];
    int i = l - 1;
    int j;
    
    for (j = l; j <= r - 1; ++j) {
        if (arr[j] < pivot) {
            ++i;
            swap(&arr[i], &arr[j]);
            draw_array(data);
            nanosleep(&req, NULL);
        }
    }
    swap(&arr[i + 1], &arr[r]);
    draw_array(data);
    nanosleep(&req, NULL);
    return i + 1;
}

/* 排序算法实现（保持不变） */
void q_sort(int arr[], int l, int r, struct sort_data *data)
{
    if (l < r) {
        int pi = partition(arr, l, r, data);
        q_sort(arr, l, pi - 1, data);
        q_sort(arr, pi + 1, r, data);
    }
}

void insert_sort(int *arr, int len, struct sort_data *data)
{
    int i, j, key;
    for (i = 1; i < len; ++i) {
        key = arr[i];
        j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            --j;
            draw_array(data);
            nanosleep(&req, NULL);
        }
        arr[j + 1] = key;
        draw_array(data);
        nanosleep(&req, NULL);
    }
}

void bubble_sort(int *arr, int len, struct sort_data *data)
{
    int i, j;
    for (i = 0; i < len - 1; ++i) {
        for (j = 0; j < len - i - 1; ++j) {
            if (arr[j] > arr[j + 1]) {
                swap(&arr[j], &arr[j + 1]);
                draw_array(data);
                nanosleep(&req, NULL);
            }
        }
    }
}

void shell_sort(int *arr, int len, struct sort_data *data)
{
    int gap, i, j, temp;
    for (gap = len / 2; gap > 0; gap /= 2) {
        for (i = gap; i < len; ++i) {
            temp = arr[i];
            for (j = i; j >= gap && arr[j - gap] > temp; j -= gap) {
                arr[j] = arr[j - gap];
                draw_array(data);
                nanosleep(&req, NULL);
            }
            arr[j] = temp;
            draw_array(data);
            nanosleep(&req, NULL);
        }
    }
}

/* 排序任务函数（保持不变） */
static void tsk_q(void *pv) { 
    struct sort_data *data = (struct sort_data *)pv;
    q_sort(data->array, 0, data->num_elements-1, data);
    task_exit(0); 
}

static void tsk_i(void *pv) { 
    struct sort_data *data = (struct sort_data *)pv;
    insert_sort(data->array, data->num_elements, data);
    task_exit(0); 
}

static void tsk_b(void *pv) { 
    struct sort_data *data = (struct sort_data *)pv;
    bubble_sort(data->array, data->num_elements, data);
    task_exit(0); 
}

static void tsk_s(void *pv) { 
    struct sort_data *data = (struct sort_data *)pv;
    shell_sort(data->array, data->num_elements, data);
    task_exit(0); 
}

/**
 * 主函数（修改颜色初始化）
 */
void main(void *pv)
{
    /* 初始化图形模式 */
    init_graphic(0x143);
    
    int **arrays;
    struct sort_data *data[4];
    int i, j;
    const int num_elements = 50;
    const size_t stack_size = 1024*1024;

    /* 颜色定义 */
    COLORREF colors[4] = {
        RGB(255, 0, 0),   // 快速排序-红色
        RGB(0, 255, 0),   // 插入排序-绿色
        RGB(0, 0, 255),   // 冒泡排序-蓝色
        RGB(255, 255, 0)  // 希尔排序-黄色
    };

    /* 创建测试数组 */
    arrays = (int **)malloc(4 * sizeof(int *));
    for (i = 0; i < 4; ++i) {
        arrays[i] = (int *)malloc(num_elements * sizeof(int));
        for (j = 0; j < num_elements; ++j) {
            arrays[i][j] = rand() % 1000;
        }
    }

    /* 初始化排序数据结构（添加颜色赋值） */
    for (i = 0; i < 4; ++i) {
        data[i] = (struct sort_data *)malloc(sizeof(struct sort_data));
        data[i]->array = arrays[i];
        data[i]->region = i;
        data[i]->num_elements = num_elements;
        data[i]->color = colors[i]; // 分配专属颜色
    }

    /* 创建排序任务 */
    task_create(malloc(stack_size) + stack_size, tsk_q, data[0]);
    task_create(malloc(stack_size) + stack_size, tsk_i, data[1]);
    task_create(malloc(stack_size) + stack_size, tsk_b, data[2]);
    task_create(malloc(stack_size) + stack_size, tsk_s, data[3]);

    /* 保持主线程运行 */
    while(1);

    /* 清理资源（实际不会执行到这里） */
    for (i = 0; i < 4; ++i) {
        free(arrays[i]);
        free(data[i]);
    }
    free(arrays);
    exit_graphic();
    task_exit(0);
}