#include <chrono>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "connection.h"
#include "connection_handlers.h"
#include "htaccess.h"
#include "http.h"
#include "request_filters.h"
#include "request_handlers.h"
#include "listener.h"
#include "mocks.h"
#include "server.h"
#include "util.h"

using namespace std;
using std::chrono::system_clock;

class TestRunner {
    int passes;
    int fails;
    vector<string> messages;

public:
    TestRunner() : passes(0), fails(0), messages() {}

    void pass() {
        passes++;
    }

    void fail(string message) {
        fails++;
        messages.push_back(message);
    }

    void assert_true(bool val, string message) {
        if (val) {
            this->pass();
        } else {
            this->fail("assert_true() failed: " + message);
        }
    }

    void assert_false(bool val, string message) {
        if (!val) {
            this->pass();
        } else {
            this->fail("assert_true() failed: " + message);
        }
    }

    template<typename T>
    void assert_equal(T expected, T actual, string message) {
        if (expected != actual) {
            stringstream error;
            error << message << ": expected '" << expected << "', was '" << actual << "'";
            this->fail(error.str());
        } else {
            this->pass();
        }
    }

    template<typename E>
    void assert_throws(function<void ()> f, string message) {
        try {
            f();
            this->fail("failed to throw: " + message);
        } catch (E&) {
            this->pass();
        } catch (...) {
            this->fail("threw unexpected exception: " + message);
        }
    }

    void print_results(ostream& os) {
        os << "Passes: " << passes << endl
           << "Fails: " << fails << endl
           << endl;

        if (fails != 0) {
            os << "Failure messages:" << endl;

            for (size_t i = 0; i < messages.size(); i++) {
                os << messages[i] << endl;
            }
        }
    }
};

string itos(int i) {
    stringstream buf;
    buf << i;
    return buf.str();
}

template <typename T, unsigned int S>
int array_size(const T (&)[S]) {
    return S;
}

HttpRequest make_request(string uri, vector<HttpHeader> headers={}, struct in_addr remote_ip={0}) {
    return {"GET", uri, HTTP_VERSION_1_1, headers, "", remote_ip};
}

HttpResponse make_response(HttpStatus status, vector<HttpHeader> headers={}, string body="") {
    return {HTTP_VERSION_1_1, status, headers, body};
}

shared_ptr<MockFile> make_file(string contents) {
    return make_shared<MockFile>(true, contents, system_clock::time_point());
}


void test_split(TestRunner& runner) {
    runner.assert_equal(vector<string>{""}, split_n("", " ", 10), "split_n 10 with empty string");
    runner.assert_equal(vector<string>{"a"}, split("a", " "), "splitting on space with 1 part");
    runner.assert_equal(vector<string>{"a", "b"}, split("a b", " "), "splitting on space with 2 parts");
    runner.assert_equal(vector<string>{"a", "b", "c"}, split("a b c", " "), "splitting on space with 3 parts");
    runner.assert_equal(vector<string>{"a", "b", "c", "d"}, split("a b c d", " "), "splitting on space with 4 parts");

    runner.assert_equal(vector<string>{"aaa"}, split("aaa", " "), "splitting on space with 1 multicharacter part");
    runner.assert_equal(vector<string>{"aaa", "bbb"}, split("aaa bbb", " "), "splitting on space with 2 multicharacter parts");
    runner.assert_equal(vector<string>{"aaa", "bbb", "ccc"}, split("aaa bbb ccc", " "), "splitting on space with 3 multicharacter parts");

    runner.assert_equal(vector<string>{"aaa"}, split_n("aaa", " ", 100), "split_n 100 on space with 1 multicharacter part");
    runner.assert_equal(vector<string>{"aaa", "bbb"}, split_n("aaa bbb", " ", 100), "split_n 100 on space with 2 multicharacter parts");
    runner.assert_equal(vector<string>{"aaa", "bbb", "ccc"}, split_n("aaa bbb ccc", " ", 100), "split_n 100 on space with 3 multicharacter parts");

    runner.assert_equal(vector<string>{"aaa"}, split_n("aaa", " ", 0), "split_n 0 on space with 1 multicharacter part");
    runner.assert_equal(vector<string>{"aaa bbb"}, split_n("aaa bbb", " ", 0), "split_n 0 on space with 2 multicharacter parts");
    runner.assert_equal(vector<string>{"aaa bbb ccc"}, split_n("aaa bbb ccc", " ", 0), "split_n 0 on space with 3 multicharacter parts");

    runner.assert_equal(vector<string>{"aaa"}, split_n("aaa", " ", 1), "split_n 1 on space with 1 multicharacter part");
    runner.assert_equal(vector<string>{"aaa", "bbb"}, split_n("aaa bbb", " ", 1), "split_n 1 on space with 2 multicharacter parts");
    runner.assert_equal(vector<string>{"aaa", "bbb ccc"}, split_n("aaa bbb ccc", " ", 1), "split_n 1 on space with 3 multicharacter parts");

    runner.assert_equal(vector<string>{"dog"}, split("dog", "ab"), "split on 'ab' with 1 multicharacter part");
    runner.assert_equal(vector<string>{"dog", "cat"}, split("dogabcat", "ab"), "split on 'ab' with 2 multicharacter parts");
    runner.assert_equal(vector<string>{"dog", "cat", "bear"}, split("dogabcatabbear", "ab"), "split on 'ab' with 3 multicharacter parts");
    runner.assert_equal(vector<string>{"dog", "cat", "bear", ""}, split("dogabcatabbearab", "ab"), "split on 'ab' with 3 multicharacter parts and trailing sep");

    runner.assert_equal(vector<string>{"Host", "localhost:6060"}, split_n("Host: localhost:6060", ": ", 1), "splitting http header containing ':'");

    runner.assert_equal(vector<string>{"f", "o", "o"}, split("foo", ""), "splitting on empty string");
    runner.assert_equal(vector<string>{""}, split("", ""), "splitting empty string on empty string");
}

