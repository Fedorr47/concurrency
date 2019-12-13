#include <iostream>
#include <thread>
#include <vector>
#include <deque>

#include "threadsafe_queue.h"

#define PUSH_THREAD_COUNT 4
#define POP_THREAD_COUNT  4

using namespace std;

struct Block
{
	size_t size_;
	std::string data_;
	explicit Block() :
		size_(sizeof(Block))
		, data_("")
	{}
	Block(std::string data) :
		size_(sizeof(Block)),
		data_(data)
	{}
};

void main()
{
	std::deque<std::unique_ptr<Block>> deq_blocks;
	auto new_block = std::make_unique<Block>(
		std::string("Very loooooooooooooooooooong string...............")
	);
	cout << (*new_block).data_ << sizeof(*new_block) << endl;
	deq_blocks.push_front(std::move(new_block));
	threadsafe_queue<int> stack(2);
	vector<thread> push_thread_pools;
	vector<thread> pop_thread_pools;
	int counter_loop = 0;
	for (counter_loop = 0; counter_loop < PUSH_THREAD_COUNT; ++counter_loop)
		push_thread_pools.push_back(thread([&]() {
		for (;;)
		{
			stack.wait_and_push(counter_loop);
		} 
		})
		);
	for (counter_loop = 0; counter_loop < POP_THREAD_COUNT; ++counter_loop)
		pop_thread_pools.push_back(thread([&]() {
			for (;;) 
			{
				auto value = stack.wait_and_pop();
				cout << "Thread pop get value = " << *value.get() << endl;
			} 
		})
		);
	pop_thread_pools[0].join();
}