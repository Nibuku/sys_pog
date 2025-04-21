#include <iostream>
#include <vector>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include "check.hpp"
#include <chrono>
#include <pthread.h>

using namespace std;

struct ThreadArg{
    const vector<double> &matrix1;
    const vector<double> &matrix2;
    vector<double> &result;

    int start_row;
    int end_row;

};

vector<double> read_matrix(const char* filename) {
    int fd = check(open(filename, O_RDONLY));

    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    size_t N = (size_t)file_size / sizeof(double); //квадрат от реального размера

    vector<double> matrix(N);
    check((read(fd, matrix.data(), file_size)));
    close(fd);
    return matrix;
}

vector<double> multiply_matrices(const vector<double>& matrix1, const vector<double>& matrix2) {
    size_t N=(size_t)sqrt(matrix1.size());
    vector<double> result(matrix1.size(), 0.0);
    auto start_time = chrono::steady_clock::now();
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            for (size_t k = 0; k < N; ++k) {
                result[i * N + j] += matrix1[i * N + k] * matrix2[k * N + j];
            }
        }
    }
    auto end_time = chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Time without pthread: " << duration << " ms\n";
    return result;
}

void* multiply_rows(void* arg_) {
    ThreadArg* arg = (ThreadArg*)arg_;
    size_t N = (size_t)sqrt(arg->matrix1.size());

    for (size_t i = arg->start_row; i < arg->end_row; ++i) {
        for (size_t j = 0; j < N; ++j) {
            double sum = 0;
            for (size_t k = 0; k < N; ++k) {
                sum += arg->matrix1[i * N + k] * arg->matrix2[k * N + j];
            }
            arg->result[i * N + j] = sum;
        }
    }
    delete arg;
    return nullptr;
}
vector<double> pthread_multiply(vector<double>& matrix1, vector<double>& matrix2, int thread_count) {
    size_t N = sqrt(matrix1.size());
    vector<double> result(N * N, 0.0);
    vector<pthread_t> threads(thread_count);

    int rows_per_thread = N / thread_count;
    int remaining_rows = N % thread_count;

    auto start = chrono::steady_clock::now();

    int start_row = 0;
    for (int i = 0; i < thread_count; ++i) {
        int end_row;
        if (i < remaining_rows) {
            end_row = start_row + rows_per_thread + 1;
        }
        else {
            end_row = start_row + rows_per_thread;
        }

        if (end_row > N)
            end_row = N;

        auto arg = new ThreadArg{matrix1, matrix2, result, start_row, end_row};
        check_result(pthread_create(&threads[i],  nullptr, multiply_rows, arg));

        start_row = end_row;
    }

    for (int i = 0; i < thread_count; ++i) {
        check_result(pthread_join(threads[i],  nullptr));
    }

    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "Time with pthread (" << thread_count << " threads): " << duration << " ms\n";

    return result;
}

bool check_multiply(const vector<double>& matrix1, const vector<double>& matrix2) {
    size_t size=matrix1.size();
    for (size_t i = 0; i < size; ++i) {
            if (matrix1[ size] != matrix2[size]) {
                return false;
            }
    }
    return true;
}

int main() {
    const char* file1 = "../matrix1.bin";
    const char* file2 = "../matrix2.bin";

    vector<double> matrix1 = read_matrix(file1);
    vector<double> matrix2 = read_matrix(file2);

    vector<double> result_seq = multiply_matrices(matrix1, matrix2);
    int matrix_size=sqrt(matrix1.size());
    /*for(int i = 0; i < matrix_size; ++i) {
        for (int j = 0; j < matrix_size; j++) {
            cout << result_seq[i * matrix_size + j] << " ";
        }
        cout << endl;
    } cout<<endl;*/

    vector<double> result_pthread = pthread_multiply(matrix1, matrix2,5);
    /*for(int i = 0; i < matrix_size; ++i) {
        for (int j = 0; j < matrix_size; j++) {
            cout << result_pthread[i * matrix_size + j] << " ";
        }
        cout << endl;
    } cout<<endl;*/

    cout<<check_multiply(result_seq, result_pthread);
    return 0;
}

