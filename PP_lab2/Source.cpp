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
int task_list[TASKS_COUNT]; // ������ �������
int current_task = 0; // ��������� �� ������� �������
pthread_mutex_t mutex; // �������
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
	// ���������� � ����� ��������� �������
	while (true) {
		// ����������� ������� ��� ��������������� �������
		// � ��������� �������� ������� (����������
		// current_task)
		err = pthread_mutex_lock(&mutex);
		if (err != 0)
			err_exit(err, "Cannot lock mutex");
		// ���������� ����� �������� �������, ������� ����� ���������
		task_no = current_task;
		// �������� ��������� �������� ������� �� ���������
		current_task++;
		// ����������� �������
		err = pthread_mutex_unlock(&mutex);
		if (err != 0)
			err_exit(err, "Cannot unlock mutex");
		// ���� ����������� ����� ������� �� ���������
		// ���������� �������, �������� �������, �������
		// �������� �������.
		// � ��������� ������ ��������� ������ ������
		if (task_no < TASKS_COUNT)
			do_task(task_no);
		else
			return NULL;
	}
}
int main()
{
	int err; // ��� ������
	// �������������� ������ ������� ���������� �������
	// ������ ������
	current_task=0;
	for (int i = 0; i < TASKS_COUNT; i++)
		task_list[i] = rand() % TASKS_COUNT;
	// �������������� �������

	err = pthread_mutex_init(&mutex, NULL);
	if (err != 0)
		err_exit(err, "Cannot initialize mutex");


	// �������������� ���������� ������� � ����������
	int threads_number = 32;
	cout << "threads_number: " << threads_number << endl;
	pthread_t* threads = new pthread_t[threads_number];
	for (size_t n = 0; n < threads_number; n++) {
		// �������� ������
		err = pthread_create(&threads[n], NULL, thread_job, (int*)n+1);
		// ���� ��� �������� ������ ��������� ������, �������
		// ��������� �� ������ � ���������� ������ ���������
		if (err != 0) {
			cout << "�� ���������� ������� �����: " << err << endl;
			exit(-1);
		}
	}

	auto start = std::chrono::steady_clock::now();

	// ������� ���������� �������
	for (size_t n = 0; n < threads_number; n++) {
		pthread_join(threads[n], NULL);
	}

	// ����������� �������, ��������� � ���������
	pthread_mutex_destroy(&mutex);
	std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
	cout << "Time spent: " << elapsed.count() << " seconds." << endl;
}