#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

using namespace std;

condition_variable cond1;
mutex mtx;
int ready = 0;

// Функция-поставщик
void producer() {
    for (int i = 0; i < 5; i++) {
        this_thread::sleep_for(chrono::seconds(1));
        
        lock_guard <mutex> locker(mtx); // Блокировка мютекса

        cout << "Поставщик: событие " << i << " отправлено" << endl;
        ready = 1; //несериализуемые данные условное событие
        
        cond1.notify_one();

    } // На выходе автоматически разблокируется
}

// Функция-потребитель
void consumer() {
    for (int i = 0; i < 5; i++) {

        unique_lock <mutex> locker(mtx); //Блокировка мьютекса
        cond1.wait(locker, []{ return ready == 1; });//Проверка наступления события. Ожидание
        
        cout << "Потребитель: событие " << i << " обработано" << endl;
        ready = 0;
    } // На выходе срабатывает деструктор, который вызывает mtx.unlock()
}

int main() {
    setlocale(LC_ALL, "RU");

    thread producer_thread(producer);
    thread consumer_thread(consumer);
    
    producer_thread.join();
    consumer_thread.join();
    
    return 0;
}