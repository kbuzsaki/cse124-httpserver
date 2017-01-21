#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "connection.h"
#include "http.h"
#include "handlers.h"
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

    try {
        make_time_point(1969, 12, 31, 23, 59, 59);
        runner.fail("make time point before epoch");
    } catch (runtime_error&) {
        runner.pass();
    }
}

void test_ends_with(TestRunner& runner) {
    runner.assert_equal(true, ends_with("", ""), "empty string ends with empty string");
    runner.assert_equal(true, ends_with("foo.html", ".html"), "foo.html ends with .html");
    runner.assert_equal(false, ends_with("bar.html.bak", ".html"), "bar.html.bak does not end with .html");
    runner.assert_equal(false, ends_with("bar.html.bak", "foo.bar.html.bak"), "bar.html.bak does not end with foo.bar.html.bak");
}

void test_mock_connection(TestRunner& runner) {
    MockConnection empty_mock("");
    runner.assert_equal(string(""), empty_mock.read(), "empty mock fails to read empty string");
    runner.assert_equal(string(""), empty_mock.read(), "empty mock fails to read empty string with repeated calls to read");
    runner.assert_equal(string(""), empty_mock.read(), "empty mock fails to read empty string with repeated calls to read");

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
    runner.assert_equal(string(""), full_mock.read(), "failed read of exhausted mock");
}

void test_mock_listener(TestRunner& runner) {
    vector<shared_ptr<Connection>> connections = {
            make_shared<MockConnection>("1"),
            make_shared<MockConnection>("2"),
            make_shared<MockConnection>("3")
    };
    MockListener mock_listener(connections);

    // TODO: test accept before listen?
    mock_listener.listen();

    runner.assert_equal(connections[0], mock_listener.accept(), "wrong first accepted conn");
    runner.assert_equal(connections[1], mock_listener.accept(), "wrong second accepted conn");
    runner.assert_equal(connections[2], mock_listener.accept(), "wrong third accepted conn");

    // TODO: test listen after exhausted
}

