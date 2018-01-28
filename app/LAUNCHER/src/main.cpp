#include "launcher.h"

int main(int argc, char **argv) {
	auto updater = new TI::Launcher();
	auto res = updater->show(argc, argv);
	updater->run();
	return res;
}
