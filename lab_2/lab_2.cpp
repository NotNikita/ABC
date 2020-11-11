#include <mutex>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>


using namespace std;
using namespace std::chrono;

mutex door;
mutex io_mtx;
const int NumTasks = 1024 * 1024;
int arr[NumTasks] = {0};
int counter = 0;
static atomic_int atomic_counter(0);
bool sleep = true;
static const int MAX_COUNTER_VAL = NumTasks;

void printArray(int* arr, int len) {
    for (int i = 0; i < len; i++) {
        cout << arr[i] << ' ';
    }
    cout << endl;
}

void thread_procMutex(int tnum) {
    
    for (;;) 
    {
        {
            lock_guard<mutex> lock(door);
            
            if (counter == MAX_COUNTER_VAL)
                break;
            arr[counter] += 1;
            int ctr_val = ++counter;
            //cout << "Thread " << tnum << ": index = " << ctr_val << endl;
        }
        if (sleep) this_thread::sleep_for(chrono::nanoseconds(10));
    }
    
}

void createThreadsMutex(int t_amount) {
    vector<thread> threads;
    auto begin = high_resolution_clock::now(); //time start

    for (int j = 0; j < t_amount; j++) {
        thread thr(thread_procMutex, j);
        threads.emplace_back(move(thr));
    }

    for (auto& thr : threads)
    {
        thr.join();
    }
    auto time = high_resolution_clock::now() - begin; // time finish
    cout << "threads: "<< t_amount  <<". runtime = " << duration<double>(time).count() << endl;
    counter = 0;
}

void thread_procAtomic(int tnum) {

    for (;;) {
        {
            int ctr_val = ++atomic_counter;
            if (ctr_val >= MAX_COUNTER_VAL)
                break;
            {
                std::lock_guard<std::mutex> lock(io_mtx);
            }
        }
        if (sleep) std::this_thread::sleep_for(std::chrono::nanoseconds(10));
    }
}

void createThreadsAtomic(int t_amount) {
    vector<thread> threads;
    auto begin = high_resolution_clock::now(); //time start

    for (int i = 0; i < t_amount; i++) {
        std::thread thr(thread_procAtomic, i);
        threads.emplace_back(std::move(thr));
    }

    for (auto& thr : threads) {
        thr.join();
    }

    auto time = high_resolution_clock::now() - begin; // time finish
    cout << "threads: " << t_amount << ". runtime = " << duration<double>(time).count() << endl;
    atomic_counter = 0;
}

int main()
{
    cout << "Mutex with sleep:" << endl;
    createThreadsMutex(4);
    createThreadsMutex(8);
    createThreadsMutex(16);
    createThreadsMutex(32);
    sleep = false;
    cout << "No sleep:" << endl;
    createThreadsMutex(4);
    createThreadsMutex(8);
    createThreadsMutex(16);
    createThreadsMutex(32);
    //
    sleep = true;
    cout << "\nAtomic with sleep:" << endl;
    createThreadsAtomic(4);
    createThreadsAtomic(8);
    createThreadsAtomic(16);
    createThreadsAtomic(32);
    sleep = false;
    cout << "No sleep:" << endl;
    createThreadsAtomic(4);
    createThreadsAtomic(8);
    createThreadsAtomic(16);
    createThreadsAtomic(32);
    
    //printArray(arr, NumTasks);
    return 0;
}

/*
void thread_proc(int tnum) {
    for (;;) {
        {
            lock_guard<mutex> lock(door);
            if (counter == MAX_COUNTER_VAL)
                break;
            int ctr_val = ++counter;
            cout << "Thread " << tnum << ": counter = " <<
                ctr_val << endl;
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}
*/