void test_canonicalize_path(TestRunner& runner) {
    runner.assert_equal(string(""), canonicalize_path(""), "canonicalize empty string");

    runner.assert_equal(string("/"), canonicalize_path("/"), "canonicalize root path");
    runner.assert_equal(string("/foo/bar"), canonicalize_path("/foo/bar"), "canonicalize valid path");
    runner.assert_equal(string("/foo/bar"), canonicalize_path("/foo/./bar"), "canonicalize path with '.'");
    runner.assert_equal(string("/bar"), canonicalize_path("/foo/../bar"), "canonicalize path with '..'");
    runner.assert_equal(string("/foo/bar"), canonicalize_path("/foo/./bar/../baz/./cat/./../../bar"), "canonicalize weird path");

    runner.assert_equal(string("/foo/bar"), canonicalize_path("foo/bar"), "canonicalize path without leading /");
    runner.assert_equal(string("/foo/bar"), canonicalize_path("/foo/bar/"), "canonicalize path with trailing /");
    runner.assert_equal(string("/foo/bar"), canonicalize_path("foo/bar/"), "canonicalize path without leading / and with trailing /");
    runner.assert_equal(string("/foo/bar"), canonicalize_path("/foo///////bar"), "canonicalize path extra /");

    runner.assert_equal(string(""), canonicalize_path("/foo/../../bar"), "canonicalize escaping path");
    runner.assert_equal(string(""), canonicalize_path("/foo/bar/../baz/../../../bar"), "canonicalize weird escaping path");
}

void test_to_http_date(TestRunner& runner) {
    runner.assert_equal(string("Sat, 19 Mar 2016 06:19:24 GMT"), to_http_date(make_time_point(2016, 3, 19, 6, 19, 24)), "to http date: 2016/03/19 6:19:24");
    runner.assert_equal(string("Thu, 01 Jan 1970 00:00:00 GMT"), to_http_date(make_time_point(1970, 1, 1, 0, 0, 0)), "to http date: 1970/01/01 0:00:00");
    runner.assert_throws<runtime_error>([](){ make_time_point(1969, 12, 31, 23, 59, 59); }, "make time point before epoch");
}

void test_ends_with(TestRunner& runner) {
    runner.assert_equal(true, ends_with("", ""), "empty string ends with empty string");
    runner.assert_equal(true, ends_with("foo.html", ".html"), "foo.html ends with .html");
    runner.assert_equal(false, ends_with("bar.html.bak", ".html"), "bar.html.bak does not end with .html");
    runner.assert_equal(false, ends_with("bar.html.bak", "foo.bar.html.bak"), "bar.html.bak does not end with foo.bar.html.bak");
}

void test_infer_content_type(TestRunner& runner) {
    runner.assert_equal(string("text/plain"), infer_content_type("foo"), "content type foo");
    runner.assert_equal(string("text/html"), infer_content_type("foo.html"), "content type foo.html");
    runner.assert_equal(string("image/png"), infer_content_type("foo.png"), "content type foo.png");
    runner.assert_equal(string("image/jpeg"), infer_content_type("foo.jpg"), "content type foo.jpg");

    runner.assert_equal(string("text/html"), infer_content_type("foo.png.html"), "content type foo.png.html");
    runner.assert_equal(string("image/png"), infer_content_type("foo.html.png"), "content type foo.html.png");
}

void test_mock_connection(TestRunner& runner) {
    MockConnection empty_mock("");
    runner.assert_throws<ConnectionClosed>([&](){ empty_mock.read(); }, "read on empty mock");
    runner.assert_throws<ConnectionClosed>([&](){ empty_mock.read(); }, "second read on empty mock");

    runner.assert_equal(string(""), empty_mock.written(), "empty mock has non-empty written buffer");
    empty_mock.write("foo");
    runner.assert_equal(string("foo"), empty_mock.written(), "initial write to mock failed");
    empty_mock.write("bar");
    runner.assert_equal(string("foobar"), empty_mock.written(), "second write to mock failed");
    empty_mock.write(string("baz\0car", sizeof("baz\0car")-1));
    runner.assert_equal(string("foobarbaz\0car", sizeof("foobarbaz\0car")-1), empty_mock.written(), "null byte write to mock failed");

    MockConnection full_mock("the big cat", 3);
    runner.assert_equal(string("the"), full_mock.read(), "failed first 3 byte read");
    runner.assert_equal(string(" bi"), full_mock.read(), "failed second 3 byte read");
    runner.assert_equal(string("g c"), full_mock.read(), "failed third 3 byte read");
    runner.assert_equal(string("at"), full_mock.read(), "failed final 3 (2) byte read");
    runner.assert_throws<ConnectionClosed>([&](){ full_mock.read(); }, "read of exhausted mock");
    runner.assert_throws<ConnectionClosed>([&](){ full_mock.read(); }, "second read of exhausted mock");
}

