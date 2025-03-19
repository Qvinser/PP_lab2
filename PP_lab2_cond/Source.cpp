#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <chrono>
#include <map>
#include <windows.h>
#include <pthread.h> 

#define PRODUCT_COUNT 50000

using namespace std;
#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
 << endl; exit(EXIT_FAILURE); }
enum store_state { EMPTY, FULL };
store_state state = EMPTY;
pthread_mutex_t conv_mutex = PTHREAD_MUTEX_INITIALIZER;
enum cond_state { LOCK, UNLOCK };
cond_state cond = UNLOCK;
int store;
int current_product = 0;
vector<int> products_list(PRODUCT_COUNT); // ������ ���������
map<int, int> product_convolution;

void* producer(void* arg)
{
	int err;
	for (; current_product < PRODUCT_COUNT;) {
		// ����������� ������� � ������� ������������ ������
		while (cond == LOCK || state == FULL);// Sleep(1);
		cond = LOCK;
		// ������� ������, ��� �� ������ �� �������� �������.
		// ���������� ����� �����.
		store = rand() % PRODUCT_COUNT;
		current_product++;
		state = FULL;
		// �������� ������, ��� �� ������ �������� �����.
		cond = UNLOCK;
	}
	return NULL;
}
void* consumer(void* arg)
{
	int err;
	//
	for (; current_product < PRODUCT_COUNT;) {
		// ����������� ������� � ������� ��������� ������� �� ������
		while (cond == LOCK || state == EMPTY);// Sleep(1);
		cond = LOCK;
		// ������� ������, ��� �� ������ ������� �����.
		// ���������� ���.
		//cout << "Consuming number " << store << "...";
		products_list[current_product - 1] = store;
		//cout << "done" << endl;
		state = EMPTY;
		// �������� ������, ��� �� ������ �� �������� �������.
		cond = UNLOCK;
	}
	return NULL;
}


struct thread_map_params {
	void (*map)(void* arg, int from, int to); // �������-����������
	void* item;			// ������, ������� ����� ����������
	int thread_number;	// ���������� ����� ������
	int from;			// ������ � �������, � �������� ����� �������� ���������
	int to; 			// ������ � �������, � ������� ����� ����������� ���������
};
struct thread_reduce_params {
	void (*reduce)(void* arg, int from, int to); // �������-����������
	void* item;			// ������, ������� ����� ����������
	int thread_number;	// ���������� ����� ������
	int from;			// ������ � �������, � �������� ����� �������� ���������
	int to; 			// ������ � �������, � ������� ����� ����������� ���������
};
void* map_job(void* arg) {
	int err;
	thread_map_params* params = (thread_map_params*)arg;
	//cout << "Thread#" << params->thread_number << " has parameters: " << endl <<
	//	"map pointer:	" << (int)params->map << endl <<
	//	"from:	" << (int)params->from << endl <<
	//	"to:	" << (int)params->to << endl <<
	//	"item pointer:	" << (int)params->item << endl << endl;
	params->map(params->item, params->from, params->to);
	return NULL;
}
void* reduce_job(void* arg) {
	int err;
	thread_reduce_params* params = (thread_reduce_params*)arg;
	//cout << "Thread#" << params->thread_number << " has parameters: " << endl <<
	//	"reduce pointer:	" << (int)params->reduce << endl <<
	//	"from:	" << (int)params->from << endl <<
	//	"to:	" << (int)params->to << endl <<
	//	"item pointer:	" << (int)params->item << endl << endl;
	params->reduce(params->item, params->from, params->to);
	return NULL;
}

