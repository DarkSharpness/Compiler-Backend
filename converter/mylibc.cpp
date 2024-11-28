
#include <cstddef>
constexpr auto MALLOC_SIZE = 1024 * 1024 * 256;
unsigned char malloc_buf[MALLOC_SIZE];
unsigned char *malloc_ptr = malloc_buf;

extern "C" void *_my_malloc(size_t size) {
	if (malloc_ptr + size > malloc_buf + MALLOC_SIZE) {
		return nullptr;
	}
	unsigned char *ret = malloc_ptr;
	malloc_ptr += (size + 15) & ~15;
	return ret;
}