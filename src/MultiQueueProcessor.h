#pragma once
#include <map>
#include <list>
#include <mutex>
#include <memory>

template<typename Key, typename Value>
struct IConsumer
{
    virtual void Consume(Key id, const Value& value)
    {
        id;
        value;
    }
};

#define MaxCapacity 1000

template<typename Key, typename Value>
class MultiQueueProcessor
{
public:

    MultiQueueProcessor()
    {
    }

    ~MultiQueueProcessor()
    {
        std::lock_guard<std::recursive_mutex> lock{ lock_queues_ };

        for (auto it = queues_.begin(); it != queues_.end();)
        {
            it->second->lock_queue_.lock();
            it = queues_.erase(it);
        }
    }

    void Subscribe(Key id, IConsumer<Key, Value>* consumer)
    {
        auto pQueue = AccessQueue(id, true);

        if (nullptr == pQueue)
            throw std::logic_error("Subscribe - unexpected, queue not exists");

        auto queueUnaccess = [this, pQueue, &id]() {UnaccessQueue(pQueue, id); };
        SQueueGuard<decltype(queueUnaccess)> qGuard(queueUnaccess);

        if (pQueue->HasSubscriber())
        {
            throw std::logic_error("Subscribe - queue already have a subscriber");
        }

        pQueue->Subscribe(consumer);

        while (!pQueue->IsEmpty())
        {
            const auto value = pQueue->Dequeue();
            pQueue->subscriber_->Consume(id, value);
        }
    }

    void Unsubscribe(Key id)
    {
        auto pQueue = AccessQueue(id);

        if (nullptr == pQueue)
            throw std::invalid_argument("Unsubscribe - invalid key value");

        auto queueUnaccess = [this, pQueue, &id]() {UnaccessQueue(pQueue, id); };
        SQueueGuard<decltype(queueUnaccess)> qGuard(queueUnaccess);

        if (!pQueue->HasSubscriber())
        {
            throw std::logic_error("Unsubscribe - queue have no subscriber");
        }

        pQueue->Unsubscribe();
    }

    void Enqueue(Key id, const Value& value)
    {
        auto pQueue = AccessQueue(id, true);

        if (nullptr == pQueue)
            throw std::logic_error("Enqueue - unexpected, queue not exists");

        auto queueUnaccess = [this, pQueue, &id]() {UnaccessQueue(pQueue, id); };
        SQueueGuard<decltype(queueUnaccess)> qGuard(queueUnaccess);

        if (pQueue->IsFull())
        {
            throw std::logic_error("Enqueue - queue is full");
        }

        if (pQueue->HasSubscriber())
        {
            pQueue->subscriber_->Consume(id, value);
        }
        else
        {
            pQueue->Enqueue(value);
        }
    }

    Value Dequeue(Key id)
    {
        auto pQueue = AccessQueue(id);

        if (nullptr == pQueue)
            throw std::invalid_argument("Dequeue - invalid key value");

        auto queueUnaccess = [this, pQueue, &id]() {UnaccessQueue(pQueue, id); };
        SQueueGuard<decltype(queueUnaccess)> qGuard(queueUnaccess);

        if (pQueue->HasSubscriber())
        {
            throw std::logic_error("Dequeue - queue has subscriber");
        }

        if (pQueue->IsEmpty())
        {
            throw std::out_of_range("Dequeue - queue is empty");
        }

        return pQueue->Dequeue();
    }

private:

    struct SQueue
    {
        ~SQueue()
        {
            lock_queue_.unlock();
        }

        void Enqueue(const Value& value)
        {
            values_.push_back(value);
        }

        Value Dequeue() noexcept
        {
            auto front = values_.front();
            values_.pop_front();
            return front;
        }

        bool IsFull() const noexcept
        {
            return values_.size() >= MaxCapacity;
        }

        bool IsEmpty() const noexcept
        {
            return values_.empty();
        }

        bool HasSubscriber() const noexcept
        {
            return nullptr != subscriber_;
        }

        void Subscribe(IConsumer<Key, Value>* consumer) noexcept
        {
            subscriber_ = consumer;
        }

        void Unsubscribe() noexcept
        {
            subscriber_ = nullptr;
        }

        std::list<Value> values_;
        IConsumer<Key, Value>* subscriber_ = nullptr;
        std::recursive_mutex lock_queue_;
    };

    template <typename Deleter>
    struct SQueueGuard
    {
        SQueueGuard(const Deleter& deleter)
            : deleter_(deleter)
        {
        }
        ~SQueueGuard()
        {
            deleter_();
        }
        Deleter deleter_;
    };

    SQueue* AccessQueue(Key id, bool bCreate = false)
    {
        const auto iter = queues_.find(id);
        if (iter == queues_.cend())
        {
            if (bCreate)
            {
                std::lock_guard<std::recursive_mutex> lock{ lock_queues_ };

                auto emplaced = queues_.emplace(id, std::make_unique<SQueue>());
                emplaced.first->second->lock_queue_.lock();
                return emplaced.first->second.get();
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            iter->second->lock_queue_.lock();
            return iter->second.get();
        }
    }

    void UnaccessQueue(SQueue* pQueue, Key id)
    {
        if (pQueue->IsEmpty() && !pQueue->HasSubscriber())
        {
            std::lock_guard<std::recursive_mutex> lock{ lock_queues_ };
            queues_.erase(id);
        }
        else
        {
            pQueue->lock_queue_.unlock();
        }
    }

    std::map<Key, std::unique_ptr<SQueue>> queues_;
    std::recursive_mutex lock_queues_;
};
