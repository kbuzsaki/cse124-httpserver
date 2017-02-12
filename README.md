# CSE 124 Project 1

Kyle Buzsaki
A11462453

I have completed all 4 of the extensions. The fourth extension can be invoked by
using "async" as the threading model when running httpd.

I discussed the project with the following people:

- Kyle Huynh
- Jordan Guymon


## Design Overview

Each class in my project has a comment block above its declaration that gives a
brief description of its abstraction and purpose.

I make heavy use of abstract classes / interfaces to encapsulate dependencies and
behavior. I use the c++11 shared_ptr<T> class to wrap all pointer usage and simplify
the memory model.

### Synchronous Design

At the edges of the system, I encapsulate network and file system access
with the Listener, Connection, FileRepository, File, and DnsClient abstract classes.
These classes all have two implementations, one that actually talks to the network
or file system, and one that mocks out behavior using in memory data structures.
This modeling both forces interaction with dependencies to be encapsulated at the edges
of the system, and enables isolated unit tests with mock data that do not depend
on external systems.

Within the system, I encapsulate pluggable pieces of logic with abstract classes.
HttpConnectionHandler, HttpRequestHandler, and HttpRequestFilter all represent
pluggable interfaces that can be fulfilled in many ways. HttpConnectionHandler
is implemented with a blocking handler, a thread-per-connection handler, and a
thread pooled handler. HttpRequestHandler is implemented by a FileServingHandler
that serves files from a FileRepository and by RequestFilterMiddleware which
applies a RequestFilter to incoming requests and passes them along to an internal
handler if the request passes the filter. This design allows for arbitrary layering
of additional middleware filtering and could be extended to support different
request handling logic depending on attributes of the request like the uri.

Finally, certain concrete pieces of logic are often encapsulated in simple classes
or utility functions. See the HttpFrame, HttpRequest, and HttpResponse classes
in http.h; the CidrBlock, HtAccessRule, and HtAccess classes in htaccess.h,
and the various utility functions in util.h.

### Non-Blocking Event Driven Design

The above overview applies largely to my synchronous server design. The asynchronous
event driven design uses a similar hierarchical structure but with a parallel set of
non-blocking abstractions.

The core of my event loop is a list of pending "Pollable" operations that I feed to
the poll() system call. When a Pollable is ready for operation, the event loop notifies
the Pollable and optionally enqueus an additional pollable onto itself.

Layering abstractions in the async event loop model are accomplished using a series
of nested callbacks. Operations that would block in the synchronous model instead accept
a callback to invoke when the operation is complete and return a Pollable to enqueue
on the event loop. I make heavy use of c++11 lambda functions when passing and managing
callbacks to reduce the verbosity of closing over request state. The inspiration for
this model comes from my experience with node.js and other non-blocking event driven
designs.
