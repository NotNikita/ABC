#include <mutex>
#include <thread>
#include <iostream>
#include <chrono>
#include <memory>
#include <condition_variable>
#include <queue>
#include <vector>
#include <queue>
#include <atomic>


using namespace std;
using namespace std::chrono;


int ConsumerNum;
int ProducerNum;
const int TaskNum = 4 * 1024 * 1024;
int TotalRead = 0;
int WeDidIt = 0;
atomic <int> SumRead( 0 );


template<typename T>
class threadsafe_queue
{
private:
    mutable mutex mut;
    queue<T> data_queue;
    condition_variable data_cond;
public:
    threadsafe_queue(){}
    threadsafe_queue(threadsafe_queue const& other)
    {
        lock_guard<mutex> lk(other.mut);
        data_queue = other.data_queue;
    }
    void push(T new_value)
    {
        //lock_guard<mutex> lk(mut);
        unique_lock<mutex> MY_GUARD(mut);
        data_queue.push(new_value);
        MY_GUARD.unlock();
        data_cond.notify_one();
    }
    bool try_pop(T& value)
    {
        /*
        Locks the associated mutex by calling m.lock().
        The behavior is undefined if the current thread already owns the mutex except when the mutex is recursive.
        */
        unique_lock<mutex> MY_GUARD(mut);
        if (data_queue.empty()) {
            MY_GUARD.unlock();
            this_thread::sleep_for(chrono::milliseconds(1));
            MY_GUARD.lock();
            if (data_queue.empty()) {
                return false;
            }
            else {
                value = data_queue.front();
                data_queue.pop();
                return true;
            }
        }
        else {
            value = data_queue.front();
            data_queue.pop();
            return true;
        }       
        
    }
    bool empty() const
    {
        lock_guard<mutex> lk(mut);
        return data_queue.empty();
    }
};

class fixedsize_queue
{
private:
    mutable mutex mut;
    queue<uint8_t> data_queue;
    uint8_t max_size;
    condition_variable push_cond;
    condition_variable pop_cond;
public:
    fixedsize_queue(uint8_t _size) { this->max_size = _size; }
    fixedsize_queue(fixedsize_queue const& other)
    {
        lock_guard<mutex> lk(other.mut);
        data_queue = other.data_queue;
    }
    void set_size(int new_size) { this->max_size = new_size; }
    void try_push(uint8_t new_value)
    {
        std::unique_lock<std::mutex> MY_GUARD(mut);
        push_cond.wait(MY_GUARD, [this] {return max_size > data_queue.size(); });
        data_queue.push(new_value);
        pop_cond.notify_one();
    }
    bool try_pop(uint8_t& value)
    {
        std::unique_lock<std::mutex> MY_GUARD(mut);
        if (pop_cond.wait_for(MY_GUARD, std::chrono::milliseconds(1), [this] { return data_queue.size(); })) {
            value = data_queue.front();
            data_queue.pop();
            push_cond.notify_one();
            return true;
        }
        return false;

    }
    bool empty() const
    {
        lock_guard<mutex> lk(mut);
        return data_queue.empty();
    }
    bool size_ok()
    {
        lock_guard<mutex> lk(mut);
        return max_size > data_queue.size();
    }
};
threadsafe_queue<uint8_t> data_queue;
fixedsize_queue fixed_queue = fixedsize_queue(1);

void data_preparation_thread() // producers
{
    int myBalance = TaskNum;
    uint8_t const data = UINT8_MAX; // 255
    while (myBalance > 255)
    {
        data_queue.push(data);
        myBalance = myBalance - data;
    }
    data_queue.push(myBalance); // остаток от деления myBalance на 255 == 64
    WeDidIt += 1;
}

void data_processing_thread() // consumers
{
    uint32_t counter = 0;//Потребители инстанциируют локальный счетчик
    uint8_t value;
    bool my_boi = true;
    while (my_boi)
    {
        value = 0;
        my_boi = data_queue.try_pop(ref(value));
        counter += value;
        // Потребители опустошили очередь
        /* ----DEPRECATED IN VERSION 2.0.1----
        if (WeDidIt == ProducerNum && data_queue.empty()){
            TotalRead += counter;
            break;
        }*/ 
    }
    TotalRead += counter;
}

void setVariablesDefault() {
    TotalRead = 0;
    WeDidIt = 0;

}

void createConsumersProducers(int cons, int prod) {
    vector<thread> consumers;
    vector<thread> producers;
    ConsumerNum = cons;
    ProducerNum = prod;

    auto begin = high_resolution_clock::now(); //time start
    for (int j = 0; j < ConsumerNum; j++) {
        thread thr(data_processing_thread);
        consumers.emplace_back(move(thr));
    }
    // Producers
    for (int j = 0; j < ProducerNum; j++) {
        thread thr(data_preparation_thread);
        producers.emplace_back(move(thr));
    }

    for (auto& thr : producers)
    {
        thr.join();
    }

    for (auto& thr : consumers)
    {
        thr.join();
    }

    auto time = high_resolution_clock::now() - begin; // time finish
    cout << "runtime = " << duration<double>(time).count() << endl;

    cout << "Producers: " << ProducerNum << ". Consumers: " << ConsumerNum << endl;
    if (TotalRead == ProducerNum * TaskNum)
        cout << "OK" << endl;
    else
        cout << "NOT OK " <<endl;
    setVariablesDefault();

}

// Fixed size queue

void fixed_data_preparation_thread() // producers
{
    int myBalance = TaskNum;
    uint8_t const data = UINT8_MAX; // 255
    while (myBalance > 255)
    {
        fixed_queue.try_push(data);
        myBalance = myBalance - data;
    }
    fixed_queue.try_push(myBalance); // остаток от деления myBalance на 255 == 64
    WeDidIt += 1;
}

void fixed_data_processing_thread() // consumers
{
    uint32_t counter = 0;//Потребители инстанциируют локальный счетчик
    uint8_t value;
    bool my_boi = true;
    while (my_boi)
    {
        value = 0;
        my_boi = fixed_queue.try_pop(ref(value));
        counter += value;
    }
    TotalRead += counter;
}

void fixed_createConsumersProducers() {
    vector<thread> consumers;
    vector<thread> producers;

    auto begin = high_resolution_clock::now(); //time start
    for (int j = 0; j < 4; j++) {
        thread thr(fixed_data_processing_thread);
        consumers.emplace_back(move(thr));
    }
    // Producers
    for (int j = 0; j < 4; j++) {
        thread thr(fixed_data_preparation_thread);
        producers.emplace_back(move(thr));
    }

    for (auto& thr : producers)
    {
        thr.join();
    }

    for (auto& thr : consumers)
    {
        thr.join();
    }

    auto time = high_resolution_clock::now() - begin; // time finish
    cout << "runtime = " << duration<double>(time).count() << endl;

    cout << "Producers: 4. Consumers: 4\n";
    if (TotalRead == 4 * TaskNum)
        cout << "OK" << endl;
    else
        cout << "NOT OK" << endl;

    setVariablesDefault();

}
/*
when push : counter in queue +1
when pop : counter -1
checking size when push to push
thats it
*/

int main()
{
    createConsumersProducers(1, 1);
    createConsumersProducers(2, 2);
    createConsumersProducers(4, 4);

    //Fixed size queue
    cout << "\n\nThreadsafe buffer queue:\n";
    fixed_createConsumersProducers();
    fixed_queue.set_size(4);
    fixed_createConsumersProducers();
    fixed_queue.set_size(16);
    fixed_createConsumersProducers();

    cout << "Job done in main\n";
    return 0;
}
