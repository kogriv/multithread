﻿#include <iostream>
#include <thread>
#include <atomic>
#include <cstring>

// Рассмотрим сит-ю с 2-мя глоб переменными b и c

// Пример (гарантированное попадание в одно машинное слово)

// 1. c и b объявлены подряд в struct, значит они будут находиться в одном блоке памяти.
// Нет alignas или #pragma pack, которые могли бы разнести c и b в разные слова.
// Компилятор скорее всего положит их в одно машинное слово (особенно если word size == 8, а мы занимаем всего 2 байта).
// 
//struct SharedData {
//    volatile char c;  // 1 байт
//    volatile char b;  // 1 байт (лежит в том же машинном слове)
//} data;  // Гарантированно размещается как единый объект в памяти

// 2. Принудительное упаковка (#pragma pack)
// Иногда компилятор добавляет пустые байты (padding) для выравнивания. 
// Можно запретить ему это делать, явно указав упаковку:
// ❗ Важно: #pragma pack(1) может ухудшить производительность на некоторых платформах.
#pragma pack(1) // Отключаем добавление пустых байтов
struct SharedData {
    volatile char c;
    volatile char b;
};
#pragma pack() // Возвращаем настройки по умолчанию

SharedData data;

// 3. Использование массива вместо двух отдельных переменных
// Компилятор всегда кладет элементы массива подряд, что можно использовать:
//volatile char shared[2] = { 0, 0 }; // Два байта подряд
//
//void f() {
//    shared[0] = 1;
//    int x = shared[0];
//}
//
//void g() {
//    shared[1] = 1;
//    int y = shared[1];
//}
// Теперь shared[0] и shared[1] гарантированно в одном машинном слове, 
// потому что это смежные элементы массива.

// Как "увидеть" машинное слово в памяти?
// Можно создать структуру с разными полями и посмотреть смещение каждого поля в памяти.
// 1: Использование offsetof()
struct Test {
    char k;   // 1 байт
    int l;    // 4 байта
    char m;   // 1 байт
};

// 2: Чтение памяти через memcpy()
struct TestM {
    char p = 0x11;
    int q = 0x22334455;
    char r = 0x66;
};


void f() {
    data.c = 1;
    int x = data.c;
    std::cout << "Thread 1: x = " << x << std::endl;
}

void g() {
    data.b = 1;
    int y = data.b;
    std::cout << "Thread 2: y = " << y << std::endl;
}

int data_race() {
    // Тип size_t обычно совпадает с размером машинного слова:
    std::cout << "Machine word size: " << sizeof(size_t) << " bytes" << std::endl;
    // На 64-битной системе вы получите 8 bytes, а на 32-битной — 4 bytes.

    // Использование sizeof(void*) (размер указателя)
	std::cout << "Pointer size: " << sizeof(void*) << " bytes" << std::endl;

    // 3: Использование std::atomic<intmax_t>::is_always_lock_free
    // Этот метод проверяет, с каким максимальным размером данных 
    // процессор может работать без блокировок (atomic operations)
    std::cout << "Atomic max lock-free size: " << sizeof(std::atomic<intmax_t>) << " bytes" << std::endl;

    // "увидеть" машинное слово в памяти
    std::cout << "Offset of a: " << offsetof(Test, k) << " bytes" << std::endl;
    std::cout << "Offset of b: " << offsetof(Test, l) << " bytes" << std::endl;
    std::cout << "Offset of c: " << offsetof(Test, m) << " bytes" << std::endl;
    std::cout << "Total struct size: " << sizeof(Test) << " bytes" << std::endl;
    // Если sizeof(Test) == 8, значит, компилятор выровнял b на границу машинного слова.
    // Если sizeof(Test) == 12, значит, компилятор добавил выравнивание (padding).

    // Чтение памяти через memcpy()
    /*struct TestM {
        char a = 0x11;
        int b = 0x22334455;
        char c = 0x66;
    };*/
    TestM t;
    unsigned char raw_data[sizeof(t)];
    std::memcpy(raw_data, &t, sizeof(t));

    std::cout << "Memory layout:" << std::endl;
    for (size_t i = 0; i < sizeof(t); ++i) {
        std::cout << std::hex << (int)raw_data[i] << " ";
    }
    std::cout << std::endl;
    // Этот код покажет, как компилятор разложил структуру в памяти!
    // получим
    /*Memory layout :
    11 cc cc cc 55 44 33 22 66 cc cc cc
    
    — это байтовое представление структуры Test в памяти.
    Разберем по байтам :
    11 — это char a = 0x11;
    cc cc cc — это паддинг(добавочные байты для выравнивания int).
    55 44 33 22 — это int b = 0x22334455; (байты идут в порядке little - endian, младшие байты 55 записаны первыми).
    66 — это char c = 0x66;
    cc cc cc — это паддинг после char c, чтобы sizeof(Test) был кратен sizeof(int) (обычно 4 или 8).

    Почему есть cc cc cc ?

    Выравнивание(alignment)

    int требует выравнивания по 4 байтам.
    После char a(1 байт) добавляются 3 паддинг - байта, чтобы b начинался с кратного 4 адреса.
    После char c тоже могут быть паддинг - байты, чтобы структура занимала целое слово памяти.

    Почему cc ?
    Это зависит от компилятора, но cc(или 0xCD в MSVC) — это специальное значение, которым компиляторы часто заполняют неинициализированную память.*/



    std::thread t1(f);
    std::thread t2(g);

    t1.join();
    t2.join();

    return 0;
}

// ожидаем x=1 y=1. А если компоновщик расположит c b в одном и том же слове памяти 
// и комп не может загрузить и хранить что-либо в объеме памяти, меньшим слова:
// word: с********b
// поток 1 мог бы прочесть слово содержащее b и c, исзменить c  и записать обратно.
// поток 2 мог бы сделать то же самое с b. вопрос: какой поток прочтет слово первым
// и какой поток будет писать последним.
// можно в результе получить 10, 01, 11 (но не 00)

// ----------------------------
// по факту, в большинстве случаев, такой код не вызовет гонку данных.
// при запуске всегда получаю 
// Thread 2: y = 1
// Thread 1 : x = 1
//Ты ожидаешь увидеть эффект гонки данных из - за того, 
// что c и b могут быть размещены в одном машинном слове.
// 
// Однако, почему этого не происходит :
//
//Оптимизации компилятора
//Современные компиляторы довольно агрессивно оптимизируют код.
// Они могут понимать, что c и b меняются в отдельных потоках, 
// и переставлять инструкции, применять кеширование, разыменовывание 
// и другие техники, которые могут предотвращать эффект гонки.
//
//Кеширование процессора
//Если переменные c и b находятся в одном кеше - пакете(cache line),
// то процессор может вести себя так, что изменения одной переменной 
// не "затирают" другую в большинстве случаев.
//
//Порядок выполнения потоков
//Хотя твой код потенциально может проявлять гонку данных, 
// в реальности std::thread создаёт потоки, которые могут 
// запускаться так, что один поток всегда успевает записать 
// своё значение до того, как второй прочтёт изменённое слово.
//
//Модель памяти процессора
//Некоторые процессоры могут обеспечивать атомарность операций 
// даже без использования std::atomic, если они работают 
// с операциями чтения / записи в границах машинного слова.