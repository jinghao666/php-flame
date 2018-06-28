#pragma once

namespace flame {
	class coroutine;
namespace redis {
	class _command_base;
	class _transaction;
	class _client: public std::enable_shared_from_this<_client> {
	public:
		_client();
		void connect(std::shared_ptr<flame::coroutine> co, std::shared_ptr<php::url> url);
		// 加入发送队列，若当前还未进行发送，启动发送过程（）
		void send(std::shared_ptr<flame::coroutine> co, _command_base* base);
		void read();
		// TRANSACTION
		std::shared_ptr<_transaction> multi(bool pipe = false);
	private:
		tcp::socket socket_;
		// 发送队列
		std::list<_command_base*> qs_;
		// 接收队列
		std::list<_command_base*> qr_;
		// 缓冲区
		php::buffer rbuffer_;
		// 若不存在 STATUS_FLAG_WRITING 则开始发送；
		boost::asio::coroutine coro_s;
		void send_ex(const boost::system::error_code& error = boost::system::error_code(), std::size_t n = 0);
		// 若不存在 STATUS_FLAG_READING 则开始接收；
		boost::asio::coroutine coro_r;
		void read_ex(const boost::system::error_code& error = boost::system::error_code(), std::size_t n = 0);
		enum status_flag {
			STATUS_FLAG_SENDING = 0x01,
			STATUS_FLAG_READING = 0x02,
		};
		int status_;
		friend class _command;
		friend class _transaction;
	};
}
}