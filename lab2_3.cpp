#include "check.hpp"
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <iostream>
#include <stack>

timespec next_timeout(timespec wait_time) {
    timespec result{};
    check(clock_gettime(CLOCK_REALTIME, &result));
    result.tv_sec += wait_time.tv_sec;
    result.tv_nsec += wait_time.tv_nsec;

    if (result.tv_nsec > 1'000'000'000) {
        result.tv_sec += result.tv_nsec / 1'000'000'000;
        result.tv_nsec %= 1'000'000'000;
    }
    return result;
}

void left_player(mqd_t queue_out, mqd_t queue_in, int max_range) {
    srand(time(0) ^ getpid());
    int number = rand() % max_range + 1;
    printf("Process %d hidden the number: %d\n", getpid(), number);
    timespec timeout = next_timeout({1, 0});

    check(mq_timedsend(queue_out, (char*)&max_range, sizeof(int), 0, &timeout));

    bool guessed = false;
    int attempt_buffer;
    int flag;

    while (!guessed) {
        timespec timeout = next_timeout({1, 0});
        ssize_t r;

        while ((r = mq_timedreceive(queue_in, (char*)&attempt_buffer, sizeof(int), nullptr, &timeout)) < 0) {
            check_except(r, ETIMEDOUT);
            if (waitpid(0, nullptr, WNOHANG) != 0)
                exit(0);
            timeout = next_timeout({1, 0});
        }

        if (attempt_buffer == number) {
            flag = 1;
            check(mq_send(queue_out, (char*)&flag, sizeof(int), 0));
            guessed = true;
        } else {
            flag = 2;
            check(mq_send(queue_out, (char*)&flag, sizeof(int), 0));
        }
    }
}


void right_player(mqd_t queue_out, mqd_t queue_in) {
    int max_range;
    timespec timeout = next_timeout({1, 0});
    check(mq_timedreceive(queue_in, (char*)&max_range, sizeof(max_range), nullptr, &timeout));

    int flag;
    bool guessed = false;
    int value;
    int count = 0;
    std::stack<int> numbers;

    for (int i = 1; i <= max_range; ++i)
        numbers.push(i);

    while (!guessed && !numbers.empty()) {  // Добавлена проверка
        value = numbers.top();
        numbers.pop();

        check(mq_timedsend(queue_out, (char*)&value, sizeof(value), 0, &timeout));
        check(mq_timedreceive(queue_in, (char*)&flag, sizeof(int), nullptr, &timeout));

        if (flag == 1) {
            guessed = true;
            printf("Count: %d\n", count);
            printf("Process %d guessed the number: %d\n", getpid(), value);
        } else {
            ++count;
        }
    }
}


void role_change(int game_count, mqd_t queue_write, mqd_t queue_read, int pid, int range_max) {
    int count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    if (game_count % 2 != 0) {
        if (pid == 0) {
            left_player(queue_write, queue_read, range_max);
        }
        else {
            right_player(queue_write, queue_read);
        }
    }
    else {
        if (pid > 0) {
            left_player(queue_write, queue_read, range_max);
        }
        else {
            right_player(queue_write, queue_read);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Time: " << duration << " ms\n";

}

void start(int rounds, int range_max) {
    mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(int);

    mq_unlink("/parent_to_child");
    mq_unlink("/child_to_parent");

    // Создание очередей
    mqd_t parent = check(mq_open("/parent_to_child", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr));
    mqd_t child = check(mq_open("/child_to_parent", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr));

    int pid = check(fork());

    mqd_t queue_write, queue_read;
    if (pid == 0) {
        queue_write = child;
        queue_read = parent;
    }
    else {
        queue_write = parent;
        queue_read = child;
    }
    for (int game_count = 1; game_count <= rounds; ++game_count) {
        printf("Game %d\n", game_count);
        role_change(game_count, queue_write, queue_read, pid, range_max);
    }

    check(mq_close(parent));
    check(mq_close(child));
    mq_unlink("/parent_to_child");
    mq_unlink("/child_to_parent");

}

int main() {

    start(200, 10000);

}
