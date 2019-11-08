#pragma once 

#include <future>
#include <thread>
#include <functional>

class thread_interrupted : public std::exception
{
private:
	std::string m_what_;
public:
	thread_interrupted(
		std::string what
	) : m_what_(what)
	{
	}
	virtual const char* what() const noexcept
	{
		return m_what_.c_str();
	}
};
class interrupt_flag
{
	std::atomic<bool> flag;
	std::condition_variable* thread_cond;
	std::mutex set_clear_mutex;

public:
	interrupt_flag() :
		thread_cond(0)
	{}

	void set()
	{
		flag.store(true, std::memory_order_relaxed);
		std::lock_guard<std::mutex> lk(set_clear_mutex);
		if (thread_cond)
		{
			thread_cond->notify_all();
		}
	}

	bool is_set() const
	{
		return flag.load(std::memory_order_relaxed);
	}

	void set_condition_variable(std::condition_variable& cv)
	{
		std::lock_guard<std::mutex> lk(set_clear_mutex);
		thread_cond = &cv;
	}

	void clear_condition_variable()
	{
		std::lock_guard<std::mutex> lk(set_clear_mutex);
		thread_cond = 0;
	}

	struct clear_cv_on_destruct
	{
		~clear_cv_on_destruct();
	};
};
thread_local interrupt_flag this_thread_interrupt_flag;
interrupt_flag::clear_cv_on_destruct::~clear_cv_on_destruct()
{
	this_thread_interrupt_flag.clear_condition_variable();
}
void interruption_point()
{
	if (this_thread_interrupt_flag.is_set())
	{
		throw thread_interrupted("Interupt");
	}
}
template<class _Predicate>
void interruptible_wait(std::condition_variable& cv,
	std::unique_lock<std::mutex>& lk,
	_Predicate predic = nullptr
	)
{
	interruption_point();
	this_thread_interrupt_flag.set_condition_variable(cv);
	interrupt_flag::clear_cv_on_destruct guard;
	interruption_point();
	cv.wait_for(lk, std::chrono::milliseconds(1));
	interruption_point();
}


class ThreadWrapper
{
private:
	std::promise<void> promise_result_;
	std::future<void> result_;
	std::thread thread_wrapper_;
	std::function<void()> callback_function_;
	std::function<void()> callback_exception_;
	interrupt_flag* flag;

	void threadFunction(
		std::promise<void>& promise_result,
		std::promise<interrupt_flag*>&
	);

public:
	template<typename Callable, typename... Args>
	explicit
		ThreadWrapper(Callable&& callbackFunc, Args&& ... args)
	{
		callback_function_ = std::bind(callbackFunc, args...);
	}
	~ThreadWrapper()
	{
		if (thread_wrapper_.joinable())
			thread_wrapper_.join();
	}
	template<typename Callable, typename... Args>
	void set_callback_exception(Callable&& callbackFunc, Args&& ... args)
	{
		callback_exception_ = std::bind(callbackFunc, args...);
	}
	void interrupt()
	{
		if (flag)
		{
			flag->set();
		}
	}

	bool isFinish();
	void run();
	void join_thread();
	void get();
};


void ThreadWrapper::run()
{
	promise_result_ = std::promise<void>();
	result_ = promise_result_.get_future();
	std::promise<interrupt_flag*> p;
	thread_wrapper_ = std::thread(
		&ThreadWrapper::threadFunction, 
		this, 
		std::ref(promise_result_),
		std::ref(p)
	);
	flag = p.get_future().get();
}

void ThreadWrapper::threadFunction(
	std::promise<void>& promise_result,
	std::promise<interrupt_flag*>& p
)
{
	try
	{
		p.set_value(reinterpret_cast<interrupt_flag*>(&this_thread_interrupt_flag));
		this->callback_function_();
	}
	catch (const std::exception& ex)
	{
		promise_result.set_exception(std::current_exception());
		if (callback_exception_)
			callback_exception_();
		return;
	}
	catch (...)
	{
		promise_result.set_exception(std::current_exception());
		if (callback_exception_)
			callback_exception_();
		return;
	}
	promise_result.set_value();
}

void ThreadWrapper::join_thread()
{
	thread_wrapper_.join();
	get();
}

void ThreadWrapper::get()
{
	result_.get();
}

bool ThreadWrapper::isFinish()
{
	return thread_wrapper_.joinable();
}






