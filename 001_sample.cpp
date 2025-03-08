#include <iostream>
#include <thread>
#include <windows.h>

namespace {
	void f() {
		std::cout << "Hello" << std::endl;
	}

	struct F {
		void operator()() {
			std::cout << "Parallel Word!" << std::endl;
		}
	};

	void parallel() {
		F p;
		std::thread t1(f);  // Внутри namespace не требуется указывать префикс
		std::thread t2(p);
		t1.join();
		t2.join();
	}

	void show_thread_info() {
		// Получить количество аппаратных потоков, которые могут выполняться параллельно
		unsigned int hardware_threads = std::thread::hardware_concurrency();
		std::cout << "Доступно аппаратных потоков: " << hardware_threads << std::endl;
	}


		
}

int sample_001() {
	SetConsoleOutputCP(CP_UTF8);
    
	
    // Выводим через cout
    show_thread_info();
	return 0;
}