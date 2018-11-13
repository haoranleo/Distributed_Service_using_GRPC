/*
 * CS 6210 Project 3 
 * Distributed Service using GRPC
 *
 * Haoran Li, 903377792
 * Submission Date: Nov.11, 2018
 * Location: 950 Marietta St. NW, Atlanta, GA
 *
 * Vendor & Store implementation
 */

# include <iostream>
# include <memory>
# include <string>
# include <thread>
# include <sstream>
# include <fstream>
# include "threadpool.h"

# include <grpc++/grpc++.h>
# include <grpc/support/log.h>

# include "vendor.grpc.pb.h"
# include "store.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using store::ProductQuery;
using store::ProductReply;
using store::ProductInfo;
using store::Store;
using vendor::BidQuery;
using vendor::BidReply;
using vendor::Vendor;

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::unique_ptr;
using std::shared_ptr;
using std::ifstream;
using std::ostringstream;

/* Vendor Stub */
class VendorClient {
   public:
      explicit VendorClient(shared_ptr<Channel> channel) 
         : stub_(Vendor::NewStub(channel)){}

      // Assembles the client's payload, sends it and presents the response back
      // from the server.
      string getBid(const string &product_name){
         // Data we are sending to the server.
         BidQuery bid_query;
         bid_query.set_product_name(product_name);

         // Container for the data we expect from the server.
         BidReply bid_reply;

         // Context for the client. It could be used to convey extra information to
         // the server and/or tweak certain RPC behaviors.
         ClientContext context;

         // The producer-consumer queue we use to communicate asynchronously with the
         // gRPC runtime.
         CompletionQueue cq;

         // Storage for the status of the RPC upon completion.
         Status status;

         // stub_->AsyncgetProductBid() creates an RPC object, returning
         // an instance to store in "call" but does not actually start the RPC
         // Because we are using the asynchronous API, we need to hold on to
         // the "call" instance in order to get updates on the ongoing RPC.
         unique_ptr<ClientAsyncResponseReader<BidReply>> rpc(stub_->AsyncgetProductBid(&context, bid_query, &cq));

         // Request that, upon completion of the RPC, "reply" be updated with the
         // server's response; "status" with the indication of whether the operation
         // was successful. Tag the request with the integer 1.
         rpc->Finish(&bid_reply, &status, (void *)1);

         void *got_tag;
         bool ok = false;

         // Block until the next result is available in the completion queue "cq".
         // The return value of Next should always be checked. This return value
         // tells us whether there is any kind of event or the cq_ is shutting down.
         GPR_ASSERT(cq.Next(&got_tag, &ok));

         // Verify that the result from "cq" corresponds, by its tag, our previous
         // request.
         GPR_ASSERT(got_tag == (void *)1);

         // ... and that the request was completed successfully. Note that "ok"
         // corresponds solely to the request for updates introduced by Finish().
         GPR_ASSERT(ok);

         // Act upon the status of the actual RPC.
         if (status.ok()) {
            ostringstream price;
            price << bid_reply.price();
            string ret = "Vendor ID: " + bid_reply.vendor_id() + " |  Product Name: " + product_name + " |  Price: " + price.str() + "\n"; 
            return ret;
         }
         else {
            return "RPC failed";
         }
      }

   private:
      // Out of the passed in Channel comes the stub, stored here, our view of the
      // server's exposed services.
      unique_ptr<Vendor::Stub> stub_;
};

string stub_call(string product_name, string ip) {
   // Instantiate the client. It requires a channel, out of which the actual RPCs
   // are created. This channel models a connection to an endpoint. We indicate that 
   // the channel isn't authenticated (use of InsecureChannelCredentials()).
   string stub_reply;
   VendorClient vendor_client(grpc::CreateChannel(ip, grpc::InsecureChannelCredentials()));
   stub_reply = vendor_client.getBid(product_name);
   cout << "Bid: " << stub_reply << endl;   //Return string from vendor
   return stub_reply;
}

/* Store Server */
class Store_Server final {
   public:
      ~Store_Server() {
         server_->Shutdown();
         // Always shutdown the completion queue after the server.
         cq_->Shutdown();
      }

      void runStore(string addr) {
         ServerBuilder builder;
         string server_addr(addr);

         // Listen on the given address without any authentication mechanism.
         builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());

         // Register "service_" as the instance through which we'll communicate with
         // clients. In this case it corresponds to an *asynchronous* service.
         builder.RegisterService(&service_);

         // Get hold of the completion queue used for the asynchronous communication
         // with the gRPC runtime.
         cq_ = builder.AddCompletionQueue();

         // Finally assemble the server.
         server_ = builder.BuildAndStart();
         cout << "Store address is: " << server_address << endl;