void test_mock_listener(TestRunner& runner) {
    vector<shared_ptr<Connection>> connections = {
            make_shared<MockConnection>("1"),
            make_shared<MockConnection>("2"),
            make_shared<MockConnection>("3")
    };
    MockListener mock_listener(connections);

    mock_listener.listen();

    runner.assert_equal(connections[0], mock_listener.accept(), "wrong first accepted conn");
    runner.assert_equal(connections[1], mock_listener.accept(), "wrong second accepted conn");
    runner.assert_equal(connections[2], mock_listener.accept(), "wrong third accepted conn");
}

void test_mock_handler(TestRunner& runner) {
    HttpResponse response = HttpResponse{HTTP_VERSION_1_1, OK_STATUS, vector<HttpHeader>{HttpHeader{"OtherKey", "othervalue"}}, ""};
    MockHttpRequestHandler mock_handler(response);

    vector<HttpRequest> requests = {
        {"GET", "/foo/bar", HTTP_VERSION_1_1, vector<HttpHeader>{HttpHeader{"SomeKey", "somevalue"}}, "", {0}},
        {"PUT", "/", HTTP_VERSION_0_9, vector<HttpHeader>{}, "", {0}},
        {"GET", "/baz", HTTP_VERSION_1_0, vector<HttpHeader>{HttpHeader{"SomeOtherKey", "othersomevalue"}, HttpHeader{"foo", "bar"}}, "", {0}}
    };

    for (size_t i = 0; i < requests.size(); i++) {
        HttpRequest& request = requests[i];
        HttpResponse received_response = mock_handler.handle_request(request);
        runner.assert_equal(request, mock_handler.requests()[mock_handler.requests().size() - 1], "stored request incorrect");
        runner.assert_equal(response, received_response, "received response incorrect");
    }
}

void test_mock_file(TestRunner& runner) {
    system_clock::time_point public_file_time = make_time_point(1995, 12, 14, 2, 24, 3);
    MockFile public_file(true, "some stuff", public_file_time);
    runner.assert_equal(true, public_file.world_readable(), "mock public file was not public");
    runner.assert_equal(string("some stuff"), public_file.contents(), "mock public file had wrong contents");
    runner.assert_equal(public_file_time, public_file.last_modified(), "mock public file had wrong last modified");

    MockFile empty_file(true, "", system_clock::time_point());
    runner.assert_equal(true, empty_file.world_readable(), "mock empty file was not public");
    runner.assert_equal(string(""), empty_file.contents(), "mock empty file had wrong contents");
    runner.assert_equal(system_clock::time_point(), empty_file.last_modified(), "mock empty file had wrong last modified");

    MockFile private_file(false, "some other stuff", system_clock::time_point());
    runner.assert_equal(false, private_file.world_readable(), "mock private file was not private");
    runner.assert_equal(string("some other stuff"), private_file.contents(), "mock private file had wrong contents");
}

void test_mock_file_repository(TestRunner& runner) {
    unordered_map<string, shared_ptr<File>> empty_map;
    MockFileRepository empty_repository(empty_map);
    runner.assert_equal(shared_ptr<File>(), empty_repository.get_file(""), "empty mock repository returned a non null file");
    runner.assert_equal(shared_ptr<File>(), empty_repository.get_file(""), "empty mock repository returned a non null file");
    runner.assert_equal(shared_ptr<File>(), empty_repository.get_file("foo"), "empty mock repository returned a non null file");
    runner.assert_equal(shared_ptr<File>(), empty_repository.get_file("foo"), "empty mock repository returned a non null file");

    shared_ptr<File> foo_file = make_shared<MockFile>(true, "foo contents here", system_clock::time_point());
    shared_ptr<File> private_bar_file = make_shared<MockFile>(false, "bar contents here", system_clock::time_point());
    shared_ptr<File> nested_file = make_shared<MockFile>(true, "baz/car/tar contents here", system_clock::time_point());
    unordered_map<string, shared_ptr<File>> full_map = {
            {"/foo", foo_file},
            {"/bar", private_bar_file},
            {"/baz/car/tar", nested_file}
    };
    MockFileRepository full_repository(full_map);

    runner.assert_equal(shared_ptr<File>(), full_repository.get_file(""), "full mock repository returned a non null file for a missing file");
    runner.assert_equal(shared_ptr<File>(), full_repository.get_file(""), "full mock repository returned a non null file for a missing file");
    runner.assert_equal(shared_ptr<File>(), full_repository.get_file("/missing"), "full mock repository returned a non null file for a missing file");
    runner.assert_equal(shared_ptr<File>(), full_repository.get_file("/missing"), "full mock repository returned a non null file for a missing file");
    runner.assert_equal(foo_file, full_repository.get_file("/foo"), "full mock repository returned the wrong file for foo");
    runner.assert_equal(foo_file, full_repository.get_file("/foo"), "full mock repository returned the wrong file for foo");
    runner.assert_equal(private_bar_file, full_repository.get_file("/bar"), "full mock repository returned the wrong file for bar");
    runner.assert_equal(nested_file, full_repository.get_file("/baz/car/tar"), "full mock repository returned the wrong file for baz/car/tar");
}

