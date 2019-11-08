#pragma once
#include <mutex>
#include <chrono>
#include <memory>

template<typename T>
class threadsafe_queue
{
private:
	struct node
	{
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;
	};

	std::mutex head_mutex;
	std::unique_ptr<node> head;
	std::mutex tail_mutex;
	node* tail;
	std::condition_variable data_cond;
	std::condition_variable free_cond;
	size_t size_node{ sizeof(node) };
	size_t size_{ sizeof(node) };

	node* get_tail()
	{
		std::lock_guard<std::mutex> tail_lock(tail_mutex);
		return tail;
	}

	std::unique_ptr<node> pop_head()
	{
		std::unique_ptr<node> old_head = std::move(head);
		head = std::move(old_head->next);
		size_ += size_node;
		return old_head;
	}

	std::unique_lock<std::mutex> wait_for_data()
	{
		std::unique_lock<std::mutex> head_lock(head_mutex);
		data_cond.wait(head_lock, [&] {
			if (head.get() != get_tail())
			{
				return true;
			}
			else
			{
				free_cond.notify_one();
				return false;
			}
		});
		return std::move(head_lock);
	}

	std::unique_ptr<node> wait_pop_head()
	{
		std::unique_lock<std::mutex> head_lock(wait_for_data());
		return pop_head();
	}

	std::unique_ptr<node> wait_pop_head(T& value)
	{
		std::unique_lock<std::mutex> head_lock(wait_for_data());
		value = std::move(*head->data);
		return pop_head();
	}

	std::unique_ptr<node> try_pop_head()
	{
		std::lock_guard<std::mutex> head_lock(head_mutex);
		if (head.get() == get_tail())
		{
			return std::unique_ptr<node>();
		}
		return pop_head();
	}

	std::unique_ptr<node> try_pop_head(T& value)
	{
		std::lock_guard<std::mutex> head_lock(head_mutex);
		if (head.get() == get_tail())
		{
			return std::unique_ptr<node>();
		}
		value = std::move(*head->data);
		return pop_head();
	}
public:
	explicit threadsafe_queue(size_t count_size) :
		head(new node), 
		tail(head.get())
	{
		size_ *= count_size;
	}
	threadsafe_queue(const threadsafe_queue& other) = delete;
	threadsafe_queue& operator=(const threadsafe_queue& other) = delete;

	void wait_and_push(T new_value)
	{
		std::shared_ptr<T> new_data(
			std::make_shared<T>(std::move(new_value)));
		std::unique_lock<std::mutex> tail_lock(tail_mutex);
		free_cond.wait(tail_lock, [&] {return size_ != 0; });
		std::unique_ptr<node> p(new node);
		tail->data = new_data;
		node* const new_tail = p.get();
		tail->next = std::move(p);
		tail = new_tail;
		size_ -= size_node;
		data_cond.notify_one();
	}

	std::shared_ptr<T> wait_and_pop()
	{
		std::unique_ptr<node> const old_head = wait_pop_head();
		free_cond.notify_one();
		return old_head->data;
	}

	void wait_and_pop(T& value)
	{
		std::unique_ptr<node> const old_head = wait_pop_head(value);
	}

	std::shared_ptr<T> try_pop()
	{
		std::unique_ptr<node> const old_head = try_pop_head();
		free_cond.notify_one();
		return old_head ? old_head->data : std::shared_ptr<T>();
	}

	bool try_pop(T& value)
	{
		std::unique_ptr<node> const old_head = try_pop_head(value);
		free_cond.notify_one();
		return old_head;
	}

	void empty()
	{
		std::lock_guard<std::mutex> head_lock(head_mutex);
		return (head.get() == get_tail());
	}
};
