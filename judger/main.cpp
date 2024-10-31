#include "share_mem.h"
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <format>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

static void shutdown_system() {
	reboot(RB_POWER_OFF);
}


void run(std::string program, std::string input_file, std::string output_file) {
	if (program.size() > 256) {
		std::cerr << "Program name too long: " << program << std::endl;
		shutdown_system();
	}
	constexpr auto tmp_out = "/tmp.out";
	auto pid = fork();
	if (pid == 0) {
		freopen(input_file.c_str(), "rb", stdin);
		freopen(tmp_out, "wb", stdout);
		fclose(stderr);
		char *r_argv[] = {(char *) program.c_str(), nullptr};
		int ret = execve(program.c_str(), r_argv, nullptr);
		std::cout << "\033[31m" << std::format("Error ! execve not succeed, code = {}", ret) << std::endl;
		perror("execve");
		exit(errno);
	}
	rusage usage;
	int status = 0;
	wait4(pid, &status, 0, &usage);


	std::cout << std::format("Run Done:\n");
	// === return code ===
	if (WIFEXITED(status)) {
		std::cout << std::format("Return code: {}\n", WEXITSTATUS(status));
	}
	else if (WIFSIGNALED(status)) {
		std::cout << std::format("Signal: {}\n", WTERMSIG(status));
	}
	else {
		std::cout << std::format("Unknown status: {}\n", status);
	}
	// === time ===
	std::cout << "Time:\n";
	std::cout << std::format("\tcycle: {} {} {}\n", run_data->cycle_end - run_data->cycle_start, run_data->cycle_start, run_data->cycle_end);
	std::cout << std::format("\tuser: {}s{}ms\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
	std::cout << std::format("\tsys: {}s{}ms\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
	// === memory ===
	std::cout << "Memory:\n";
	std::cout << std::format("\tmaxrss: {}KB\n", usage.ru_maxrss);
	std::cout << std::format("\tixrss: {}KB\n", usage.ru_ixrss);
	std::cout << std::format("\tidrss: {}KB\n", usage.ru_idrss);
	std::cout << std::format("\tisrss: {}KB\n", usage.ru_isrss);
	std::cout << std::format("\tminflt: {}\n", usage.ru_minflt);
	std::cout << std::format("\tmajflt: {}\n", usage.ru_majflt);
	std::cout << std::format("\tnswap: {}\n", usage.ru_nswap);
	std::cout << std::format("\tinblock: {}\n", usage.ru_inblock);
	std::cout << std::format("\toublock: {}\n", usage.ru_oublock);
	std::cout << std::format("\tmsgsnd: {}\n", usage.ru_msgsnd);
	std::cout << std::format("\tmsgrcv: {}\n", usage.ru_msgrcv);
	std::cout << std::format("\tnsignals: {}\n", usage.ru_nsignals);
	std::cout << std::format("\tnvcsw: {}\n", usage.ru_nvcsw);
	std::cout << std::format("\tnivcsw: {}\n", usage.ru_nivcsw);
	// === output ===
	std::cout << "Output:\n";

	char buffer[1024];
	FILE *f = fopen(tmp_out, "rb");
	while (auto size = fread(buffer, 1, sizeof(buffer), f))
		std::cout << std::string_view(buffer, size);
	fclose(f);
}


int main(int argc, char *argv[]) {
	if (auto shm = create_shm(); shm != 0) {
		std::cerr << "create_shm failed: " << shm << std::endl;
		perror("create_shm");
		shutdown_system();
	}
	run("/usr/bin/test.elf", "/var/1.in", "/var/1.out");
	shutdown_system();
	return 0;
}
