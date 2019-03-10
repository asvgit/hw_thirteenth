#include <iostream>
#include <cassert>
#include <sstream>
#include <memory>

#include <thread>
#include <boost/asio.hpp>

#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

using string = std::string;

class MysqlDbConnect {
	sql::Driver *driver;
	sql::Connection *con;
	sql::Statement *stmt;

public:
	MysqlDbConnect() {
		driver = get_driver_instance();
		con = (driver->connect("tcp://127.0.0.1:3306", "root", ""));
		con->setSchema("mysql");
		stmt = (con->createStatement());
		stmt->execute("DROP TABLE IF EXISTS friday_a");
		stmt->execute("CREATE TABLE friday_a(id INT, name char(255))");
		stmt->execute("DROP TABLE IF EXISTS friday_b");
		stmt->execute("CREATE TABLE friday_b(id INT, name char(255))");
	}
	~MysqlDbConnect() {
		delete con;
		delete stmt;
	}

	sql::ResultSet* Query(const string &query_str) {
		return stmt->executeQuery(query_str);
	}

	void Insert(const string &table, const string &id, const string &name) {
		stmt->execute("INSERT INTO " + table + " (id, name)"
				" VALUES(" + id + ", '" + name + "')");
	}
};

namespace ba = boost::asio;

class Server {
	using QueryPtr = std::shared_ptr<MysqlDbConnect>;

	QueryPtr m_query_ptr;
	int m_port;

public:
	Server(const QueryPtr &query, const int port)
		: m_query_ptr(query), m_port(port) {}

	void Start() {
		ba::io_service service;
		ba::ip::tcp::endpoint ep(ba::ip::tcp::v4(), m_port);
		ba::ip::tcp::acceptor acc(service, ep);
		while (true) {
			auto sock = ba::ip::tcp::socket(service);
			acc.accept(sock);
			std::thread(client_session, std::move(sock), std::ref(m_query_ptr))
				.detach();
		}
	}

private:
	static void client_session(ba::ip::tcp::socket sock, QueryPtr query_ptr) {
		try {
			string request;
			boost::asio::streambuf buf;
			read_until(sock, buf, '\n');
			std::istream(&buf) >> request;
			query_ptr->Insert("friday_a", "0", request);
			sql::ResultSet *res = query_ptr->Query("SELECT * from friday_a");
			while (res->next()) {
				std::cout << "id(" << res->getString("id")
					<< ") :" << res->getString("name") << std::endl;
			}
			delete res;
			ba::write(sock, ba::buffer("pong", 4));
		} catch (const std::exception &e) {
			std::cerr << e.what() << std::endl;
			throw;
		}
	}
};

int main(int argc, char *argv[]) {
	try {
		assert(argc == 2);
		const int port = [&]() {
			std::stringstream ss(argv[1]);
			int n;
			ss >> n;
			return n;
		}();
		// std::cout << "Listen: *:" << port << std::endl;

		auto query_ptr = std::make_shared<MysqlDbConnect>();
		Server server(query_ptr, port);
		server.Start();
	} catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
