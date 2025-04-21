#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "check.hpp"

using namespace std;

vector<vector<double>> generate_matrix(size_t size) {
    vector<vector<double>> matrix(size, vector<double>(size));
    for (size_t i = 0; i < size; ++i) {
        for (size_t j = 0; j < size; ++j) {
            matrix[i][j] = rand();
        }
    }
    return matrix;
}

void write_matrix(size_t size, const char* filename) {
    int fd = check(open(filename, O_CREAT | O_WRONLY| O_TRUNC, S_IRWXU));
    vector<vector<double>> matrix=generate_matrix(size);
    for (const auto& row : matrix) {
        ssize_t bytes = check(write(fd, row.data(), row.size() * sizeof(double)));
        }
    close(fd);
}

vector<int> generate_arr(size_t size, int min, int max) {
    vector<int> array(size);
    for (size_t i = 0; i < size; ++i) {
        array[i]= rand()% (max-min+1)+min;
    }
    return array;
}

void write_array(size_t size, const char* filename, int min, int max) {
    int fd = check(open(filename, O_CREAT | O_WRONLY| O_TRUNC, S_IRWXU));
    vector<int> array=generate_arr(size, min, max);
    for (const auto& number: array) {
        ssize_t bytes = check(write(fd, &number, sizeof(int)));
    }
    close(fd);
}

int main() {
    srand(10000);
    //write_matrix(1000, "../matrix1.bin");
    //write_matrix(1000, "../matrix2.bin");
    write_array(10000000, "../array.bin", -1000, 1000);

}
