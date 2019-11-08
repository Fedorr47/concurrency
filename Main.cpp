#include <iostream>
#include <thread>
#include <vector>

#include "threadsafe_queue.h"

#define PUSH_THREAD_COUNT 4
#define POP_THREAD_COUNT  4

using namespace std;

void main()
{
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