template<typename T>
void convolute(void* arg, int from, int to) {
	int err;
	std::vector<T>* vec = (vector<T>*) arg;
	for (size_t i = from; i < to && i < vec->size(); i += 1)
	{
		int elem = (*vec)[i];
		if (product_convolution.count(elem)) {
			product_convolution[elem] += 1;
		}
		else {
			err = pthread_mutex_lock(&conv_mutex);
			if (err != 0)
				err_exit(err, "Cannot lock mutex"); 

			product_convolution[elem] = 1;

			err = pthread_mutex_unlock(&conv_mutex);
			if (err != 0)
			err_exit(err, "Cannot unlock mutex");
		}

	}
}
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
void MyMapReduce(void* inpt, int threads_number) {
	int err;
	vector<int>* data = (vector<int>*)inpt;
	pthread_t* threads = new pthread_t[threads_number];
	thread_map_params* map_params = new thread_map_params[threads_number];
	thread_reduce_params* reduce_params = new thread_reduce_params[threads_number];
	auto start_map = std::chrono::steady_clock::now();
	for (size_t n = 0; n < threads_number; n++) {
		// ������������� ����������
		map_params[n].map = increment<int>;
		map_params[n].item = data;
		map_params[n].from = (double)PRODUCT_COUNT / threads_number * n;
		map_params[n].to = (double)PRODUCT_COUNT / threads_number * (n + 1);
		map_params[n].thread_number = n;
		// �������� ������
		err = pthread_create(&threads[n], NULL, map_job, &map_params[n]);
		// ���� ��� �������� ������ ��������� ������, �������
		// ��������� �� ������ � ���������� ������ ���������
		if (err != 0) {
			cout << "�� ���������� ������� �����: " << err << endl;
			exit(-1);
		}
	}
	// ������� ���������� �������
	for (size_t n = 0; n < threads_number; n++) {
		pthread_join(threads[n], NULL);
	}

	std::chrono::duration<double> elapsed_map = std::chrono::steady_clock::now() - start_map;
	cout << "Time spent on map: " << elapsed_map.count() << " seconds." << endl;
	auto start_reduce = std::chrono::steady_clock::now();
	for (size_t n = 0; n < threads_number; n++) {
		// ������������� ����������
		reduce_params[n].reduce = convolute<int>;
		reduce_params[n].item = data;
		reduce_params[n].from = (double)PRODUCT_COUNT / threads_number * n;
		reduce_params[n].to = (double)PRODUCT_COUNT / threads_number * (n + 1);
		reduce_params[n].thread_number = n;
		// �������� ������
		err = pthread_create(&threads[n], NULL, reduce_job, &reduce_params[n]);
		// ���� ��� �������� ������ ��������� ������, �������
		// ��������� �� ������ � ���������� ������ ���������
		if (err != 0) {
			cout << "�� ���������� ������� �����: " << err << endl;
			exit(-1);
		}
	}
	// ������� ���������� �������
	for (size_t n = 0; n < threads_number; n++) {
		pthread_join(threads[n], NULL);
	}
	std::chrono::duration<double> elapsed_reduce = std::chrono::steady_clock::now() - start_reduce;
	cout << "Time spent on reduce: " << elapsed_reduce.count() << " seconds." << endl;
}

int main()
{
	int err;
	setlocale(LC_ALL, "Russian");
	pthread_t producer_thread, consumer_thread; // �������������� �������
	// �������������� ������� � �������� ����������
	pthread_mutexattr_t conv_attr;
	pthread_mutexattr_init(&conv_attr);
	pthread_mutexattr_settype(&conv_attr, PTHREAD_MUTEX_NORMAL);
	err = pthread_mutex_init(&conv_mutex, &conv_attr);
	pthread_mutexattr_destroy(&conv_attr);
	// ������ ������
	err = pthread_create(&producer_thread, NULL, producer, NULL);
	if (err != 0)
		err_exit(err, "Cannot create thread 1");
	err = pthread_create(&consumer_thread, NULL, consumer, NULL);
	if (err != 0)
		err_exit(err, "Cannot create thread 2");
	// ���������� ���������� �������
	pthread_join(producer_thread, NULL);
	pthread_join(consumer_thread, NULL);
	// ����������� �������, ��������� � ���������
	// � �������� ����������

	int threads_number = 32;
	cout << "������: " << endl;
	MyMapReduce(&products_list, threads_number);

	//for (auto elem = product_convolution.begin();
	//	elem != product_convolution.end(); ++elem)
	//	std::cout << (*elem).first << ": " << (*elem).second << std::endl;

	pthread_mutex_destroy(&conv_mutex);
}