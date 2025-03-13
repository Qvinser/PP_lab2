#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <vector>
#include <chrono>
#include <random>
#include <locale>
using namespace std;

struct thread_params {
	void (*reduce)(void* arg, int from, int to); // функци-обработчик
	void* item;			// объект, который нужно обработать
	int thread_number;	// порядковый номер потока
	int from;			// индекс в массиве, с которого поток начинает обработку
	int to; 			// индекс в массиве, в котором поток заканчивает обработку
	bool show_params;	// флаг - нужно ли отображать переданные параметры
};


template<typename T>
void increment(void* arg, int from, int to)
{
	std::vector<T>* vec = (vector<T>*) arg;
	for (size_t i = from; i < to && i < vec->size(); i += 1)
	{
		(*vec)[i] += 1;
		(*vec)[i] -= 1;
		(*vec)[i] *= 3;
		(*vec)[i] *= (*vec)[i];
	}
}


/* Функция, которую будет исполнять созданный поток */
void* thread_job(void* arg)
{
	int err;
	thread_params* params = (thread_params*)arg;
	if (params->show_params) {
		//int detachstate;
		//size_t stacksize;
		//void *stackaddr;
		//pthread_attr_t self_attr;
		//if ((err = pthread_attr_init(&self_attr)) != 0 ||
		//	(err = pthread_attr_getdetachstate(&self_attr, &detachstate)) != 0 ||
		//	(err = pthread_attr_getstackaddr(&self_attr, &stackaddr)) != 0 ||
		//	(err = pthread_attr_getstacksize(&self_attr, &stacksize)) != 0)
		//{
		//	cout << "Не удалось прочесть атрибуты потока: " << strerror(err) << endl;
		//	exit(-1);
		//}
		//cout << "Thread#" << params->thread_number << " has attributes: " << endl <<
		//	"detachstate:	" << detachstate << endl <<
		//	"stackaddr:	" << stackaddr << endl <<
		//	"stacksize:	" << stacksize << endl << endl;
		cout << "Thread#" << params->thread_number << " has parameters: " << endl <<
			"reduce pointer:	" << (int)params->reduce << endl <<
			"from:	" << (int)params->from << endl <<
			"to:	" << (int)params->to << endl <<
			"item pointer:	" << (int)params->item << endl << endl;
		//pthread_attr_destroy(&self_attr);
	}
	params->reduce(params->item, params->from, params->to);
	return NULL;
}
int main()
{
	// Определяем переменные
	setlocale(LC_ALL, "Russian");
	int err;
	int threads_number, tmp_show_params;
	bool show_params;
	// Узнаем у пользователя, в каком виде и количестве должны быть созданы потоки
	cout << "Введите количество потоков: ";
	cin >> threads_number;
	cout << endl << "Должны ли потоки выводить информацию о своих параметрах (0/1): ";
	cin >> tmp_show_params;
	show_params = tmp_show_params != 0;
	int detachstate;
	size_t stacksize;
	void* stackaddr;
	cout << "Введите аттрибут detachstate (0 - по умолчанию): ";
	cin >> detachstate;
	cout << "Введите аттрибут stackaddr (0 - по умолчанию): ";
	cin >> stacksize;
	cout << "Введите аттрибут stacksize (0 - по умолчанию): ";
	cin >> stackaddr;
	// Инициализируем переменные потоков и параметров
	pthread_t* threads = new pthread_t[threads_number];
	thread_params* params = new thread_params[threads_number];
	pthread_attr_t threads_attr;
	if ((err = pthread_attr_init(&threads_attr)) != 0) {
		cout << "Не получилось инициализировать аттрибуты: " << strerror(err) << endl;
		exit(-1);
	}
	if (stacksize != 0) {
		err = pthread_attr_setstacksize(&threads_attr, stacksize);
		if (err != 0) {
			cout << "Setting stack size attribute failed: " << strerror(err)
				<< endl;
			//exit(-1);
		}
	}
	if (stackaddr != 0) {
		err = pthread_attr_setstackaddr(&threads_attr, stackaddr);
		if (err != 0) {
			cout << "Setting stack addr attribute failed: " << strerror(err)
				<< endl;
			//exit(-1);
		}
	}
	if (detachstate != 0) {
		err = pthread_attr_setdetachstate(&threads_attr, detachstate);
		if (err != 0) {
			cout << "Setting detachstate attribute failed: " << strerror(err)
				<< endl;
			//exit(-1);
		}
	}

	// Размер вектора-объекта, над которым будут проводиться работы
	size_t vector_size = 1000000;
	vector<int> vector_item(vector_size);
	// Инициализация вектора
	for (size_t n = 0; n < vector_size; n++) {
		vector_item[n] = rand() % vector_size;
	}

	auto start = std::chrono::steady_clock::now();
	for (size_t n = 0; n < threads_number; n++) {
		// Инициализация параметров
		params[n].show_params = show_params;
		params[n].reduce = increment<int>;
		params[n].item = &vector_item;
		params[n].from = vector_size / threads_number * n;
		params[n].to = vector_size / threads_number * (n + 1);
		params[n].thread_number = n;
		// Создание потока
		err = pthread_create(&threads[n], &threads_attr, thread_job, &params[n]);
		// Если при создании потока произошла ошибка, выводим
		// сообщение об ошибке и прекращаем работу программы
		if (err != 0) {
			cout << "Не получилось создать поток: " << strerror(err) << endl;
			exit(-1);
		}
	}
	pthread_attr_destroy(&threads_attr);

	// Засекаем время, за которое будут созданы потоки
	auto end1 = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed1 = end1 - start;
	cout << "Все потоки были запущены за " << elapsed1.count() << " seconds." << endl;

	// Ожидаем завершения потоков
	for (size_t n = 0; n < threads_number; n++) {
		pthread_join(threads[n], NULL);
	}

	// Засекаем время, за работа будет выполнена с использованием мультипоточности
	auto end2 = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed2 = end2 - start;
	cout << "Все потоки завершили свою работу за " << elapsed2.count() << " seconds." << endl;
}