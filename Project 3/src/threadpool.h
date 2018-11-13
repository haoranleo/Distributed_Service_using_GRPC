/*
 * CS 6210 Project 3 
 * Distributed Service using GRPC
 *
 * Haoran Li, 903377792
 * Submission Date: Nov.11, 2018
 * Location: 950 Marietta St. NW, Atlanta, GA
 *
 * Threadpool library
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>
#include <list>
#include <vector>
#include <condition_variable>

using std::cout;
using std::endl;
using std::move;
using std::thread;
using std::function;
using std::lock_guard;
using std::mutex;
using std::unique_lock;
using std::vector;
using std::list;
using std::atomic_int;
using std::atomic_bool;
using std::condition_variable;

class threadpool {
   public:
      threadpool(unsigned num_threads): thread_pool(num_threads), remain(0), hold_flg(false), finish_flg(false) {	
         unsigned int i;
         cout << "Threadpool initialize successfully!" << endl;
         cout << "Maximum number of threads in threadpool is: " << num_threads << endl;
         for(i = 0; i < num_threads; i++){
            thread_pool[i] = move(thread([this, i]{this->task();}));
         }
      }

      ~threadpool() {
         joinAll();
      }
      
      void allWait() {
         if(remain) {
            unique_lock<mutex> lock(m_wait);
            is_await.wait(lock, [this]{return (this->remain == 0);});
            lock.unlock();
         }
      }

      void addJob(function<void()> job) {
         lock_guard<mutex> lock(my_queue);
         job_queue.emplace_back(job);
         remain++;
         is_available.notify_one();
      }

      void joinAll() {
         bool wait_all = true;
         if(!finish_flg) {
            if(wait_all) {
               allWait();
            }

            hold_flg = true;
            is_available.notify_all();
            for(auto &thread : thread_pool) {
               if(thread.joinable()) {
                  thread.join();
               }
            }
            finish_flg = true;
         }
      }

   private:
      void task() {
         while(!hold_flg) {
            next_task()();
            remain--;
            is_await.notify_one();
         }
      }

      function<void()> next_task() {
         function<void()> task;
         unique_lock<mutex> lock(my_queue);
         is_available.wait(lock, [this]()->bool{return (hold_flg || job_queue.size());});
         if(!hold_flg) {
            task = job_queue.front();
            job_queue.pop_front();
         } else { 
            task = []{};
            remain++;
         }
         return task;
      }

      vector<thread> thread_pool;
      mutex my_queue;
      mutex m_wait;
      list<function<void()>> job_queue;
      atomic_int remain;
      atomic_bool hold_flg;
      atomic_bool finish_flg;
      condition_variable is_available;
      condition_variable is_await;
};

#endif
