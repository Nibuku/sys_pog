#include "lab3_3.h"
#include "check.hpp"
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

constexpr int NUM_WRITERS = 2;
constexpr int NUM_READERS = 5;

struct ThreadArg {
    int32_t id;
    mt_queue<int>* msg_queue;
};

void* writer(void* arg) {
    ThreadArg* args = (ThreadArg*)(arg);
    int32_t id = args->id;
    auto* q = args->msg_queue;

    while (q->get_exists()) {
        int value = rand() % 100;
        printf("Writer %d:  %d\n", id, value);
        q->enqueue(value);

        //sleep(2);
    }
    printf("Writer %d exiting.\n", id);
    return nullptr;
}


void* reader(void* arg) {
    ThreadArg* args = (ThreadArg*)(arg);
    int32_t id = args->id;
    auto* q = args->msg_queue;

    while (q->get_exists()) {
        if (q->empty()) {
            cout<<"Queue is empty"<<endl;
        }
        std::optional<int> value = q->dequeue();
        if (value.has_value()) {
            printf("Reader %d received: %d\n", id, *value);
        }
        //sleep(1);
    }
    if (!q->get_exists())
        printf("Reader %d exiting (queue doesn't exist).\n", id);

    return nullptr;
}

int main() {

    srand(getpid() ? 1000 : 100000 );

    mt_queue<int> msg_queue(10);

    pthread_t writers[NUM_WRITERS];
    pthread_t readers[NUM_READERS];

    ThreadArg writer_args[NUM_WRITERS];
    ThreadArg reader_args[NUM_READERS];

    for (int32_t i = 0; i < NUM_WRITERS; ++i) {
        writer_args[i] = {i, &msg_queue};
        check_result(pthread_create(&writers[i], nullptr, writer, &writer_args[i]));
        sleep(1);
    }

    for (int32_t i = 0; i < NUM_READERS; ++i) {
        reader_args[i] = {i, &msg_queue};
        check_result(pthread_create(&readers[i], nullptr, reader, &reader_args[i]));

    }

    sleep(20);

    msg_queue.my_exit();

    for (int i = 0; i < NUM_WRITERS; ++i) {
        check_result(pthread_join(writers[i], nullptr));

    }
    for (int i = 0; i < NUM_READERS; ++i) {
        check_result(pthread_join(readers[i], nullptr));
    }
    return 0;
}
