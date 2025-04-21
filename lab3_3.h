#ifndef LAB_3_3_H
#define LAB_3_3_H

#include <queue>
#include <fcntl.h>
#include <unistd.h>
#include "check.hpp"
#include <pthread.h>

using namespace std;

template <typename T>
class mt_queue{
    std::queue<T> msg_queue;
    const size_t max_size;
    mutable pthread_mutex_t mutex;
    pthread_cond_t can_write;
    pthread_cond_t can_read;
    bool exists=true;

public:
    mt_queue(size_t max_size): max_size(max_size) {
        mutex=PTHREAD_MUTEX_INITIALIZER;
        can_write=PTHREAD_COND_INITIALIZER;
        can_read=PTHREAD_COND_INITIALIZER;
        exists=true;
    };

    mt_queue(const mt_queue&) = delete;//no queue copy
    mt_queue(mt_queue&&) = delete; //no queue move

    ~mt_queue() {
        my_exit();
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&can_read);
        pthread_cond_destroy(&can_write);
    };

    void enqueue(const T& v) {
        check_result(pthread_mutex_lock(&mutex));
        while(msg_queue.size() >= max_size && exists) {
            check_result(pthread_cond_wait(&can_write, &mutex));
        }
        if (!exists) {
            check_result(pthread_mutex_unlock(&mutex));
            printf("Queue doesn't exist\n");
        }
        else {
            msg_queue.push(v);
        }
        check_result(pthread_cond_signal(&can_read));
        pthread_mutex_unlock(&mutex);
    }; //shall block if full

    T dequeue() {
        T value;
        check_result(pthread_mutex_lock(&mutex));
        while(msg_queue.empty() && exists) {
            check_result(pthread_cond_wait(&can_read, &mutex));
        }
        if (!exists) {
           pthread_mutex_unlock(&mutex);
            printf("Queue doesn't exist\n");
        }
        else {
        value= msg_queue.front();
        msg_queue.pop();}

        check_result(pthread_cond_signal(&can_write));
        pthread_mutex_unlock(&mutex);
        return value;
    }; //shall block if empty


    bool full() const {

        if (msg_queue.size() < max_size)
            return false;

        bool result = false;
        check_result(pthread_mutex_lock(&mutex));
        if (msg_queue.size() >= max_size)
            result=true;
        pthread_mutex_unlock(&mutex);
        return result;
    }

    bool empty() const {
        if (!msg_queue.empty())
            return false;

        check_result(pthread_mutex_lock(&mutex));
        bool result = msg_queue.empty();
        pthread_mutex_unlock(&mutex);

        return result;
    }

    std::optional<T> try_dequeue() {
        check_result(pthread_mutex_lock(&mutex));

        if (msg_queue.empty()) {
            pthread_mutex_unlock(&mutex);
            return std::nullopt;
        }

        T value = msg_queue.front();
        msg_queue.pop();

        check_result(pthread_cond_signal(&can_write));
        pthread_mutex_unlock(&mutex);
        return value;
    }

    bool try_enqueue(const T& v) {
        check_result(pthread_mutex_lock(&mutex));
        if (msg_queue.size() >= max_size) {
            pthread_mutex_unlock(&mutex);
            return false;
        }
        msg_queue.push(v);
        check_result(pthread_cond_signal(&can_read));
        pthread_mutex_unlock(&mutex);
        return true;
    };

    void my_exit() {
        check_result(pthread_mutex_lock(&mutex));
        exists = false;
        pthread_cond_broadcast(&can_read);
        pthread_cond_broadcast(&can_write);
        pthread_mutex_unlock(&mutex);
    }

    bool get_exists() {
        //check_result(pthread_mutex_lock(&mutex));
        //bool result = exists;
       // pthread_mutex_unlock(&mutex);
        return exists;
    }
};

#endif