
#include "updater.h"






int main(int argc, char **argv) {
	auto updater = new TI::Updater();
	return updater->show(argc, argv);
}
