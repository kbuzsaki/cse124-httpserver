#ifndef ASYNC_CONNECTION_HANDLER_H
#define ASYNC_CONNECTION_HANDLER_H

#include "async_event_loop.h"
#include "async_connection.h"


std::shared_ptr<Pollable> handle_socket_connection(std::shared_ptr<AsyncSocketConnection> conn);

#endif //ASYNC_CONNECTION_HANDLER_H
