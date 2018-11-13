### Big Picture

  - In this project, you are going to implement major chunks of a simple distributed service using [grpc](http://www.grpc.io).
  - Learnings from this project will also help you in the next project as you will become familiar with grpc and multithreading with threadpool.
  
### Overview
  - You are going to build a store (You can think of Amazon Store!), which receives requests from different users, querying the prices offered by the different registered vendors.
  - Your store will be provided with a file of <ip address:port> of vendor servers. On each product query, your server is supposed to request all of these vendor servers for their bid on the queried product.
  - Once your store has responses from all the vendors, it is supposed to collate the (bid, vendor_id) from the vendors and send it back to the requesting client.
  
### Learning outcomes
  - Synchronous and Asynchronous RPC packages
  - Building a multi-threaded store in a distributed service

### How You Are Going to Implement It ( Step-by-step suggestions)

1. Install  [GRPC] (http://www.grpc.io/docs/tutorials/basic/c.html) and [Protobuf] (https://github.com/google/protobuf)
2. Make sure you understand how GRPC- synchronous and asynchronous calls work. Understand the given helloworld [example](https://github.com/grpc/grpc/tree/master/examples/cpp/helloworld). You will be building your store with asynchronous mechanisms ONLY.
3. Establish asynchronous GRPC communication between -
    - Your store and user client. 
    - Your store and the vendors.  
  ( We will be providing you the vendors and the user clients soon.)
4. Create your thread pool and use it. Where will you use it and for what?
   Upon receiving a client request, you store will assign a thread from the thread pool to the incoming request for processing.
    - The thread will make async RPC calls to the vendors
    - The thread will await for all results to come back
    - The thread will collate the results
    - The thread will reply to the store client with the results of the call
    - Having completed the work, the thread will return to the thread pool
5. Do you have your user client request reaching to the vendors now? And can you see the bids from the different vendors at your user client end? Congratulations you almost got it! Now use the test harness to test if your server can serve multiple clients concurrently and make sure that your thread handling is correct. 

### Keep In Mind
1. Your Server has to handle
  - Multiple concurrent requests from clients
  - Be stateless so far as client requests are concerned (once the client request is serviced it can forget the client)
  - Manage the connections to the client requests and the requests it makes to the 3rd party vendors.
2.  Server will get the vendor addresses from a file with line separated strings <ip:port>
3.  Your server should be able to accept `command line input` of the `address` on which it is going to expose its service and `maximum number of threads` its threadpool should have.

### Given to You
  1. run_tests.cc - This will simulate real world users sending concurrent product queries. This will be released soon to you.
  2. client.cc - This will be providing ability to connect to the store as a user.
  3. vendor.cc - This wil act as the server providing bids for different products. Multiple instances of it will be run listening on different ip address and port.
  4. `Two .proto files`  
    - store.proto - Comm. protocol between user(client) and store(server)  
    - vendor.proto -Comm. protocol between store(client) and vendor(server)  

### How to run the test setup
  - Goto test directory and run `make` command
  - Two binaries would be created `run_tests` and `run_vendors`
  - First run the command `./run_vendors ../src/vendor_addresses.txt &` to start a process which will run multiple servers on different threads listening to (ip_address:ports) from the file given as command line argument.
  - Then start up your store which will read the same address file to know vendors' listening addresses. Also, your store should start listening on a port(for clients to connect to) given as command line argument.
  - Then finally run the command `./run_tests $IP_and_port_on_which_store_is_listening $max_num_concurrent_client_requests` to start a process which will simulate real world clients sending requests at the same time.
  - This process read the queries from the file `product_query_list.txt`
  - It will send some queries and print back the results, which you can use to verify your whole system's flow.

### Grading
This project is not performance oriented, we will only test the functionality and correctness.  
The Rubric will be:
  - `Async client of your store` - 4 pts.
        - If you do 'Sync client', you only get 2pts.
  - 'Async server of your store' - 4 pts.
    - If you do 'Sync server', you only get 2pts.
  - `Threadpool management` - 4 pts
  
### Deliverables
Please follow the instructions carefully. The folder you hand in must contain the following:
  - `README` - text file containing anything about the project that you want to tell the TAs.
  - `Makefile` - it is already given to you working for the two files below. You might need to change it if you add more source files.
  - `Store source files` - store.cc(must) containing the source code for store management
  - `Threadpool source files` - threadpool.h(must), containing the source code for threadpool management
  - You can add supporting files too(in addition to above two), if you need to keep your code more structured, clean etc.
Hand in your src folder as a zip file through [Canvas](https://canvas.gatech.edu)
