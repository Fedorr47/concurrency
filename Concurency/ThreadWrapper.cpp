#pragma once

#include <future>
#include <thread>
#include <functional>


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
		bool ThreadWrapper::isFinish()
		{
			return thread_wrapper_.joinable();
		}
		void ThreadWrapper::run()
		{
			promise_error_ = std::promise<void>();
			result_error_ = promise_error_.get_future();
			thread_wrapper_ = std::thread(&ThreadWrapper::threadFunction, this, std::ref(promise_error_));
		}
		void ThreadWrapper::getException()
		{
			result_error_.get();
		}
		void ThreadWrapper::join_thread()
		{
			thread_wrapper_.join();
			getException();
		}
		
	};
}



