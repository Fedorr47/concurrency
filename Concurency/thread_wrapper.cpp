#pragma once 

#include <future>
#include <thread>
#include <functional>

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
		~clear_cv_on_destruct()
		{
			this_thread_interrupt_flag.clear_condition_variable();
		}
	};
};
thread_local interrupt_flag this_thread_interrupt_flag;

class interruptible_thread
{
	std::thread internal_thread;
	interrupt_flag* flag;
public:
	template<typename FunctionType>
	interruptible_thread(FunctionType f)
	{
		std::promise<interrupt_flag*> p;
		internal_thread = std::thread([f, &p] {
			p.set_value(&this_thread_interrupt_flag);
			f();
		});
		flag = p.get_future().get();
	}
	void interrupt()
	{
		if (flag)
		{
			flag->set();
		}
	}
};

void interruptible_wait(std::condition_variable& cv,
	std::unique_lock<std::mutex>& lk)
{
	interruption_point();
	this_thread_interrupt_flag.set_condition_variable(cv);
	interrupt_flag::clear_cv_on_destruct guard;
	interruption_point();
	cv.wait_for(lk, std::chrono::milliseconds(1));
	interruption_point();
}

namespace threadwrapper
{
	class ThreadWrapper
	{
	private:
		std::promise<void> promise_error_;
		std::future<void> result_error_;
		std::thread thread_wrapper_;
		std::function<void()> callback_function_;
		std::function<void()> callback_exception_;

		void threadFunction(std::promise<void>& promiseThreadError);

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

		bool isFinish();
		void run();
		void join_thread();
		void getException();
	};
}

namespace threadwrapper
{
	void ThreadWrapper::run()
	{
		promise_error_ = std::promise<void>();
		result_error_ = promise_error_.get_future();
		thread_wrapper_ = std::thread(&ThreadWrapper::threadFunction, this, std::ref(promise_error_));
	}

	void ThreadWrapper::threadFunction(std::promise<void>& promiseThreadError)
	{
		try
		{
			this->callback_function_();
		}
		catch (const std::exception& ex)
		{
			promiseThreadError.set_exception(std::current_exception());
			if (callback_exception_)
				callback_exception_();
			return;
		}
		catch (...)
		{
			promiseThreadError.set_exception(std::current_exception());
			if (callback_exception_)
				callback_exception_();
			return;
		}
		promiseThreadError.set_value();
	}

	void ThreadWrapper::join_thread()
	{
		thread_wrapper_.join();
		getException();
	}

	void ThreadWrapper::getException()
	{
		result_error_.get();
	}

	bool ThreadWrapper::isFinish()
	{
		return thread_wrapper_.joinable();
	}
}





