#include <iostream>
#include <cassert>
#include <sstream>

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

	} catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
