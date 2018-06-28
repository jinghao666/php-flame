#include "../controller.h"
#include "../coroutine.h"
#include "../time/time.h"
#include "logger.h"

namespace flame {
namespace log {
	void logger::declare(php::extension_entry& ext) {
		php::class_entry<logger> class_logger("flame\\log\\logger");
		class_logger
			.method<&logger::__construct>("__construct", {
				{"filepath", php::TYPE::STRING, false, true},
			})
			.method<&logger::write>("write", {
				{"message", php::TYPE::STRING}
			})
			.method<&logger::info>("info", {
				{"message", php::TYPE::STRING}
			})
			.method<&logger::warn>("warn", {
				{"message", php::TYPE::STRING}
			})
			.method<&logger::fail>("fail", {
				{"message", php::TYPE::STRING}
			})
			.method<&logger::rotate>("rotate");

		ext.add(std::move(class_logger));
	}
	logger::~logger() {
		if(controller_->type == controller::MASTER) {
			queue_->send("c", 1, 0);
			write_.join();
		}
	}
	void logger::initialize() {
		if(fpath_.size() == 0) {
			fpath_ = controller_->environ["FLAME_PROCESS_TITLE"].to_string();
		}
		php::md5(reinterpret_cast<const unsigned char*>(fpath_.c_str()), fpath_.size(), qname_);
		if(controller_->type == controller::MASTER) {
			boost::interprocess::message_queue::remove(qname_);
			queue_.reset(new boost::interprocess::message_queue(
				boost::interprocess::create_only, qname_, 256, 8192
			));
			write_ = std::thread([this] () {
ROTATING:
				static std::shared_ptr<std::ostream> file;
				static char     data[8192];
				static size_t   size;
				static unsigned sort;
				
				if(fpath_[0] == '/') { // 文件路径
					file.reset(new std::ofstream(fpath_, std::ios_base::out | std::ios_base::app));
				}else{ // 进程标题
					file.reset(&std::clog, boost::null_deleter());
				}
				while(true) {
					queue_->receive(&data, 8192, size, sort);
					switch(data[0]) {
					case 'c':
						goto CLOSING; // 退出停止机制
					case 'r':
						goto ROTATING; // 重载日志文件
					default:
						file->put('[');
						file->write(time::datetime(), 19);
						file->put(']');
						file->put(' ');
						file->write(data, size);
						file->put('\n');
						file->flush();
					}
				}
CLOSING:		
				boost::interprocess::message_queue::remove(qname_);
			});
		}else{
			// open_or_create 方便调试
			queue_.reset(new boost::interprocess::message_queue(
				boost::interprocess::open_or_create, qname_, 256, 8192
			));
		}
	}
	void logger::write(const php::string& data) {
		queue_->send(data.c_str(), data.size(), 0);
	}
	void logger::rotate() {
		queue_->send("r", 1, 0);
	}
	bool logger::to_file() {
		return fpath_[0] == '/';
	}
	php::value logger::__construct(php::parameters& params) {
		fpath_ = params[0].to_string();
		initialize();
		if(controller_->type == controller::MASTER) {
			// 主进程在 SIGUSR2 时重载日志(文件)
			signal_.reset(new boost::asio::signal_set(context));
			signal_->async_wait(std::bind(&logger::on_sigusr2, this, std::placeholders::_1, std::placeholders::_2));
		}
		return nullptr;
	}
	php::value logger::write(php::parameters& params) {
		php::stream_buffer sb;
		std::ostream os(&sb);
		write_ex(os, params);
		write(std::move(sb));
		return nullptr;
	}
	php::value logger::info(php::parameters& params) {
		php::stream_buffer sb;
		std::ostream os(&sb);
		os << " (INFO)";
		write_ex(os, params);
		write(std::move(sb));
		return nullptr;
	}
	php::value logger::warn(php::parameters& params) {
		php::stream_buffer sb;
		std::ostream os(&sb);
		os << " (WARN)";
		write_ex(os, params);
		write(std::move(sb));
		return nullptr;
	}
	php::value logger::fail(php::parameters& params) {
		php::stream_buffer sb;
		std::ostream os(&sb);
		os << " (FAIL)";
		write_ex(os, params);
		write(std::move(sb));
		return nullptr;
	}
	php::value logger::rotate(php::parameters& params) {
		rotate();
		return nullptr;
	}
	void logger::write_ex(std::ostream& os, php::parameters& params) {
		for(int i=0;i<params.size();++i) {
			os << ' ' << params[i].ptr();
		}
	}

	void logger::on_sigusr2(const boost::system::error_code& error, int sig) {
		if(!error) {
			write("(INFO) logger rotating ...");
			rotate();
			signal_->async_wait(std::bind(&logger::on_sigusr2, this, std::placeholders::_1, std::placeholders::_2));
		}
	}
}
}