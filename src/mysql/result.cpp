#include "result.h"

namespace flame {
namespace mysql {
	void result::declare(php::extension_entry& ext) {
		php::class_entry<result> class_result("flame\\mysql\\result");
		class_result
			.property({"affected_rows", 0})
			.property({"insert_id", 0})
			.method<&result::fetch_row>("fetch_row")
			.method<&result::fetch_all>("fetch_all");

		ext.add(std::move(class_result));
	}
	result::result()
	: r_(nullptr) {
		
	}
	result::~result() {
		if(r_ != nullptr) mysql_free_result(r_);
	}
	php::value result::fetch_row(php::parameters& params) {
		if(r_ == nullptr) return nullptr;

		php::array data_row(4);
		MYSQL_ROW row;
		unsigned int num_fields;
		MYSQL_FIELD *fields;
		num_fields = mysql_num_fields(r_);
		fields = mysql_fetch_fields(r_);
		while ((row = mysql_fetch_row(r_))) {
			
			unsigned long *lengths;
			lengths = mysql_fetch_lengths(r_);

			for(int i = 0; i < num_fields; i++) {
				data_row.set( php::string(fields[i].name), php::string(row[i], lengths[i]) );
			}
			break;
		}
		return std::move(data_row);
	}
	php::value result::fetch_all(php::parameters& params) {
		if(r_ == nullptr) return nullptr;

		php::array data_all(4);
		MYSQL_ROW row;
		unsigned int num_fields;
		MYSQL_FIELD *fields;
		num_fields = mysql_num_fields(r_);
		fields = mysql_fetch_fields(r_);
		while ((row = mysql_fetch_row(r_))) {
			php::array data_row(4);
			unsigned long *lengths;
			lengths = mysql_fetch_lengths(r_);

			for(int i = 0; i < num_fields; i++) {
				data_row.set( php::string(fields[i].name), php::string(row[i], lengths[i]) );
			}
			
			data_all.set(data_all.size(), data_row);
		}
		return std::move(data_all);
	}
}
}