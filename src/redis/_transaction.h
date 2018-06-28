#pragma once

namespace flame {
namespace redis {
	class _command;
	class _transaction: public _command_base {
	public:
		_transaction(int m = _command_base::REPLY_EXEC);
		std::size_t writer() override;
		std::size_t reader(const char* data, std::size_t size) override;

		void append(std::shared_ptr<_command> cmd);
	private:
		std::list<std::shared_ptr<_command>> qs_;
		std::list<std::shared_ptr<_command>> qr_;
		int status_;
	};
}
}