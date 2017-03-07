#pragma once
#include <queue>  
#include <memory>  
#include <mutex>  
#include <condition_variable> 

template<typename T>
class threadsafe_queue
{
private:
	mutable std::mutex mut;
	std::queue<T> data_queue;
	std::condition_variable data_cond;
public:
	threadsafe_queue() {}
	threadsafe_queue(threadsafe_queue const& other)
	{
		std::lock_guard<std::mutex> lk(other.mut);
		data_queue = other.data_queue;
	}
	~threadsafe_queue()
	{
		
	}
	void push(T& new_value)//入队操作  
	{
		std::lock_guard<std::mutex> lk(mut);
		data_queue.push(new_value);
		data_cond.notify_one();
	}
	T wait_and_pop(void)//直到有元素可以删除为止  
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		auto value = data_queue.front();
		data_queue.pop();
		return value;
	}
	T front(void)
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		return data_queue.front();
	}
	void pop(void)//Wait and pop
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		data_queue.pop();
	}
	size_t size(void)
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		size_t retv = data_queue.size();
		return retv;
	}
	//std::shared_ptr<T> wait_and_pop()
	//{
	//	std::unique_lock<std::mutex> lk(mut);
	//	data_cond.wait(lk, [this] {return !data_queue.empty(); });
	//	std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
	//	data_queue.pop();
	//	return res;
	//}
	bool try_pop(T& value)//不管有没有队首元素直接返回  
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return false;
		value = data_queue.front();
		data_queue.pop();
		return true;
	}
	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return std::shared_ptr<T>();
		std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
		data_queue.pop();
		return res;
	}
	bool empty() const
	{
		std::lock_guard<std::mutex> lk(mut);
		return data_queue.empty();
	}
};