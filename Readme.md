## CS 6210 Project 3
Author: Haoran Li, haoranli@gatech.edu

## Description
This project implements major chunks of a distributed service using **grpc**. To be more specific, the goal is to build a store (think of Amazon Store!) with asynchronous mechanism, which receives requests from different users, querying the prices offered by the different registered vendors.

In addition, this project uses my own threadpool: upon receiving a client request, the store will assign a thread from the threadpool to the incoming request for processing.
* The thread will make async RPC calls to the vendors.
* The thread will await for all results to come back.
* The thread will collate the results.
* The thread will reply to the store client with the results of the call.
* Having completed the work, the thread will return to the threadpool.

## Source file
**store.cc:** Implementation of store management.
**threadpool.h** Implementation of threadpool management.

## Compile
Simply use **make**

## Run
The server is able to accept command line input of the address on which it is going to expose its service and maximum number of threads its threadpool should have. 
And the default address is: `localhost:50040`, the default maximum number of threads is `4`.
The command line argument format is like:
```
	./store address maximum_number_of_threads
```

## Test
Simply run following command:
```
	cd Project\ 3/test
	make
	./run_vendors ../src/vendor_addresses.txt &
	cd ../src
	make
	./store address maximum_number_of_threads
	cd ../test
	./run_tests $IP_and_port_on_which_store_is_listening 
```