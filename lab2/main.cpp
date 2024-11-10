#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <math.h>
#include <vector>
#include <limits>

void cmpAndSwap (std::vector<int> &vec, size_t i, size_t j, bool dir) {
    if (dir == (vec[i] > vec[j])) std::swap(vec[i], vec[j]);
}

void bitonicMerge (std::vector<int> &vec, size_t start, size_t size, bool dir) {
    if (size <= 1) return;

    int halfSize = size / 2;

    for (int i = start; i < start + halfSize; i++) {
        cmpAndSwap(vec, i, halfSize + i, dir);
    }

    bitonicMerge(vec, start, halfSize, dir);
    bitonicMerge(vec, start + halfSize, halfSize, dir);
}

void bitonicSort (std::vector<int> &vec, size_t start, size_t size, std::mutex &mtx, int &threadsCount, int maxThreads, bool dir) {
    if (size <= 1) return;

    int halfSize = size / 2;

    if (threadsCount < maxThreads) {
        std::unique_lock uniqueLock(mtx);
        threadsCount += 1;
        uniqueLock.unlock();

        std::thread th1(bitonicSort, std::ref(vec), start, halfSize, std::ref(mtx), std::ref(threadsCount), maxThreads, dir);
        //std::thread th2(bitonicSort, std::ref(vec), start + halfSize, halfSize, std::ref(mtx), std::ref(threadsCount), maxThreads, !dir);
        bitonicSort(vec, start + halfSize, halfSize, mtx, threadsCount, maxThreads, !dir);

        th1.join();
        //th2.join();

        // 16825005
        // 22844319

        uniqueLock.lock();
        threadsCount -= 1;
        uniqueLock.unlock();
    } else {
        bitonicSort(vec, start, halfSize, mtx, threadsCount, maxThreads, dir);
        bitonicSort(vec, start + halfSize, halfSize, mtx, threadsCount, maxThreads, !dir);
    }

    bitonicMerge(vec, start, size, dir);
}

void sortVector(std::vector<int> &vec, int maxThreads) {
    std::mutex mtx;
    int threadsCount = 1;

    int originSize = vec.size();
    int nearest2Power = 1;
    while (nearest2Power < vec.size()) nearest2Power *= 2;

    vec.resize(nearest2Power, std::numeric_limits<int>::max());
    bitonicSort(vec, 0, vec.size(), mtx, threadsCount, maxThreads, true);
    vec.resize(originSize);
}

int main (int argc, char const *argv[]) {
    if (argc < 2) return 1;

    int hardwareThreads = std::thread::hardware_concurrency();
    int maxThreads = std::min(atoi(argv[1]), hardwareThreads);

    std::cout << "Input array size: ";
    int vecSize;
    std::cin >> vecSize;

    std::vector<int> vec(vecSize);

    for (int i = 0; i < vecSize; i++) {
        //std::cout << "Input element " << i << ": ";
        std::cin >> vec[i];
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    sortVector(vec, maxThreads);
    auto t2 = std::chrono::high_resolution_clock::now();

    // std::cout << "[";
    // for (int i = 0; i < vecSize - 1; i++) {
    //     std::cout << vec[i] << ", ";
    // }

    // std::cout << vec[vecSize - 1] << "]" << std::endl;

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count(); 
    std::cout << "Execution takes: " << duration << " micro secs" << std::endl;

    return 0;
}