void test_mock_dns_client(TestRunner& runner) {
    unordered_map<string, vector<struct in_addr>> empty_map;
    MockDnsClient empty_client(empty_map);
    runner.assert_equal(vector<struct in_addr>(), empty_client.lookup(""), "empty mock client returned a non-empty vector");
    runner.assert_equal(vector<struct in_addr>(), empty_client.lookup(""), "empty mock client returned a non-empty vector");
    runner.assert_equal(vector<struct in_addr>(), empty_client.lookup("foo.bar.com"), "empty mock client returned a non-empty vector");
    runner.assert_equal(vector<struct in_addr>(), empty_client.lookup("foo.bar.com"), "empty mock client returned a non-empty vector");

    vector<struct in_addr> foo_results = {parse_ip("1.2.3.4"), parse_ip("255.255.255.255")};
    vector<struct in_addr> bar_com_results = {parse_ip("192.168.0.1")};
    vector<struct in_addr> www_bar_com_results = {parse_ip("10.0.0.2"), parse_ip("127.0.0.1"), parse_ip("164.2.2.3")};
    unordered_map<string, vector<struct in_addr>> full_map = {
            {"foo", foo_results},
            {"bar.com", bar_com_results},
            {"www.bar.com", www_bar_com_results}
    };
    MockDnsClient full_client(full_map);

    runner.assert_equal(vector<struct in_addr>(), full_client.lookup(""), "full mock client returned non empty vector for missing domain");
    runner.assert_equal(vector<struct in_addr>(), full_client.lookup("bar"), "full mock client returned non empty vector for missing domain");
    runner.assert_equal(foo_results, full_client.lookup("foo"), "mock dns client foo");
    runner.assert_equal(vector<struct in_addr>(), full_client.lookup("bar"), "mock dns client bar");
    runner.assert_equal(vector<struct in_addr>(), full_client.lookup("com"), "mock dns client com");
    runner.assert_equal(bar_com_results, full_client.lookup("bar.com"), "mock dns client bar.com");
    runner.assert_equal(www_bar_com_results, full_client.lookup("www.bar.com"), "mock dns client www.bar.com");
}

void test_buffered_connection(TestRunner& runner) {
    shared_ptr<MockConnection> empty_mock = make_shared<MockConnection>("");
    BufferedConnection empty_buffered(empty_mock);
    runner.assert_throws<ConnectionClosed>([&](){ empty_buffered.read_until("\r\n"); }, "read of empty buffered conn");
    runner.assert_equal(string(""), empty_mock->written(), "buffered conn wrote to inner conn on read");

    empty_buffered.write("foo");
    runner.assert_equal(string("foo"), empty_mock->written(), "buffered conn failed first write to inner con");
    empty_buffered.write("bar");
    runner.assert_equal(string("foobar"), empty_mock->written(), "buffered conn failed second write to inner con");
    runner.assert_throws<ConnectionClosed>([&](){ empty_buffered.read_until("\r\n"); }, "read of empty buffered conn after writes");

    // test buffered reads with a variety of read buffer sizes
    int read_sizes[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100};
    for (int i = 0; i < array_size(read_sizes); i++) {
        int read_size = read_sizes[i];
        shared_ptr<MockConnection> full_mock = make_shared<MockConnection>("the big cat", read_size);
        BufferedConnection full_buffered(full_mock);
        runner.assert_equal(string("the"), full_buffered.read_until(" "), itos(read_size) + ") full mock failed first read");
        runner.assert_equal(string("big"), full_buffered.read_until(" "), itos(read_size) + ") full mock failed second read");
        runner.assert_throws<ConnectionClosed>([&](){ full_buffered.read_until(" "); }, itos(read_size) + ") full mock failed third (buffered without delim) read");
        runner.assert_throws<ConnectionClosed>([&](){ full_buffered.read_until(" "); }, itos(read_size) + ") full mock failed fourth (empty) read");
        runner.assert_throws<ConnectionClosed>([&](){ full_buffered.read_until(" "); }, itos(read_size) + ") full mock failed fifth (empty) read");
    }

    // test multicharacter separator buffered reads with a variety of read buffer sizes
    for (int i = 0; i < array_size(read_sizes); i++) {
        int read_size = read_sizes[i];
        shared_ptr<MockConnection> full_mock = make_shared<MockConnection>("halloweenabfull cat stuffabmelodyababbutter", read_size);
        BufferedConnection full_buffered(full_mock);
        runner.assert_equal(string("halloween"), full_buffered.read_until("ab"), itos(read_size) + ") full mock failed first multichar read");
        runner.assert_equal(string("full cat stuff"), full_buffered.read_until("ab"), itos(read_size) + ") full mock failed second multichar read");
        runner.assert_equal(string("melody"), full_buffered.read_until("ab"), itos(read_size) + ") full mock failed third multichar read");
        runner.assert_equal(string(""), full_buffered.read_until("ab"), itos(read_size) + ") full mock failed fourth (empty sep) read");
        runner.assert_throws<ConnectionClosed>([&](){ full_buffered.read_until("ab"); }, itos(read_size) + ") full mock failed fifth (buffered without delim) read");
        runner.assert_throws<ConnectionClosed>([&](){ full_buffered.read_until("ab"); }, itos(read_size) + ") full mock failed sixth (empty conn) read");
        runner.assert_throws<ConnectionClosed>([&](){ full_buffered.read_until("ab"); }, itos(read_size) + ") full mock failed seventh (empty conn) read");
    }
}

