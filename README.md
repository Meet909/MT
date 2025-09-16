
# Multithreading cheat sheet for modern c++
## Thread

```
  std::thread t1(threadFunction, 1);
  t1.join(); // Waits for t1 to finish before continuing

  std::thread t2(threadFunction, 2);
  t2.detach(); // t2 runs independently; main does not wait
  std::cout << "t2 has been detached.\n";
```

## Mutex Reference

####  mutex

```
  std::mutex mtx;
  
  mtx.lock();
  mtx.unlock();
  mtx.try_lock();
```

#### shared_mutex

```
  std::shared_mutex smtx;

  smtx.lock();            //  Exclusive locking 
  smtx.unlock();          //  Exclusive locking 
  smtx.try_lock();        //  Exclusive locking 
  or
  smtx.lock_shared();     //  Shared locking 
  smtx.unlock_shared();   //  Shared locking 
  smtx.try_lock_shared(); //  Shared locking 
```

####  recursive_mutex

```
  std::recursive_mutex rmtx;
  
  rmtx.lock();
  rmtx.unlock();
  rmtx.try_lock();
```

## Mutex Management 

Instead of using manually lock() and unlock() functions in mutexes we use RAII wrappers. It's safer if exceptions arise :)

####   lock_guard
- Simplest form. 
- Locks on construction, unlocks on destruction.

```
  void f() {
    std::lock_guard<std::mutex> lock(mtx);
    // critical section
} // m.unlock() automatically
```

####   unique_lock
- More flexible than lock_guard. 
- Can unlock() and lock() again.
- Can be constructed without immediately locking (std::defer_lock).

```
  void f() {
    std::unique_lock<std::mutex> lock(m, std::defer_lock);
    // do some setup
    lock.lock();  // lock later
    // critical section
} // unlock automatically
```

####  shared_lock

- Works with std::shared_mutex
- Can be constructed without immediately locking (std::defer_lock).
```
  void reader() {
    std::shared_lock<std::shared_mutex> lock(smtx);
    // multiple readers allowed here
}

void writer() {
    std::unique_lock<std::shared_mutex> lock(smtx);
    // exclusive access
}
```

####  * scoped_lock
- Use std::scoped_lock (C++17), which locks multiple mutexes without risk of deadlock.

```
  void f() {
    std::scoped_lock lock(mtx, mtx); // locks both safely
    // critical section
}
```

##  Call once 
- Euns exactly once, even in the presence of multiple threads racing

```
  std::once_flag init_flag;

  void f() { //Multiple threads
    std::call_once(init_flag, init_resource);
}
```
##  Atomic operations

- Euns exactly once, even in the presence of multiple threads racing

```
  std::atomic<int> counter{0};  // shared atomic counter

  void f() { //Multiple threads
     counter.fetch_add(1);
}
```
##  Futures
No need to manage mutex or condition variables manually for this simple producer-consumer scenario. Avoid blocking the main thread unnecessarily. 

- std::async: Launches each computation asynchronously.
- std::future: Represents the eventual result of each task.
- .get(): Waits for completion and retrieves the result.

```
  int square(int x) {
    return x * x;
  }

  std::future<int> fut = std::async(std::launch::async, square, 5); 
  int result = fut.get(); / Wait for the result and get it
}
```

##  Condition variables
- std::condition_variable
- notify()_one or notify_all()
- wait()
- Unlock before notify! - "the waiting thread wakes up but cannot proceed because you still own the lock."

```  
std::condition_variable cv;
std::mutex m;

bool ready = false;
bool processed = false;
std::string data;

void worker_thread()
{
    std::unique_lock lk(m);                 
    cv.wait(lk, []{ return ready; });      
    data += " after processing";
    processed = true;
    lk.unlock();
    cv.notify_one();                  // Wakes cv.wait(lk, []{ return processed; });
}
 
int main()
{
    std::thread worker(worker_thread);
    data = "Example data";                 
    {
        std::lock_guard lk(m);
        ready = true;                      
    }
    cv.notify_one();                  // Wakes cv.wait(lk, []{ return ready; });
    {
        std::unique_lock lk(m);
        cv.wait(lk, []{ return processed; });
    }
    worker.join();
}

