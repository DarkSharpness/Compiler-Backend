#include "share_mem.h"
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

static void shutdown_system() {
	reboot(RB_POWER_OFF);
}

bool compare(std::string const &file1, std::string const &file2) {
	// 1. filesystem, size should be same
	// 2. content
	auto sz = std::filesystem::file_size(file1);
	if (sz != std::filesystem::file_size(file2))
		return false;
	std::ifstream f1(file1, std::ios::binary);
	std::ifstream f2(file2, std::ios::binary);
	std::array<std::byte, 1024> buffer1, buffer2;

	while (sz >= buffer1.size()) {
		f1.read(reinterpret_cast<char *>(buffer1.data()), buffer1.size());
		f2.read(reinterpret_cast<char *>(buffer2.data()), buffer2.size());
		if (buffer1 != buffer2)
			return false;
		sz -= buffer1.size();
	}

	if (sz > 0) {
		f1.read(reinterpret_cast<char *>(buffer1.data()), sz);
		f2.read(reinterpret_cast<char *>(buffer2.data()), sz);
		if (buffer1 != buffer2)
			return false;
	}
	return true;
}

void run(std::string program, std::string input_file, std::string output_file) {
	if (program.size() > 256) {
		std::cerr << "Program name too long: " << program << std::endl;
		shutdown_system();
	}
	constexpr auto tmp_out = "/tmp.out";
	std::cout << "Run: " << program << std::endl;
	run_data->cycle_start = 0;
	run_data->cycle_end = 0;
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

	// 使用 pidfd_open 获取子进程的文件描述符
	int pidfd = syscall(SYS_pidfd_open, pid, 0);
	if (pidfd == -1) {
		perror("pidfd_open failed");
		exit(EXIT_FAILURE);
	}

	// 使用 poll 等待子进程退出，设置超时时间为 60 秒
	pollfd pfd;
	pfd.fd = pidfd;
	pfd.events = POLLIN;

	auto poll_ret = poll(&pfd, 1, 1000 * 60);
	if (poll_ret == -1) {
		perror("poll failed");
		exit(EXIT_FAILURE);
	}
	if (poll_ret == 0) {
		kill(pid, SIGKILL);
		std::cout << "Killed: Timeout" << std::endl;
	}
	close(pidfd);

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
	std::cout << "Output: " << (compare(tmp_out, output_file) ? "AC" : "WA") << std::endl;
}

#include "testcase_list.h"

int main(int argc, char *argv[]) {
	if (getpid() == 1) {
		sleep(5);
		// wait for kernel fully output
	}
	if (auto shm = create_shm(); shm != 0) {
		std::cerr << "create_shm failed: " << shm << std::endl;
		perror("create_shm");
		shutdown_system();
	}
	for (auto const &testcase: testcases) {
		run(testcase[0], testcase[1], testcase[2]);
		sleep(1);
	}
	sleep(5);
	shutdown_system();
	sleep(60);
	return 0;
}