void test_http_connection(TestRunner& runner) {
    shared_ptr<MockConnection> mock_conn = make_shared<MockConnection>("GET /foo/bar/baz?param HTTP/0.9\r\nHost: example.com\r\nMyHeader: my_value\r\n\r\n");
    HttpConnection request_conn(mock_conn);
    HttpRequest request = request_conn.read_request();
    runner.assert_equal(string("GET"), request.method, "incorrect request http method");
    runner.assert_equal(string("/foo/bar/baz?param"), request.uri, "incorrect request uri");
    runner.assert_equal(string(HTTP_VERSION_0_9), request.version, "incorrect request http version");
    runner.assert_equal(vector<HttpHeader>{{"Host", "example.com"}, {"MyHeader", "my_value"}}, request.headers, "incorrect request headers");
    runner.assert_equal(string(""), request.body, "incorrect request body");
    runner.assert_throws<ConnectionClosed>([&](){ request_conn.read_request(); }, "read from exhausted request");
    runner.assert_equal(string(""), mock_conn->written(), "http conn wrote to socket before write was called");

    string pipelined_request_str = "GET /foo.html HTTP/1.1\r\nHost: fuz\r\n\r\nGET /bar.html HTTP/0.9\r\nHost: baz\r\n\r\n";
    shared_ptr<MockConnection> pipelined_mock_conn = make_shared<MockConnection>(pipelined_request_str);
    HttpConnection pipelined_request_conn(pipelined_mock_conn);
    HttpRequest first_request = pipelined_request_conn.read_request();
    runner.assert_equal(HttpRequest{"GET", "/foo.html", HTTP_VERSION_1_1, vector<HttpHeader>{{"Host", "fuz"}}, "", {0}}, first_request, "first pipelined request");
    HttpRequest second_request = pipelined_request_conn.read_request();
    runner.assert_equal(HttpRequest{"GET", "/bar.html", HTTP_VERSION_0_9, vector<HttpHeader>{{"Host", "baz"}}, "", {0}}, second_request, "second pipelined request");
    runner.assert_throws<ConnectionClosed>([&](){ pipelined_request_conn.read_request(); }, "third (empty) read from empty pipeline");

    HttpResponse response;
    response.version = HTTP_VERSION_1_1;
    response.status = OK_STATUS;
    response.headers = vector<HttpHeader>{{"Server", "SomeServer"}, {"SomeHeader", "some_value"}};
    response.body = "foobar baz\ndog";
    request_conn.write_response(response);
    runner.assert_equal(string("HTTP/1.1 200 OK\r\nServer: SomeServer\r\nSomeHeader: some_value\r\n\r\nfoobar baz\ndog"), mock_conn->written(), "incorrect response");

    shared_ptr<MockConnection> empty_mock_conn = make_shared<MockConnection>("");
    HttpConnection empty_request_conn(empty_mock_conn);
    runner.assert_throws<ConnectionClosed>([&](){ empty_request_conn.read_request(); }, "reading request from empty connection");

    shared_ptr<MockConnection> bad_initial_line_mock_conn = make_shared<MockConnection>("foo bar\r\n\r\n");
    HttpConnection bad_initial_line_request_conn(bad_initial_line_mock_conn);
    runner.assert_throws<HttpRequestParseError>([&]() { bad_initial_line_request_conn.read_request(); }, "bad initial line connection");

    shared_ptr<MockConnection> bad_header_mock_conn = make_shared<MockConnection>("GET / HTTP/1.1\r\nfoobar\r\n\r\n");
    HttpConnection bad_header_request_conn(bad_header_mock_conn);
    runner.assert_throws<HttpRequestParseError>([&](){ bad_header_request_conn.read_request(); }, "bad header connection");
}

void test_http_listener(TestRunner& runner) {
    HttpRequest request_1{"GET", "/foo/bar", HTTP_VERSION_1_0, vector<HttpHeader>{{"MyHeader", "myval"}}, "", {0}};
    HttpRequest request_2{"GET", "/", HTTP_VERSION_1_1, vector<HttpHeader>{{"MyHeader", "myval2"}}, "", {0}};
    HttpRequest request_3{"GET", "/foo/bar/baz", HTTP_VERSION_0_9, vector<HttpHeader>{{"MyHeader", "myval3"}}, "", {0}};

    vector<shared_ptr<MockConnection>> mock_connections = {
            make_shared<MockConnection>(request_1.pack().serialize()),
            make_shared<MockConnection>(request_2.pack().serialize()),
            make_shared<MockConnection>(request_3.pack().serialize())
    };
    shared_ptr<MockListener> mock_listener = make_shared<MockListener>(mock_connections);
    HttpListener http_listener(mock_listener);

    HttpResponse response{HTTP_VERSION_1_1, OK_STATUS, vector<HttpHeader>{HttpHeader{"KEY", "VAL"}}, ""};
    string response_str = response.pack().serialize();

    http_listener.listen();

    // chunk in blocks to guard against mistakes due to similar variable names
    {
        HttpConnection conn_1 = http_listener.accept();
        HttpRequest received_req_1 = conn_1.read_request();
        runner.assert_equal(request_1, received_req_1, "wrong first received request");
        conn_1.write_response(response);
        runner.assert_equal(response_str, mock_connections[0]->written(), "wrong first response");
    } {
        HttpConnection conn_2 = http_listener.accept();
        HttpRequest received_req_2 = conn_2.read_request();
        runner.assert_equal(request_2, received_req_2, "wrong second received request");
        conn_2.write_response(response);
        runner.assert_equal(response_str, mock_connections[1]->written(), "wrong second response");
    } {
        HttpConnection conn_3 = http_listener.accept();
        HttpRequest received_req_3 = conn_3.read_request();
        runner.assert_equal(request_3, received_req_3, "wrong third received request");
        conn_3.write_response(response);
        runner.assert_equal(response_str, mock_connections[2]->written(), "wrong third response");
    }
}

