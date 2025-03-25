#include "../sys_pog/check.hpp"
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

bool process_exists(pid_t p) {
    if (kill(p, 0) == -1 && errno == ESRCH)
        return false;
    return true;
}

void no_zombie() {
    struct sigaction s {};
    s.sa_handler = SIG_IGN;
    check(sigaction(SIGCHLD, &s, NULL));
}

void wait_m( mqd_t queue, int attempt_buffer, pid_t opponent, bool direction) {
    timespec timeout = next_timeout({1, 0});
    ssize_t r;
    while (true) {
        if (direction==0)
            r = check_except(mq_timedreceive(queue, (char*)&attempt_buffer, sizeof(int), nullptr, &timeout), ETIMEDOUT);
        else {
            r = check_except(mq_timedsend(queue, (char*)&attempt_buffer, sizeof(int), 0, &timeout), ETIMEDOUT);}

        if (r >= 0)
            break;

        if (!process_exists(opponent))
            exit(EXIT_FAILURE);
    }
        timeout = next_timeout({1, 0});
    }

void left_player(mqd_t queue_out, mqd_t queue_in, int max_range, pid_t opponent) {

    int number = rand() % max_range + 1;
    printf("Process %d hidden the number: %d\n", getpid(), number);
    wait_m( queue_out, max_range, opponent, 1);

    int guessed = 0;
    int attempt_buffer=0;
    int flag;

    while (!guessed) {

        wait_m(queue_in, attempt_buffer, opponent, 0);
        if (attempt_buffer == number) {
            flag = 1;
            check(mq_send(queue_out, (char*)&flag, sizeof(int), 0));
            guessed = 1;
        } else {
            flag = 2;
            check(mq_send(queue_out, (char*)&flag, sizeof(int), 0));
        }
    }
}


void right_player(mqd_t queue_out, mqd_t queue_in, pid_t opponent) {
    int max_range;
    wait_m( queue_in, max_range, opponent, 0);

    int flag;
    int guessed = 0;
    int value;
    int count = 0;
    std::stack<int> numbers;

    for (int i = 1; i <= max_range; ++i)
        numbers.push(i);

    while (!guessed && !numbers.empty()) {
        value = numbers.top();
        numbers.pop();

        wait_m( queue_out, value, opponent, 1);
        wait_m( queue_in, flag, opponent, 0);

        if (flag == 1) {
            guessed = 1;
            printf("Count: %d\n", count);
            printf("Process %d guessed the number: %d\n", getpid(), value);
        } else {
            ++count;
        }
    }
}

void role_change(int game_count, mqd_t queue_write, mqd_t queue_read, pid_t pid, pid_t pid_p, int range_max) {
    int count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    if (game_count % 2 != 0) {
        if (pid == 0) {
            left_player(queue_write, queue_read, range_max, pid_p);
        }
        else {
            right_player(queue_write, queue_read, pid);
        }
    }
    else {
        if (pid > 0) {
            left_player(queue_write, queue_read, range_max, pid);
        }
        else {
            right_player(queue_write, queue_read, pid_p);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Time: " << duration << " ms\n";
}

void start(int rounds, int range_max) {
    no_zombie();
    mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(int);

    mq_unlink("/parent_to_child");
    mq_unlink("/child_to_parent");

    // Создание очередей
    mqd_t parent = check(mq_open("/parent_to_child", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr));
    mqd_t child = check(mq_open("/child_to_parent", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr));

    pid_t pid_p = getpid();
    pid_t pid = check(fork());

    srand(pid ? 1000 : 100000 );
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
        role_change(game_count, queue_write, queue_read, pid, pid_p, range_max);
    }

    check(mq_close(parent));
    check(mq_close(child));
    mq_unlink("/parent_to_child");
    mq_unlink("/child_to_parent");

}

int main() {
    start(200, 10000);
}
