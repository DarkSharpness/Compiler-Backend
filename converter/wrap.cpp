#include "share_mem.h"

extern "C" int _trampoline_to_main();

int main() {
	if (auto shm = create_shm(); shm != 0)
		return shm;
	return _trampoline_to_main();
}
