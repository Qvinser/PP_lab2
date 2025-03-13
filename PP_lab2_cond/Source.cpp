#include <cstdlib>
#include <cstring>
#include <iostream>
#include <pthread.h> 

#define PROFUCT_COUNT 50

using namespace std;
#define err_exit(code, str) { cerr << str << ": " << (code) \
 << endl; exit(EXIT_FAILURE); }
enum store_state { EMPTY, FULL };
store_state state = EMPTY;
pthread_mutex_t mutex;
pthread_cond_t cond;
int store;
int current_product = 0;
void* producer(void* arg)
{
	int err;
	//
	for (; current_product < PROFUCT_COUNT;) {
		// Захватываем мьютекс и ожидаем освобождения склада
		err = pthread_mutex_lock(&mutex);
		if (err != 0)
			err_exit(err, "Cannot lock mutex");
		while (state == FULL) {
			err = pthread_cond_wait(&cond, &mutex);
			if (err != 0)
				err_exit(err, "Cannot wait on condition variable");
		}
		// Получен сигнал, что на складе не осталось товаров.
		// Производим новый товар.
		store = rand();
		current_product++;
		state = FULL;
		// Посылаем сигнал, что на складе появился товар.
		err = pthread_cond_signal(&cond);
		if (err != 0)
			err_exit(err, "Cannot send signal");
		err = pthread_mutex_unlock(&mutex);
		if (err != 0)
			err_exit(err, "Cannot unlock mutex");
	}
}
void* consumer(void* arg)
{
	int err;
	//
	for (; current_product < PROFUCT_COUNT;) {
		// Захватываем мьютекс и ожидаем появления товаров на складе
		err = pthread_mutex_lock(&mutex);
		if (err != 0)
			err_exit(err, "Cannot lock mutex");
		while (state == EMPTY) {
			err = pthread_cond_wait(&cond, &mutex);
			if (err != 0)
				err_exit(err, "Cannot wait on condition variable");
		}
		// Получен сигнал, что на складе имеется товар.
		// Потребляем его.
		cout << "Consuming number " << store << "...";
		//Sleep(1);
		cout << "done" << endl;
		state = EMPTY;
		// Посылаем сигнал, что на складе не осталось товаров.
		err = pthread_cond_signal(&cond);
		if (err != 0)
			err_exit(err, "Cannot send signal");
		err = pthread_mutex_unlock(&mutex);
		if (err != 0)
			err_exit(err, "Cannot unlock mutex");
	}
}
int main()
{
	int err;
	//
	pthread_t thread1, thread2; // Идентификаторы потоков
	// Инициализируем мьютекс и условную переменную
	err = pthread_cond_init(&cond, NULL);
	if (err != 0)
		err_exit(err, "Cannot initialize condition variable");
	err = pthread_mutex_init(&mutex, NULL);
	if (err != 0)
		err_exit(err, "Cannot initialize mutex");
	// Создаём потоки
	err = pthread_create(&thread1, NULL, producer, NULL);
	if (err != 0)
		err_exit(err, "Cannot create thread 1");
	err = pthread_create(&thread2, NULL, consumer, NULL);
	if (err != 0)
		err_exit(err, "Cannot create thread 2");
	// Дожидаемся завершения потоков
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	// Освобождаем ресурсы, связанные с мьютексом
	// и условной переменной
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}