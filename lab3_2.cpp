#include <iostream>
#include <vector>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include "check.hpp"
#include <chrono>
#include <pthread.h>

using namespace std;

pthread_spinlock_t spin;

struct ThreadArg{
    const vector<int> &array;
    vector<int>& result;
    int search_number;
    int start_idx;
    int end_idx;
};

vector<int> read_array(const char* filename) {
    int fd = check(open(filename, O_RDONLY));

    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    size_t N = (size_t)file_size / sizeof(int); 

    vector<int> array(N);
    check((read(fd, array.data(), file_size)));
    close(fd);
    return array;
}

vector<int> search(vector<int> &array, int number) {
    vector<int> idx;
    auto start = chrono::steady_clock::now();
    for (int i=array.size()-1; i>=0; --i) {
        if (array[i]==number)
            idx.push_back(i);
    }
    auto end = chrono::steady_clock::now();

    using duration_t = std::chrono::duration<double, std::milli>;

    auto duration = chrono::duration_cast<duration_t>(end - start).count();
    cout << "Time without pthread: " << duration << " ms\n";
    if (idx.empty()) {
        printf( "The number not found\n");
    }
    return idx;
}

void* search_(void* arg_) {
    ThreadArg* arg = (ThreadArg*)arg_;
    vector<int> local_idx;
    size_t insert_pos = 0;
    for (int i=arg->end_idx-1; i>=arg->start_idx; --i) {
        if (arg->array[i]==arg->search_number)
            local_idx.push_back(i);
    }
    check_result(pthread_spin_lock(&spin));
    while (insert_pos < arg->result.size() &&
           arg->result[insert_pos] > local_idx[0]) {
        insert_pos++;
    }
    arg->result.insert(arg->result.begin() + insert_pos, local_idx.begin(), local_idx.end());
    pthread_spin_unlock(&spin);
    delete arg;
    return nullptr;
}

vector<int> pthread_search(vector<int>& array, int number, size_t thread_count) {
    size_t step= array.size()/thread_count;
    size_t remaining =array.size() % thread_count;
    vector<int> idx;
    vector<pthread_t> threads(thread_count);
    check_result(pthread_spin_init(&spin, 0));
    auto start = chrono::steady_clock::now();
    int end_index = array.size();
    for (int i = 0; i < thread_count; ++i) {
        int start_index = end_index - step;
        if (i < remaining) {
            --start_index;
        }

        if (start_index < 0)
            start_index = 0;

        auto arg = new ThreadArg{array, idx, number, start_index, end_index};
        check_result(pthread_create(&threads[i], nullptr, search_, arg));

        end_index = start_index;
    }

    for (int i = 0; i < thread_count; ++i) {
        check_result(pthread_join(threads[i],  nullptr));
    }
        pthread_spin_destroy(&spin);
        auto end = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        cout << "Time with pthread (" << thread_count << " threads): " << duration << " ms\n";
        if (idx.empty()) {
            printf( "The number not found\n");
        }
        return idx;
}

bool check_order(const vector<int>& array_1, const vector<int>& array_2) {
    size_t size=array_1.size();
    for (size_t i = 0; i < size; ++i) {
        if (array_1[ size] != array_2[size]) {
            return false;
        }
    }
    return true;
}

int main() {
    vector<int> array= read_array("../array.bin");
    vector<int> idx= search(array, 20);
    for (int i=0; i<idx.size(); ++i)
        cout<< idx[i]<<" ";
    cout<<endl;
    vector<int> idx_p= pthread_search(array, 20, 5);
    for (int i=0; i<idx_p.size(); ++i)
        cout<< idx_p[i]<<" ";
    cout<<endl;
    cout<<check_order(idx, idx_p);
}
