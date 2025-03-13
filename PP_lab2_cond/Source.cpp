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
		// ����������� ������� � ������� ������������ ������
		err = pthread_mutex_lock(&mutex);
		if (err != 0)
			err_exit(err, "Cannot lock mutex");
		while (state == FULL) {
			err = pthread_cond_wait(&cond, &mutex);
			if (err != 0)
				err_exit(err, "Cannot wait on condition variable");
		}
		// ������� ������, ��� �� ������ �� �������� �������.
		// ���������� ����� �����.
		store = rand();
		current_product++;
		state = FULL;
		// �������� ������, ��� �� ������ �������� �����.
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
		// ����������� ������� � ������� ��������� ������� �� ������
		err = pthread_mutex_lock(&mutex);
		if (err != 0)
			err_exit(err, "Cannot lock mutex");
		while (state == EMPTY) {
			err = pthread_cond_wait(&cond, &mutex);
			if (err != 0)
				err_exit(err, "Cannot wait on condition variable");
		}
		// ������� ������, ��� �� ������ ������� �����.
		// ���������� ���.
		cout << "Consuming number " << store << "...";
		//Sleep(1);
		cout << "done" << endl;
		state = EMPTY;
		// �������� ������, ��� �� ������ �� �������� �������.
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
	pthread_t thread1, thread2; // �������������� �������
	// �������������� ������� � �������� ����������
	err = pthread_cond_init(&cond, NULL);
	if (err != 0)
		err_exit(err, "Cannot initialize condition variable");
	err = pthread_mutex_init(&mutex, NULL);
	if (err != 0)
		err_exit(err, "Cannot initialize mutex");
	// ������ ������
	err = pthread_create(&thread1, NULL, producer, NULL);
	if (err != 0)
		err_exit(err, "Cannot create thread 1");
	err = pthread_create(&thread2, NULL, consumer, NULL);
	if (err != 0)
		err_exit(err, "Cannot create thread 2");
	// ���������� ���������� �������
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	// ����������� �������, ��������� � ���������
	// � �������� ����������
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}