#include <iostream>
#include <cassert>
#include <sstream>

#include <thread>
#include <boost/asio.hpp>

#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace ba = boost::asio;
void client_session(ba::ip::tcp::socket sock, sql::Statement *stmt) {
	try {
		char data[4];
		size_t len = sock.read_some(ba::buffer(data));
		std::cout << "receive " << len << "=" << std::string{data, len} << std::endl;
		stmt->execute("INSERT INTO friday_a (id, name) VALUES(0, '" + std::string{data, len} + "')");

		sql::ResultSet *res = stmt->executeQuery("SELECT * from friday_a");
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

int main(int argc, char *argv[]) {
	try {
		assert(argc == 2);
		const int port = [&]() {
			std::stringstream ss(argv[1]);
			int n;
			ss >> n;
			return n;
		}();

		std::cout << "Listen: *:" << port << std::endl;

		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;

		driver = get_driver_instance();
		con = driver->connect("tcp://127.0.0.1:3306", "root", "");
		con->setSchema("mysql");

		stmt = con->createStatement();
		stmt->execute("DROP TABLE IF EXISTS friday_a");
		stmt->execute("CREATE TABLE friday_a(id INT, name char(255))");
		stmt->execute("DROP TABLE IF EXISTS friday_b");
		stmt->execute("CREATE TABLE friday_b(id INT, name char(255))");

		ba::io_service service;
		ba::ip::tcp::endpoint ep(ba::ip::tcp::v4(), port);
		ba::ip::tcp::acceptor acc(service, ep);
		while (true) {
			auto sock = ba::ip::tcp::socket(service);
			acc.accept(sock);
			std::thread(client_session, std::move(sock), std::ref(stmt)).detach();
		}

		delete stmt;
		delete con;
	} catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
