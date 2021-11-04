#include <iostream>
#include <thread>
#include <atomic>
#include "MultiQueueProcessor.h"

std::atomic_bool sProcessing = false;

template<typename Key, typename Value>
class Consumer : public IConsumer<Key, Value>
{
public:
    Consumer(const std::string& name)
        : name_(name)
    {}

    virtual void Consume(Key id, const Value& value) override
    {
        std::cout << name_ << " " << id << " " << value << std::endl;
    }
private:
    std::string name_;
};

template<typename Key, typename Value>
void DoThread(MultiQueueProcessor<Key, Value>* mqp, const Key& key, Value start )
{
    Value i = start;
    try
    {
        while (sProcessing.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            mqp->Enqueue(key, i++);
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
}

int main()
{
    try
    {
        MultiQueueProcessor<std::string, int> mqp;

        sProcessing.store(true);

        std::thread th1(&DoThread<std::string, int>, &mqp, "t1", 0);
        std::thread th2(&DoThread<std::string, int>, &mqp, "t2", 10);
        std::thread th3(&DoThread<std::string, int>, &mqp, "t1", 100);
        std::thread th4(&DoThread<std::string, int>, &mqp, "t2", 1000);
        std::thread th5(&DoThread<std::string, int>, &mqp, "t1", 10000);

        Consumer<std::string, int> c1("C1");
        mqp.Subscribe("t1", &c1);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        mqp.Unsubscribe("t1");

        std::cout << "Main enqueue started" << std::endl;

        for (int i = 0; i < 100; ++i)
        {
            mqp.Enqueue("t1", i);
        }

        std::cout << "Main enqueue finished, dequeue started" << std::endl;

        for (int i = 0; i < 100; ++i)
        {
            std::cout << "Main: " << "t1" << " " << mqp.Dequeue("t1") << std::endl;
        }

        std::cout << "Main dequeue finished" << std::endl;

        Consumer<std::string, int> c2("C2");
        mqp.Subscribe("t1", &c2);
        mqp.Subscribe("t2", &c1);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        sProcessing.store(false);

        th1.join();
        th2.join();
        th3.join();
        th4.join();
        th5.join();
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    return 0;
}
