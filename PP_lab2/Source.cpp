#include <cstdlib>
#include <iostream>
#include <cstring>
#include <sstream>
#include <pthread.h>
#include <vector>
#include <chrono>
#include <random>
#include <locale>
using namespace std;

#define err_exit(code, str) { cerr << str << ": " << (code) << endl; exit(EXIT_FAILURE); }
const int TASKS_COUNT = 1000;
int task_list[TASKS_COUNT]; // Массив заданий
int current_task = 0; // Указатель на текущее задание
pthread_mutex_t mutex; // Мьютекс
void do_task(int task_no)
{
	int start_number = 123456789;
	for (int i = 0; i < 10000000; i++) {
		start_number = (start_number * start_number) % 10000000000;
	}
}
void* thread_job(void* arg)
{
	int thread_no = (int)arg;
	int task_no;
	int err;
	// Перебираем в цикле доступные задания
	while (true) {
		// Захватываем мьютекс для исключительного доступа
		// к указателю текущего задания (переменная
		// current_task)
		err = pthread_mutex_lock(&mutex);
		if (err != 0)
			err_exit(err, "Cannot lock mutex");
		// Запоминаем номер текущего задания, которое будем исполнять
		task_no = current_task;
		// Сдвигаем указатель текущего задания на следующее
		current_task++;
		// Освобождаем мьютекс
		err = pthread_mutex_unlock(&mutex);
		if (err != 0)
			err_exit(err, "Cannot unlock mutex");
		// Если запомненный номер задания не превышает
		// количества заданий, вызываем функцию, которая
		// выполнит задание.
		// В противном случае завершаем работу потока
		if (task_no < TASKS_COUNT)
			do_task(task_no);
		else
			return NULL;
	}
}
int main()
{
	int err; // Код ошибки
	// Инициализируем массив заданий случайными числами
	// Создаём потоки
	current_task=0;
	for (int i = 0; i < TASKS_COUNT; i++)
		task_list[i] = rand() % TASKS_COUNT;
	// Инициализируем мьютекс

	err = pthread_mutex_init(&mutex, NULL);
	if (err != 0)
		err_exit(err, "Cannot initialize mutex");


	// Инициализируем переменные потоков и параметров
	int threads_number = 32;
	cout << "threads_number: " << threads_number << endl;
	pthread_t* threads = new pthread_t[threads_number];
	for (size_t n = 0; n < threads_number; n++) {
		// Создание потока
		err = pthread_create(&threads[n], NULL, thread_job, (int*)n+1);
		// Если при создании потока произошла ошибка, выводим
		// сообщение об ошибке и прекращаем работу программы
		if (err != 0) {
			cout << "Не получилось создать поток: " << err << endl;
			exit(-1);
		}
	}

	auto start = std::chrono::steady_clock::now();

	// Ожидаем завершения потоков
	for (size_t n = 0; n < threads_number; n++) {
		pthread_join(threads[n], NULL);
	}

	// Освобождаем ресурсы, связанные с мьютексом
	pthread_mutex_destroy(&mutex);
	std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
	cout << "Time spent: " << elapsed.count() << " seconds." << endl;
}