void test_mock_handler(TestRunner& runner) {
    HttpResponse response = HttpResponse{HTTP_VERSION_1_1, OK_STATUS, vector<HttpHeader>{HttpHeader{"OtherKey", "othervalue"}}, ""};
    MockHttpHandler mock_handler(response);

    vector<HttpRequest> requests = {
        {"GET", "/foo/bar", HTTP_VERSION_1_1, vector<HttpHeader>{HttpHeader{"SomeKey", "somevalue"}}, ""},
        {"PUT", "/", HTTP_VERSION_0_9, vector<HttpHeader>{}, ""},
        {"GET", "/baz", HTTP_VERSION_1_0, vector<HttpHeader>{HttpHeader{"SomeOtherKey", "othersomevalue"}, HttpHeader{"foo", "bar"}}, ""}
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

    // TODO: should a private file not have any contents?
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

void test_buffered_connection(TestRunner& runner) {
    shared_ptr<MockConnection> empty_mock = make_shared<MockConnection>("");
    BufferedConnection empty_buffered(empty_mock);
    runner.assert_equal(string(""), empty_buffered.read_until("\r\n"), "empty mock fails to read empty string");
    runner.assert_equal(string(""), empty_buffered.read_until("\r\n"), "empty mock fails to read empty string");
    runner.assert_equal(string(""), empty_buffered.read_until("\r\n"), "empty mock fails to read empty string");
    runner.assert_equal(string(""), empty_mock->written(), "buffered conn wrote to inner conn on read");

    empty_buffered.write("foo");
    runner.assert_equal(string("foo"), empty_mock->written(), "buffered conn failed first write to inner con");
    empty_buffered.write("bar");
    runner.assert_equal(string("foobar"), empty_mock->written(), "buffered conn failed second write to inner con");
    runner.assert_equal(string(""), empty_buffered.read_until("\r\n"), "empty mock fails to read empty string after writes");

    // test buffered reads with a variety of read buffer sizes
    int read_sizes[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100};
    for (int i = 0; i < array_size(read_sizes); i++) {
        int read_size = read_sizes[i];
        shared_ptr<MockConnection> full_mock = make_shared<MockConnection>("the big cat", read_size);
        BufferedConnection full_buffered(full_mock);
        runner.assert_equal(string("the"), full_buffered.read_until(" "), itos(read_size) + ") full mock failed first read");
        runner.assert_equal(string("big"), full_buffered.read_until(" "), itos(read_size) + ") full mock failed second read");
        runner.assert_equal(string("cat"), full_buffered.read_until(" "), itos(read_size) + ") full mock failed third read");
        runner.assert_equal(string(""), full_buffered.read_until(" "), itos(read_size) + ") full mock failed fourth (empty) read");
        runner.assert_equal(string(""), full_buffered.read_until(" "), itos(read_size) + ") full mock failed fifth (empty) read");
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
        runner.assert_equal(string("butter"), full_buffered.read_until("ab"), itos(read_size) + ") full mock failed fifth read");
        runner.assert_equal(string(""), full_buffered.read_until("ab"), itos(read_size) + ") full mock failed sixth (empty conn) read");
        runner.assert_equal(string(""), full_buffered.read_until("ab"), itos(read_size) + ") full mock failed seventh (empty conn) read");
    }
}

// TODO: test failure cases
void test_http_connection(TestRunner& runner) {
    shared_ptr<MockConnection> mock_conn = make_shared<MockConnection>("GET /foo/bar/baz?param HTTP/0.9\r\nHost: example.com\r\nMyHeader: my_value\r\n\r\n");
    HttpConnection request_conn(mock_conn);
    HttpRequest request = request_conn.read_request();
    runner.assert_equal(string("GET"), request.method, "incorrect request http method");
    runner.assert_equal(string("/foo/bar/baz?param"), request.uri, "incorrect request uri");
    runner.assert_equal(string(HTTP_VERSION_0_9), request.version, "incorrect request http version");
    runner.assert_equal(vector<HttpHeader>{{"Host", "example.com"}, {"MyHeader", "my_value"}}, request.headers, "incorrect request headers");
    runner.assert_equal(string(""), request.body, "incorrect request body");
    runner.assert_equal(string(""), mock_conn->written(), "http conn wrote to socket before write was called");

    // TODO: test body with \r\n sequence
    HttpResponse response;
    response.version = HTTP_VERSION_1_1;
    response.status = OK_STATUS;
    response.headers = vector<HttpHeader>{{"Server", "SomeServer"}, {"SomeHeader", "some_value"}};
    response.body = "foobar baz\ndog";
    request_conn.write_response(response);
    runner.assert_equal(string("HTTP/1.1 200 OK\r\nServer: SomeServer\r\nSomeHeader: some_value\r\n\r\nfoobar baz\ndog"), mock_conn->written(), "incorrect response");

    shared_ptr<MockConnection> empty_mock_conn = make_shared<MockConnection>("");
    HttpConnection empty_request_conn(empty_mock_conn);
    try {
        empty_request_conn.read_request();
        runner.fail("failed to raise exception with empty connection");
    } catch (HttpRequestParseError&) {
        runner.pass();
    }

    shared_ptr<MockConnection> bad_initial_line_mock_conn = make_shared<MockConnection>("foo bar\r\n\r\n");
    HttpConnection bad_initial_line_request_conn(bad_initial_line_mock_conn);
    try {
        bad_initial_line_request_conn.read_request();
        runner.fail("failed to raise exception with bad initial line connection");
    } catch (HttpRequestParseError&) {
        runner.pass();
    }

    shared_ptr<MockConnection> bad_header_mock_conn = make_shared<MockConnection>("GET / HTTP/1.1\r\nfoobar\r\n\r\n");
    HttpConnection bad_header_request_conn(bad_header_mock_conn);
    try {
        bad_header_request_conn.read_request();
        runner.fail("failed to raise exception with bad header connection");
    } catch (HttpRequestParseError&) {
        runner.pass();
    }
}

void test_http_listener(TestRunner& runner) {
    HttpRequest request_1{"GET", "/foo/bar", HTTP_VERSION_1_0, vector<HttpHeader>{{"MyHeader", "myval"}}, ""};
    HttpRequest request_2{"GET", "/", HTTP_VERSION_1_1, vector<HttpHeader>{{"MyHeader", "myval2"}}, ""};
    HttpRequest request_3{"GET", "/foo/bar/baz", HTTP_VERSION_0_9, vector<HttpHeader>{{"MyHeader", "myval3"}}, ""};

    vector<shared_ptr<MockConnection>> mock_connections = {
            make_shared<MockConnection>(request_1.pack().serialize()),
            make_shared<MockConnection>(request_2.pack().serialize()),
            make_shared<MockConnection>(request_3.pack().serialize())
    };
    shared_ptr<MockListener> mock_listener = make_shared<MockListener>(mock_connections);
    HttpListener http_listener(mock_listener);

    HttpResponse response{HTTP_VERSION_1_1, OK_STATUS, vector<HttpHeader>{HttpHeader{"KEY", "VAL"}}, ""};
    string response_str = response.pack().serialize();

    // TODO: test failure before listen?
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
    } {
        // TODO: test accept when no more connections are remaining?
    }
}

void test_http_server(TestRunner& runner) {
    HttpRequest bad_request{"GET", "/foo/bar", HTTP_VERSION_1_1, vector<HttpHeader>{{"MyHeader", "myval2"}}, ""};
    HttpRequest request_1{"GET", "/foo/bar", HTTP_VERSION_1_0, vector<HttpHeader>{{"Host", "foo"}, {"MyHeader", "myval"}}, ""};
    HttpRequest request_2{"GET", "/", HTTP_VERSION_1_1, vector<HttpHeader>{{"MyHeader", "myval2"}, {"Host", "bar"}}, ""};
    HttpRequest request_3{"GET", "/foo/bar/baz", HTTP_VERSION_0_9, vector<HttpHeader>{{"Host", "foobar.com"}, {"MyHeader", "myval3"}}, ""};
    vector<shared_ptr<MockConnection>> mock_connections = {
            make_shared<MockConnection>(bad_request.pack().serialize()),
            make_shared<MockConnection>(request_1.pack().serialize()),
            make_shared<MockConnection>(request_2.pack().serialize()),
            make_shared<MockConnection>(request_3.pack().serialize()),
            make_shared<MockConnection>("foo bar")
    };
    shared_ptr<MockListener> mock_listener = make_shared<MockListener>(mock_connections);

    HttpResponse response{HTTP_VERSION_1_1, OK_STATUS, vector<HttpHeader>{HttpHeader{"OtherKey", "othervalue"}}, ""};
    shared_ptr<MockHttpHandler> mock_handler = make_shared<MockHttpHandler>(response);

    HttpServer server(HttpListener(mock_listener), mock_handler);

    server.serve();

    runner.assert_equal(vector<HttpRequest>{request_1, request_2, request_3}, mock_handler->requests(), "handler received requests incorrectly");
    // missing host header case
    runner.assert_equal(bad_request_response().pack().serialize(), mock_connections[0]->written(), "mock bad request conn received wrong response");
    // multiple success cases
    runner.assert_equal(response.pack().serialize(), mock_connections[1]->written(), "mock conn 1 received wrong response");
    runner.assert_equal(response.pack().serialize(), mock_connections[2]->written(), "mock conn 2 received wrong response");
    runner.assert_equal(response.pack().serialize(), mock_connections[3]->written(), "mock conn 3 received wrong response");
    // malformed request case
    runner.assert_equal(bad_request_response().pack().serialize(), mock_connections[4]->written(), "mock malformed request conn received wrong response");
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

    HttpRequest foo_request = HttpRequest{"GET", "/foo.html", HTTP_VERSION_1_1, vector<HttpHeader>{}, ""};
    HttpResponse foo_response = handler.handle_request(foo_request);
    runner.assert_equal(ok_response("foo.html contents here", "text/html", foo_file_time), foo_response, "wrong response for good public file");

    HttpRequest bar_request = HttpRequest{"GET", "/bar", HTTP_VERSION_1_1, vector<HttpHeader>{}, ""};
    HttpResponse bar_response = handler.handle_request(bar_request);
    runner.assert_equal(forbidden_response(), bar_response, "wrong response for private file");

    HttpRequest missing_request = HttpRequest{"GET", "/missing", HTTP_VERSION_1_1, vector<HttpHeader>{}, ""};
    HttpResponse missing_response = handler.handle_request(missing_request);
    runner.assert_equal(not_found_response(), missing_response, "wrong response for missing file");

    HttpRequest nested_request = HttpRequest{"GET", "/baz/car/tar", HTTP_VERSION_1_1, vector<HttpHeader>{}, ""};
    HttpResponse nested_response = handler.handle_request(nested_request);
    runner.assert_equal(ok_response("baz/car/tar contents here", "text/plain", system_clock::time_point()), nested_response, "wrong response for nested file");
}

int main() {
    TestRunner runner;

    test_split(runner);
    test_canonicalize_path(runner);
    test_to_http_date(runner);
    test_ends_with(runner);
    test_mock_connection(runner);
    test_mock_listener(runner);
    test_mock_handler(runner);
    test_mock_file(runner);
    test_mock_file_repository(runner);
    test_buffered_connection(runner);
    test_http_connection(runner);
    test_http_listener(runner);
    test_http_server(runner);
    test_file_serving_handler(runner);

    runner.print_results(cout);
    return 0;
}