void test_http_server(TestRunner& runner) {
    HttpRequest bad_request = make_request("/foo/bar", vector<HttpHeader>{{"MyHeader", "myval2"}});
    HttpRequest request_1 = make_request("/foo/bar", vector<HttpHeader>{{"Host", "foo"}, {"MyHeader", "myval"}});
    HttpRequest request_2 = make_request("/", vector<HttpHeader>{{"MyHeader", "myval2"}, {"Host", "bar"}});
    HttpRequest request_3 = make_request("/foo/bar/baz", vector<HttpHeader>{{"Host", "foobar.com"}, {"MyHeader", "myval3"}});
    vector<shared_ptr<MockConnection>> mock_connections = {
            make_shared<MockConnection>(bad_request.pack().serialize()),
            make_shared<MockConnection>(request_1.pack().serialize()),
            make_shared<MockConnection>(request_2.pack().serialize()),
            make_shared<MockConnection>(request_3.pack().serialize()),
            make_shared<MockConnection>("foo bar\r\n\r\n")
    };
    shared_ptr<MockListener> mock_listener = make_shared<MockListener>(mock_connections);

    HttpResponse response = make_response(OK_STATUS, vector<HttpHeader>{{"OtherKey", "othervalue"}});
    shared_ptr<MockHttpRequestHandler> mock_request_handler = make_shared<MockHttpRequestHandler>(response);
    shared_ptr<HttpConnectionHandler> connection_handler = make_shared<BlockingHttpConnectionHandler>(mock_request_handler);

    HttpServer server(HttpListener(mock_listener), connection_handler);

    server.serve();

    runner.assert_equal(vector<HttpRequest>{request_1, request_2, request_3}, mock_request_handler->requests(), "handler received requests incorrectly");
    // missing host header case
    runner.assert_equal(bad_request_response().pack().serialize(), mock_connections[0]->written(), "mock bad request conn received wrong response");
    // multiple success cases
    runner.assert_equal(response.pack().serialize(), mock_connections[1]->written(), "mock conn 1 received wrong response");
    runner.assert_equal(response.pack().serialize(), mock_connections[2]->written(), "mock conn 2 received wrong response");
    runner.assert_equal(response.pack().serialize(), mock_connections[3]->written(), "mock conn 3 received wrong response");
    // malformed request case
    runner.assert_equal(bad_request_response().pack().serialize(), mock_connections[4]->written(), "mock malformed request conn received wrong response");
}

void test_pipelined_http_server(TestRunner& runner) {
    HttpRequest request_1{"GET", "/foo", HTTP_VERSION_1_0, vector<HttpHeader>{{"Host", "foo"}, {"MyHeader", "myval"}}, "", {0}};
    HttpRequest request_2{"GET", "/bar", HTTP_VERSION_1_1, vector<HttpHeader>{{"MyHeader", "myval2"}, {"Host", "bar"}}, "", {0}};
    vector<shared_ptr<MockConnection>> mock_connections = {
            make_shared<MockConnection>(request_1.pack().serialize() + request_2.pack().serialize())
    };
    shared_ptr<MockListener> mock_listener = make_shared<MockListener>(mock_connections);

    HttpResponse response{HTTP_VERSION_1_1, OK_STATUS, vector<HttpHeader>{HttpHeader{"OtherKey", "othervalue"}}, ""};
    shared_ptr<MockHttpRequestHandler> mock_request_handler = make_shared<MockHttpRequestHandler>(response);
    shared_ptr<HttpConnectionHandler> connection_handler = make_shared<BlockingHttpConnectionHandler>(mock_request_handler);

    HttpServer server(HttpListener(mock_listener), connection_handler);

    server.serve();

    // TODO: maybe make two different responses to verify sent in the correct order
    runner.assert_equal(vector<HttpRequest>{request_1, request_2}, mock_request_handler->requests(), "pipelined handler received requests incorrectly");
    runner.assert_equal(response.pack().serialize() + response.pack().serialize(), mock_connections[0]->written(), "pipelined conn received responses incorrectly");
}

void test_file_serving_handler(TestRunner& runner) {
    system_clock::time_point foo_file_time = make_time_point(2004, 1, 31, 2, 2, 2);
    unordered_map<string, shared_ptr<File>> repository_map = {
            {"/foo.html", make_shared<MockFile>(true, "foo.html contents here", foo_file_time)},
            {"/bar", make_shared<MockFile>(false, "bar contents here", system_clock::time_point())},
            {"/baz/car/tar", make_shared<MockFile>(true, "baz/car/tar contents here", system_clock::time_point())}
    };
    shared_ptr<MockFileRepository> mock_repository = make_shared<MockFileRepository>(repository_map);

    FileServingHttpHandler handler(mock_repository);

    HttpRequest foo_request = HttpRequest{"GET", "/foo.html", HTTP_VERSION_1_1, vector<HttpHeader>{}, "", {0}};
    HttpResponse foo_response = handler.handle_request(foo_request);
    runner.assert_equal(ok_response("foo.html contents here", "text/html", foo_file_time), foo_response, "wrong response for good public file");

    HttpRequest bar_request = HttpRequest{"GET", "/bar", HTTP_VERSION_1_1, vector<HttpHeader>{}, "", {0}};
    HttpResponse bar_response = handler.handle_request(bar_request);
    runner.assert_equal(forbidden_response(), bar_response, "wrong response for private file");

    HttpRequest missing_request = HttpRequest{"GET", "/missing", HTTP_VERSION_1_1, vector<HttpHeader>{}, "", {0}};
    HttpResponse missing_response = handler.handle_request(missing_request);
    runner.assert_equal(not_found_response(), missing_response, "wrong response for missing file");

    HttpRequest nested_request = HttpRequest{"GET", "/baz/car/tar", HTTP_VERSION_1_1, vector<HttpHeader>{}, "", {0}};
    HttpResponse nested_response = handler.handle_request(nested_request);
    runner.assert_equal(ok_response("baz/car/tar contents here", "text/plain", system_clock::time_point()), nested_response, "wrong response for nested file");
}

