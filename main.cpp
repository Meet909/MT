#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <sstream>
#include <vector>
#include <chrono>
#include <atomic>

/*
 * Model based on multiple consumers and producers
*/

std::vector <int> data_vector;

std::mutex smtx;
std::mutex cout_mtx;
std::condition_variable go_consume;

std::atomic<int> MAX_production {0};
std::atomic<bool> production_stopped{false};

class tcout : public std::ostringstream //safety std::cout
{
public:
	~tcout()
	{
		std::unique_lock<std::mutex> lock(cout_mtx);
		std::cout << std::this_thread::get_id() << " : " << this->str() << std::endl;
	}
};

void worker_thread(unsigned const short index_thread)
{
	tcout() << "[worker_thread]: DOING PRODUCTION " << index_thread;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	/*
	  Doing some production
	*/
	
	std::unique_lock<std::mutex> lock_smtx(smtx, std::defer_lock);
	lock_smtx.lock();

	for (auto i = 0; i <= index_thread; ++i )
	{
		if (MAX_production.load() >= 8)
                {
                        tcout() << "[worker_thread]: STOPPING PRODUCTION " << index_thread;
			production_stopped.store(true);
			break;
                }
		else
		{
			data_vector.push_back(index_thread * 100 + i);
			std::atomic_fetch_add(&MAX_production, 1);
			tcout() << "[worker_thread]: " << index_thread << " --> " << int(index_thread * 100 + i);
		}
	}
	lock_smtx.unlock();
       	go_consume.notify_all();
}

void consumer_thread(unsigned const short index_thread)
{
	tcout() << "[consumer_thread]: STARTED " << index_thread;

	while(true)
	{
		std::unique_lock<std::mutex> lock_smtx(smtx);

		if(production_stopped && data_vector.empty()) break;

		if(go_consume.wait_for(lock_smtx, std::chrono::milliseconds(100) , []{ return !data_vector.empty(); } ))
		{
			tcout() << "[consumer_thread]: " << index_thread << " <-- " << data_vector.back();	
			data_vector.pop_back();
	
			lock_smtx.unlock();
		}
		else
			continue;
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		/*
          	Doing some consumption
        	*/
	}

}

int main()
{
	unsigned const short number_of_workers = 4;
	unsigned const short number_of_consumers = 3; 
	std::vector<std::thread> workers;
	std::vector<std::thread> consumers;

	for (auto i = 0; i < number_of_workers; ++i)
		workers.push_back(std::thread(worker_thread, i));

	for (auto i = 0; i < number_of_consumers; ++i)
                consumers.push_back(std::thread(consumer_thread, i));

	tcout() << "[main]";	

	for (auto &a : workers)
		a.join();

	for (auto &a : consumers)
                a.join();

}
