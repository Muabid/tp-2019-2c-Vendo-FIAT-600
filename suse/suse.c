#include "net.h"

int main() {
	init_server(10500);
	close(10500);
	return 0;
}