         // Proceed to the server's main loop.
         HandleRpcs();
      }

   private:
      // Class encompasing the state and logic needed to serve a request.
      class CallData {
         public:
            // Take in the "service" instance (in this case representing an asynchronous
            // server) and the completion queue "cq" used for asynchronous communication
            // with the gRPC runtime.
            CallData(Store::AsyncService *service, ServerCompletionQueue *cq) 
               : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
               // Invoke the serving logic right away.
               proceed();
            }

            void proceed() {
               if (status_ == CREATE) {
                  // Make this instance progress to the PROCESS state.
                  status_ = PROCESS;

                  // As part of the initial CREATE state, we *request* that the system
                  // start processing SayHello requests. In this request, "this" acts are
                  // the tag uniquely identifying the request (so that different CallData
                  // instances can serve different requests concurrently), in this case
                  // the memory address of this CallData instance.
                  service_->RequestgetProducts(&ctx_, &request_, &responder_, cq_, cq_, this);
               }
               else if (status_ == PROCESS) {
                  // Spawn a new CallData instance to serve new clients while we process
                  // the one for this CallData. The instance will deallocate itself as
                  // part of its FINISH state.
                  new CallData (service_, cq_);
                  
                  // The actual processing.
                  vector<string> ip;
                  string addr;
                  ifstream file("vendor_addresses.txt");

                  while (getline(file, addr)) {
                     ip.push_back(addr);
                  }

                  for(int i = 0; i < ip.size(); i++) {
                     string product_name = request_.product_name();
                     string reply_msg = stub_call(product_name, ip[i]); //To get vendor's reply
                     ProductInfo product;
                     product.set_vendor_id(reply_msg);
                  
                     reply_.add_products()->CopyFrom(product); //Reply to client
                     reply_.products(0).vendor_id();
                     reply_.products(0).price();
                  }

                  // And we are done! Let the gRPC runtime know we've finished, using the
                  // memory address of this instance as the uniquely identifying tag for
                  // the event.
                  status_ = FINISH;
                  responder_.Finish(reply_, Status::OK, this);
               }
               else {
                  GPR_ASSERT(status_ == FINISH);
                  // Once in the FINISH state, deallocate ourselves (CallData).
                  delete this;
               }
            }

         private:
            // The means of communication with the gRPC runtime for an asynchronous server.
            Store::AsyncService *service_;

            // The producer-consumer queue where for asynchronous server notifications.
            ServerCompletionQueue *cq_;

            // Context for the rpc, allowing to tweak aspects of it such as the use
            // of compression, authentication, as well as to send metadata back to the
            // client.
            ServerContext ctx_;

            // What we get from the client.
            ProductQuery request_;

            // What we send back to the client.
            ProductReply reply_;

            // The means to get back to the client.
            ServerAsyncResponseWriter<ProductReply> responder_;

            // Let's implement a tiny state machine with the following states.
            enum CallStatus {
               CREATE,
               PROCESS,
               FINISH
            };

            // The current serving state.
            CallStatus status_;
      };

      void HandleRpcs() {
         // Spawn a new CallData instance to serve new clients.
         new CallData(&service_, cq_.get());
         // uniquely identifies a request.
         void *tag;
         bool ok;

         while (1) {
            // Block waiting to read the next event from the completion queue. The
            // event is uniquely identified by its tag, which in this case is the
            // memory address of a CallData instance.
            // The return value of Next should always be checked. This return value
            // tells us whether there is any kind of event or cq_ is shutting down.
            GPR_ASSERT(cq_->Next(&tag, &ok));
            GPR_ASSERT(ok);
            static_cast<CallData *>(tag)->proceed();
         }
      }

      unique_ptr<ServerCompletionQueue> cq_;
      Store::AsyncService service_;
      unique_ptr<Server> server_;
};

string store_addr;   // Used to store different store's IP address

int run_store() {
   Store_Server store_server;
   store_server.runStore(store_addr);
   return 0;
}

int main(int argc, char** argv) {
   unsigned int max_threads;
   // Check input parameters
   if(argc > 3){
      cout << "Invalid Input Parameter! Input format: ./Name store_address maximum_number_of_threads" << endl;
   } else if(argc == 3){
      store_addr = string(argv[1]);
      max_threads = atoi(argv[2]);
   } else if(argc == 2){
      store_addr = string(argv[1]);
      max_threads = 4;
   } else{
      store_addr = "localhost:50040";
      max_threads = 4;
   }

   threadpool thread_pool(max_threads);
   thread_pool.addJob(&run_store);
   thread_pool.joinAll();

   return EXIT_SUCCESS;
}
