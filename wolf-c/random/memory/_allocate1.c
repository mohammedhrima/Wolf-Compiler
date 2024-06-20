#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define SIZE 100
typedef struct Memo Memo;
struct Memo
{
    uintptr_t buffer[SIZE];
    Memo *next;
};

Memo *head;
Memo *curr;
size_t pos;

void *_allocate(size_t size)
{
    if (head == NULL)
    {
        head = calloc(1, sizeof(Memo));
        curr = head;
    }
    else if (pos == SIZE)
    {
        // printf("reached end\n");
        pos = 0;
        curr->next = calloc(1, sizeof(Memo));
        curr = curr->next;
    }
    // printf("%zu ", pos);
    void *ptr = calloc(size + sizeof(size_t), sizeof(char));
    ((size_t *)ptr)[0] = size;
    curr->buffer[pos++] = (uintptr_t)ptr;
    return ptr + sizeof(size_t);
}

void _free_memory()
{
    while (head)
    {
        for (int i = 0; i < SIZE; i++)
            free((void *)head->buffer[i]);
        Memo *tmp = head;
        head = head->next;
        free(tmp);
    }
}

int main()
{
    void *ptr = _allocate(100);
    ptr = _allocate(90);
    printf("%zu\n", ((size_t *)ptr)[-1]);
    ptr = _allocate(80);
    printf("%zu\n", ((size_t *)ptr)[-1]);
    ptr = _allocate(70);
    printf("%zu\n", ((size_t *)ptr)[-1]);
    ptr = _allocate(60);
    printf("%zu\n", ((size_t *)ptr)[-1]);
    // ptr = _allocate(50);
    // printf("%zu\n", *(ptr + sizeof(size_t)));
    _free_memory();
}