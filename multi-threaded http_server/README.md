# A Multi-Threaded HTTP Server

## Project Details:

### Multi-Threading
Made a server that uses a thread-pool design, a popular concurrent programming construct for implementing
servers. A thread pool has two types of threads, worker threads, which process requests, and a dispatcher
thread, which listens for connections and dispatches them to the workers. It makes sense to
dispatch requests by having the dispatcher push them into a threadsafe queue and having worker threads pop elements from that queue.

### Handle Put/Get
In our implementation, we treated requests into two different categories.
Put receives a message and a file to write into, get receives a filename 
and returns the contents of the file to the standard output for the user/
Any other requests are considered foul and would face an error message.

### Queue
We use a simple queue implementation to keep a linear order track of 
the requests, simply using threads and equivalent locks to make sure
there is no atomicity between requests.