void test_cidr_block(TestRunner& runner) {
    CidrBlock root_block = parse_cidr("0.0.0.0/0");
    runner.assert_true(root_block.matches(parse_ip("0.0.0.0")), "root block matches 0.0.0.0");
    runner.assert_true(root_block.matches(parse_ip("1.2.3.4")), "root block matches 1.2.3.4");
    runner.assert_true(root_block.matches(parse_ip("255.255.255.255")), "root block matches 255.255.255");

    CidrBlock local_block = parse_cidr("192.0.0.0/8");
    runner.assert_true(local_block.matches(parse_ip("192.0.0.0")), "192.0.0.0/8 matches 192.0.0.0");
    runner.assert_true(local_block.matches(parse_ip("192.1.2.3")), "192.0.0.0/8 matches 192.1.2.3");
    runner.assert_true(local_block.matches(parse_ip("192.255.255.255")), "192.0.0.0/8 matches 192.255.255.255");
    runner.assert_false(local_block.matches(parse_ip("255.255.255.255")), "192.0.0.0/8 doesn't match 255.255.255.255");
    runner.assert_false(local_block.matches(parse_ip("0.0.0.0")), "192.0.0.0/8 doesn't match 0.0.0.0");
    runner.assert_false(local_block.matches(parse_ip("0.0.0.192")), "192.0.0.0/8 doesn't match 0.0.0.192");

    CidrBlock a_block = parse_cidr("0.0.0.0/1");
    runner.assert_true(a_block.matches(parse_ip("0.0.0.0")), "0.0.0.0/1 matches 0.0.0.0");
    runner.assert_true(a_block.matches(parse_ip("0.0.0.1")), "0.0.0.0/1 doesn't match 0.0.0.1");
    runner.assert_true(a_block.matches(parse_ip("64.0.0.0")), "0.0.0.0/1 doesn't match 64.0.0.0");
    runner.assert_true(a_block.matches(parse_ip("1.0.0.0")), "0.0.0.0/1 doesn't match 1.0.0.0");
    runner.assert_false(a_block.matches(parse_ip("192.0.0.0")), "0.0.0.0/1 doesn't match 192.0.0.0");
    runner.assert_false(a_block.matches(parse_ip("255.0.0.0")), "0.0.0.0/1 doesn't match 255.0.0.0");

    CidrBlock non_a_block = parse_cidr("128.0.0.0/1");
    runner.assert_true(non_a_block.matches(parse_ip("192.0.0.0")), "128.0.0.0/1 matches 192.0.0.0");
    runner.assert_true(non_a_block.matches(parse_ip("255.0.0.0")), "128.0.0.0/1 matches 255.0.0.0");
    runner.assert_false(non_a_block.matches(parse_ip("0.0.0.0")), "128.0.0.0/1 doesn't match 0.0.0.0");
    runner.assert_false(non_a_block.matches(parse_ip("1.0.0.0")), "128.0.0.0/1 doesn't match 1.0.0.0");
    runner.assert_false(non_a_block.matches(parse_ip("0.0.0.1")), "128.0.0.0/1 doesn't match 0.0.0.1");
    runner.assert_false(non_a_block.matches(parse_ip("64.0.0.0")), "128.0.0.0/1 doesn't match 64.0.0.0");

    CidrBlock c_block = parse_cidr("192.0.0.0/3");
    runner.assert_true(c_block.matches(parse_ip("192.0.0.0")), "192.0.0.0/3 matches 192.0.0.0");
    runner.assert_true(c_block.matches(parse_ip("192.0.0.1")), "192.0.0.0/3 matches 192.0.0.1");
    runner.assert_true(c_block.matches(parse_ip("193.0.0.1")), "192.0.0.0/3 matches 193.0.0.1");
    runner.assert_false(c_block.matches(parse_ip("0.0.0.0")), "192.0.0.0/3 doesn't match 0.0.0.0");
    runner.assert_false(c_block.matches(parse_ip("255.255.255.255")), "192.0.0.0/3 doesn't match 255.255.255.255");

    runner.assert_throws<runtime_error>([](){ parse_cidr("192.0.0.0/0"); }, "instantiating cidr block with prefix larger than length");
}

