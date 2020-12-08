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
int WeDidIt = 0;
atomic <int> SumRead(0);

class BaseQueue {
public:
    virtual void push(uint8_t val) = 0;
    virtual bool try_pop(uint8_t& val) = 0;
    virtual bool empty() const = 0;
};

class threadsafe_queue : public BaseQueue
{
private:
    mutable mutex mut;
    queue<uint8_t> data_queue;
    condition_variable data_cond;
public:
    threadsafe_queue() {}
    threadsafe_queue(threadsafe_queue const& other)
    {
        lock_guard<mutex> lk(other.mut);
        data_queue = other.data_queue;
    }
    void push(uint8_t new_value)
    {
        //lock_guard<mutex> lk(mut);
        unique_lock<mutex> MY_GUARD(mut);
        data_queue.push(new_value);
        MY_GUARD.unlock();
        data_cond.notify_one();
    }
    bool try_pop(uint8_t& value)
    {
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

class fixedsize_queue : public BaseQueue
{
private:
    mutable mutex mut;
    queue<uint8_t> data_queue;
    uint8_t max_size = 4;
    condition_variable push_cond;
    condition_variable pop_cond;
public:
    fixedsize_queue() {}
    fixedsize_queue(fixedsize_queue const& other)
    {
        lock_guard<mutex> lk(other.mut);
        data_queue = other.data_queue;
    }
    void push(uint8_t new_value)
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
};


void data_preparation_thread(BaseQueue& data_queue) // producers
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

void data_processing_thread(BaseQueue& data_queue) // consumers
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
    SumRead.store(counter + SumRead.load());
}

void setVariablesDefault() {
    SumRead.store(0);
    WeDidIt = 0;

}

void createConsumersProducers(int cons, int prod, char queueType) {
    vector<thread> consumers;
    vector<thread> producers;
    ConsumerNum = cons;
    ProducerNum = prod;
    BaseQueue* data_queue;
    auto begin = high_resolution_clock::now();

    if (queueType == 'd') // for Dynamic
    {
        data_queue = new threadsafe_queue();
        begin = high_resolution_clock::now(); //time start
        for (int j = 0; j < ConsumerNum; j++) {
            thread thr(data_processing_thread, std::ref(*data_queue));
            consumers.emplace_back(move(thr));
        }
        // Producers
        for (int j = 0; j < ProducerNum; j++) {
            thread thr(data_preparation_thread, std::ref(*data_queue));
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
    }
    else if (queueType == 'f') // For fixed
    {
        data_queue = new fixedsize_queue();
        begin = high_resolution_clock::now(); //time start
        for (int j = 0; j < ConsumerNum; j++) {
            thread thr(data_processing_thread, std::ref(*data_queue));
            consumers.emplace_back(move(thr));
        }
        // Producers
        for (int j = 0; j < ProducerNum; j++) {
            thread thr(data_preparation_thread, std::ref(*data_queue));
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
    }

    auto time = high_resolution_clock::now() - begin; // time finish
    cout << "runtime = " << duration<double>(time).count() << endl;

    cout << "Producers: " << ProducerNum << ". Consumers: " << ConsumerNum << endl;
    if (SumRead.load() == ProducerNum * TaskNum)
        cout << "OK" << endl;
    else
        cout << "NOT OK " << endl;

    setVariablesDefault();

}

int main()
{
    createConsumersProducers(1, 1, 'd');
    createConsumersProducers(2, 2, 'd');
    createConsumersProducers(4, 4, 'd');

    //Fixed size queue
    cout << "\n\nThreadsafe buffer queue:\n";
    createConsumersProducers(1, 1, 'f');
    createConsumersProducers(2, 2, 'f');
    createConsumersProducers(4, 4, 'f');

    cout << "Job done in main\n";
    return 0;
}
