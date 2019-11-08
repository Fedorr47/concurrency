#include <thread>
#include <vector>
#include <deque>
#include <iostream>

#include "pch.h"
#include "../Concurency/threadsafe_queue.h"
#include "../Concurency/parallel_sort.h"
#include "../Concurency/thread_wrapper.h"

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

bool stop_thread = false;

class TestThreadSafeContainers : public ::testing::Test
{
protected:
	void SetUp()
	{
		
	}
	void TearDown()
	{
		
	}
	static void SetUpTestCase()
	{

	}
	static void TearDownTestCase()
	{

	}
};

class TestThreadInterrupt : public ::testing::Test
{
protected:
	void SetUp()
	{}
	void TearDown()
	{}
	static void SetUpTestCase()
	{}
	static void TearDownTestCase()
	{}
};

TEST_F(TestThreadSafeContainers, test_queue)
{
	
}

TEST_F(TestThreadSafeContainers, test_list)
{
	
}

TEST_F(TestThreadSafeContainers, parallel_sort)
{
	std::list<int> list_to_sort{ 9,7,10,6,4,5,1 };
	auto sorted_list = parallel_quick_sort(list_to_sort);
	std::copy(sorted_list.begin(), sorted_list.end(), ostream_iterator<int>(cout, "; "));
	std::cout << std::endl;
}

TEST_F(TestThreadInterrupt, interrupt_thread)
{
	auto function_thread = [&]() {
		std::mutex mx;
		std::condition_variable cv;
		std::unique_lock<mutex> lock_wait(mx);
		cout << "Begin thread" << endl; 
		while (!stop_thread)
			interruptible_wait(cv, lock_wait, [&]() {return stop_thread; });
		cout << "End thread" << endl;
	};
	ThreadWrapper interruptible_thread(function_thread);
	interruptible_thread.run();
	this_thread::sleep_for(chrono::seconds(2));
	cout << "Start interrupt" << endl;
	interruptible_thread.interrupt();
	cout << "End interrupt" << endl;
}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
