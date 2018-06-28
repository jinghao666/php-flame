#include "_command_base.h"
#include "_transaction.h"

namespace flame {
namespace redis {
	_transaction::_transaction(int m)
	: _command_base(m)
	, status_(0) {

	}
	std::size_t _transaction::writer() {
		// TODO 实现TRANSACTION 写入
		if(status_ == 0) {
			
		}
		return 0;
	}
	std::size_t _transaction::reader(const char* data, std::size_t size) {
		// TODO 实现TRANSACTION 解析
		return 0;
	}
	void _transaction::append(std::shared_ptr<_command> cmd) {
		qs_.push_back(cmd);
	}
}
}