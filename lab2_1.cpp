#include "../sys_pog/check.hpp"
#include <unistd.h>
#include <ctime>
#include <signal.h>
#include <stdio.h>
#include <wait.h>
#include <stack>
#include <iostream>
#include <chrono>
#include <errno.h>
#include <time.h>

volatile sig_atomic_t sig_id;
volatile sig_atomic_t sig_value;

void setting_mask() {
    sigset_t set;
    sigemptyset(&set);
    sigfillset(&set);
    check(sigprocmask(SIG_SETMASK, &set, nullptr));
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

int wait_signal(sigset_t wait_set, siginfo_t &info, pid_t opponent) {

    struct timespec timeout = {0, 1000000};
    while (true) {
        ssize_t r = check_except(sigtimedwait(&wait_set, &info, &timeout), EAGAIN);

        if (r >= 0) break;

        if (!process_exists(opponent))
            exit(EXIT_FAILURE);

    }
    return info.si_signo;
}


void left_player(int range_max, pid_t opponent) {
    int number = rand() % range_max + 1;
    std::cout << "The process id: " << getpid() << std::endl;
    std::cout << "The hidden number is " << number << std::endl;

    check(sigqueue(opponent, SIGRTMAX, sigval{ range_max }));

    bool guessed = false;
    while (!guessed) {
        siginfo_t info;
        sigset_t wait_set;
        sigemptyset(&wait_set);
        sigaddset(&wait_set, SIGRTMAX);
        int sig_number= wait_signal(wait_set,info, opponent);
        sig_value = info.si_value.sival_int;
        if (sig_value == number) {
            guessed = true;
            check(kill(opponent, SIGUSR1));
        } else {
            check(kill(opponent, SIGUSR2));
        }
    }
}

void right_player(const pid_t opponent) {
    {
        siginfo_t info;
        sigset_t wait_set;
        sigemptyset(&wait_set);
        sigaddset(&wait_set, SIGRTMAX);
        int sig_number= wait_signal(wait_set,info, opponent);
        sig_value = info.si_value.sival_int;
    }
    int max_range = sig_value;
    std::stack<int> numbers;
    for (int i = 1; i <= max_range; ++i)
        numbers.push(i);

    int count = 0;
    bool guessed = false;
    while (!numbers.empty() && !guessed) {
        int value = numbers.top();
        numbers.pop();

        check(sigqueue(opponent, SIGRTMAX, sigval{ value }));

        siginfo_t info;
        sigset_t wait_set;
        sigemptyset(&wait_set);
        sigaddset(&wait_set, SIGUSR1);
        sigaddset(&wait_set, SIGUSR2);

        sig_id= wait_signal(wait_set,info, opponent);
        sig_value = info.si_value.sival_int;

        if (sig_id == SIGUSR1) {
            guessed = true;
            printf("Process %d guessed the number: %d\n", getpid(), value);
            printf("Count: %d\n", count);
        }
        else {
            ++count;
        }
    }
}

void role_change(int game_count, pid_t pid, pid_t pid_p, int range_max) {
    auto start_time = std::chrono::high_resolution_clock::now();
    if (game_count % 2 == 0) {
        if (pid > 0) {
            left_player(range_max, pid);
        }
        else {
            right_player(pid_p);
        }
    }
    else {
        if (pid == 0) {
            left_player(range_max, pid_p);
        }
        else {
            right_player(pid);
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Time: " << duration << " ms\n";
}

void start(int rounds, int range_max) {
    no_zombie();
    setting_mask();
    pid_t pid_p = getpid();
    pid_t pid = check(fork());
    srand(pid ? 1000 : 100000 );

    for (int game_count = 1; game_count <= rounds; ++game_count) {
        printf("Game: %d\n", game_count);
        role_change(game_count, pid, pid_p, range_max);
    }
}

int main() {
    start(200, 10000);
}
