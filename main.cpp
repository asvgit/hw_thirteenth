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
using StringVec = std::vector<string>;

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

	string DoCmd(const StringVec &request) {
		if (request.empty())
			return "ERR empty request";
		if (request.front() == "INSERT")
			return Insert(request);
		if (request.front() == "TRUNCATE")
			return Truncate(request);
		if (request.front() == "INTERSECTION")
			return Intersection();
		if (request.front() == "SYMMETRIC_DIFFERENCE")
			return Difference();
		return "ERR unknown command '" + request.front() + "'";
	}

private:
	sql::ResultSet* Query(const string &query_str) {
		return stmt->executeQuery(query_str);
	}

	string Insert(const StringVec &request) {
		if (request.size() < 4)
			return "ERR not enough arguments for insertion";
		const string &table = request[1];
		if (table != "A" && table != "B")
			return "ERR unknown table '" + table + "'";

		const string &id = request[2];
		const string real_table = table == "A" ? "friday_a" : "friday_b";
		const string query_str = "SELECT *"
				" FROM " + real_table +
				" WHERE id = " + id;
		if (stmt->executeQuery(query_str)->next()) {
			return "ERR duplicate '" + id + "'";
		}

		stmt->execute("INSERT INTO " + real_table + " (id, name)"
				" VALUES(" + id + ", '" + request[3] + "')");
		return "OK";
	}

	string Truncate(const StringVec &request) {
		if (request.size() < 2)
			return "ERR not enough arguments for truncation";
		const string &table = request[1];
		if (table != "A" && table != "B")
			return "ERR unknown table '" + table + "'";
		const string real_table = table == "A" ? "friday_a" : "friday_b";
		stmt->execute("DELETE FROM " + real_table);
		return "OK";
	}

	string Intersection() {
		string result;
		const string query_str = "SELECT"
				" a.id, a.name AS a_name, b.name AS b_name"
			" FROM friday_a AS a"
			" INNER JOIN friday_b AS b ON b.id = a.id";
		auto res = stmt->executeQuery(query_str);
		while (res->next()) {
			result += res->getString("id")
				+ "," + res->getString("a_name")
				+ "," + res->getString("b_name")
				+ "\n";
		}
		delete res;
		return result + "OK";
	}

	string Difference() {
		string result;
		const string query_str = "SELECT"
				" a.id, a.name AS a_name, b.name AS b_name"
			" FROM friday_a AS a"
			" LEFT OUTER JOIN friday_b AS b ON b.id = a.id"
			" WHERE a.name IS NULL OR b.name IS NULL"
			" UNION"
			" SELECT b.id, a.name AS a_name, b.name AS b_name"
			" FROM friday_a AS a"
			" RIGHT OUTER JOIN friday_b AS b ON b.id = a.id"
			" WHERE a.name IS NULL OR b.name IS NULL";
		auto res = stmt->executeQuery(query_str);
		while (res->next()) {
			result += res->getString("id")
				+ "," + res->getString("a_name")
				+ "," + res->getString("b_name")
				+ "\n";
		}
		delete res;
		return result + "OK";
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
			std::vector<string> request;
			boost::asio::streambuf buf;
			auto len = read_until(sock, buf, '\n');
			string l;
			do {
				l.clear();
				std::istream(&buf) >> l;
				if (!l.empty())
					request.push_back(l);
			} while (!l.empty());
			const string response = query_ptr->DoCmd(request);
			ba::write(sock, ba::buffer(response + "\n", response.size() + 1));
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
		auto query_ptr = std::make_shared<MysqlDbConnect>();
		Server server(query_ptr, port);
		server.Start();
	} catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