void test_htaccess_request_filter(TestRunner& runner) {
    HtAccessRequestFilter empty_filter(
            make_shared<MockFileRepository>(unordered_map<string, shared_ptr<File>>{}),
            make_shared<NopDnsClient>()
    );

    runner.assert_true(empty_filter.allow_request(make_request("/")), "empty htaccess filter /");
    runner.assert_true(empty_filter.allow_request(make_request("/foo")), "empty htaccess filter /foo");
    runner.assert_true(empty_filter.allow_request(make_request("/foo/bar")), "empty htaccess filter /foo/bar");


    unordered_map<string, shared_ptr<File>> mock_deny_files = {
            {"/foo/.htaccess", make_file("deny from 192.0.0.0/8")},
            {"/bar/.htaccess", make_file("deny from 123.4.0.0/16")}
    };
    HtAccessRequestFilter deny_filter(
            make_shared<MockFileRepository>(mock_deny_files),
            make_shared<NopDnsClient>()
    );

    runner.assert_true(deny_filter.allow_request(make_request("/", {}, parse_ip("10.0.0.1"))), "/ from 10.");
    runner.assert_true(deny_filter.allow_request(make_request("/foo.html", {}, parse_ip("10.0.0.1"))), "/foo.html from 10.");
    runner.assert_true(deny_filter.allow_request(make_request("/foo/bar.html", {}, parse_ip("10.0.0.1"))), "/foo/bar.html from 10.");
    runner.assert_true(deny_filter.allow_request(make_request("/bar/foo.html", {}, parse_ip("10.0.0.1"))), "/bar/foo.html from 10.");

    runner.assert_true(deny_filter.allow_request(make_request("/", {}, parse_ip("192.168.0.1"))), "/ from 192.");
    runner.assert_true(deny_filter.allow_request(make_request("/foo.html", {}, parse_ip("192.168.0.1"))), "/foo.html from 192.");
    runner.assert_false(deny_filter.allow_request(make_request("/foo/bar.html", {}, parse_ip("192.168.0.1"))), "/foo/bar.html from 192.");
    runner.assert_true(deny_filter.allow_request(make_request("/bar/foo.html", {}, parse_ip("192.168.0.1"))), "/bar/foo.html from 192.");

    runner.assert_true(deny_filter.allow_request(make_request("/", {}, parse_ip("123.4.0.1"))), "/ from 123.4.");
    runner.assert_true(deny_filter.allow_request(make_request("/foo.html", {}, parse_ip("123.4.0.1"))), "/foo.html from 123.4.");
    runner.assert_true(deny_filter.allow_request(make_request("/foo/bar.html", {}, parse_ip("123.4.0.1"))), "/foo/bar.html from 123.4.");
    runner.assert_false(deny_filter.allow_request(make_request("/bar/foo.html", {}, parse_ip("123.4.0.1"))), "/bar/foo.html from 123.4.");

    runner.assert_true(deny_filter.allow_request(make_request("/", {}, parse_ip("123.5.0.1"))), "/ from 123.5.");
    runner.assert_true(deny_filter.allow_request(make_request("/foo.html", {}, parse_ip("123.5.0.1"))), "/foo.html from 123.5.");
    runner.assert_true(deny_filter.allow_request(make_request("/foo/bar.html", {}, parse_ip("123.5.0.1"))), "/foo/bar.html from 123.5.");
    runner.assert_true(deny_filter.allow_request(make_request("/bar/foo.html", {}, parse_ip("123.5.0.1"))), "/bar/foo.html from 123.5.");
}

void test_request_filter_middleware(TestRunner& runner) {
    HttpResponse good_response = make_response(OK_STATUS, vector<HttpHeader>{{"SomeHeader", "someval"}}, "foobar");
    shared_ptr<RequestFilter> mock_filter = make_shared<MockRequestFilter>(vector<pair<HttpRequest,bool>>{make_pair(make_request("/foo/bar.html"), false)});
    shared_ptr<MockHttpRequestHandler> mock_handler = make_shared<MockHttpRequestHandler>(good_response);
    RequestFilterMiddleware middleware(mock_filter, mock_handler);

    runner.assert_equal(good_response, middleware.handle_request(make_request("/")), "filter middleware /");
    runner.assert_equal(good_response, middleware.handle_request(make_request("/foo.html")), "filter middleware /foo.html");
    runner.assert_equal(good_response, middleware.handle_request(make_request("/bar/bar.html")), "filter middleware /bar/bar.html");
    runner.assert_equal(good_response, middleware.handle_request(make_request("/foo/bar/baz.html")), "filter middleware /foo/bar/baz.html");

    runner.assert_equal(forbidden_response(), middleware.handle_request(make_request("/foo/bar.html")), "filter middleware /foo/bar.html");
}

typedef void (*TestFunc)(TestRunner&);

int main() {
    TestRunner runner;

    vector<TestFunc> test_funcs = {
        test_split,
        test_canonicalize_path,
        test_to_http_date,
        test_ends_with,
        test_infer_content_type,
        test_mock_connection,
        test_mock_listener,
        test_mock_handler,
        test_mock_file,
        test_mock_file_repository,
        test_mock_dns_client,
        test_buffered_connection,
        test_http_connection,
        test_http_listener,
        test_http_server,
        test_pipelined_http_server,
        test_file_serving_handler,
        test_cidr_block,
        test_htaccess_request_filter,
        test_request_filter_middleware
    };

    for (size_t i = 0; i < test_funcs.size(); i++) {
        try {
            test_funcs[i](runner);
        } catch (std::exception& e) {
            cerr << "test_funcs[" << i << "] threw exception: " << e.what() << endl;
        } catch (...) {
            cerr << "test_funcs[" << i << "] threw an unknown exception!" << endl;
        }
    }

    runner.print_results(cout);
    return 0;
}

