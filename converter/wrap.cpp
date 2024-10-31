#include "share_mem.h"

extern "C" int _trampoline_to_main(volatile shm_data *run_data);

int main() {
	if (auto shm = create_shm(); shm != 0)
		return shm;
	return _trampoline_to_main(run_data);
}
