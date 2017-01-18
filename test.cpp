#include <iostream>
#include <vector>
#include "connection.h"
#include "http.h"
#include "listener.h"
#include "mocks.h"
#include "server.h"
#include "util.h"

using namespace std;

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
    runner.assert_equal(vector<string>{""}, split("", " "), "splitting empty string");
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
    Connection* mock_conn_1 = new MockConnection("1");
    Connection* mock_conn_2 = new MockConnection("2");
    Connection* mock_conn_3 = new MockConnection("3");
    vector<Connection*> connections = {mock_conn_3, mock_conn_2, mock_conn_1};
    MockListener mock_listener(connections);

    // TODO: test accept before listen?
    mock_listener.listen();

    runner.assert_equal(mock_conn_1, mock_listener.accept(), "wrong first accepted conn");
    runner.assert_equal(mock_conn_2, mock_listener.accept(), "wrong second accepted conn");
    runner.assert_equal(mock_conn_3, mock_listener.accept(), "wrong third accepted conn");

    // TODO: test listen after exhausted

    delete mock_conn_1;
    delete mock_conn_2;
    delete mock_conn_3;
}

void test_mock_handler(TestRunner& runner) {
    HttpResponse response = HttpResponse{HTTP_VERSION_1_1, OK_STATUS, vector<HttpHeader>{HttpHeader{"OtherKey", "othervalue"}}, ""};
    MockHttpHandler mock_handler(response);

    vector<HttpRequest> requests = {
        {"GET", "/foo/bar", HTTP_VERSION_1_1, vector<HttpHeader>{HttpHeader{"SomeKey", "somevalue"}}, ""},
        {"PUT", "/", HTTP_VERSION_0_9, vector<HttpHeader>{}, ""},
        {"GET", "/baz", HTTP_VERSION_1_0, vector<HttpHeader>{HttpHeader{"SomeOtherKey", "othersomevalue"}, HttpHeader{"foo", "bar"}}, ""}
    };

    for (auto& request : requests) {
        HttpResponse received_response = mock_handler.handle_request(request);
        runner.assert_equal(request, mock_handler.request(), "stored request incorrect");
        runner.assert_equal(response, received_response, "received response incorrect");
    }
}

void test_buffered_connection(TestRunner& runner) {
    MockConnection* empty_mock = new MockConnection("");
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
        MockConnection* full_mock = new MockConnection("the big cat", read_size);
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
        MockConnection* full_mock = new MockConnection("halloweenabfull cat stuffabmelodyababbutter", read_size);
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
    MockConnection* mock_conn = new MockConnection("GET /foo/bar/baz?param HTTP/0.9\r\nHost: example.com\r\nMyHeader: my_value\r\n\r\n");
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

    MockConnection* empty_mock_conn = new MockConnection("");
    HttpConnection empty_request_conn(empty_mock_conn);
    try {
        empty_request_conn.read_request();
        runner.fail("failed to raise exception with empty connection");
    } catch (HttpRequestParseError&) {
        runner.pass();
    }

    MockConnection* bad_initial_line_mock_conn = new MockConnection("foo bar\r\n\r\n");
    HttpConnection bad_initial_line_request_conn(bad_initial_line_mock_conn);
    try {
        bad_initial_line_request_conn.read_request();
        runner.fail("failed to raise exception with bad initial line connection");
    } catch (HttpRequestParseError&) {
        runner.pass();
    }

    MockConnection* bad_header_mock_conn = new MockConnection("GET / HTTP/1.1\r\nfoobar\r\n\r\n");
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

    MockConnection* mock_conn_1 = new MockConnection(request_1.pack().serialize());
    MockConnection* mock_conn_2 = new MockConnection(request_2.pack().serialize());
    MockConnection* mock_conn_3 = new MockConnection(request_3.pack().serialize());
    MockListener* mock_listener = new MockListener(vector<Connection*> {mock_conn_3, mock_conn_2, mock_conn_1});
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
        runner.assert_equal(response_str, mock_conn_1->written(), "wrong first response");
    } {
        HttpConnection conn_2 = http_listener.accept();
        HttpRequest received_req_2 = conn_2.read_request();
        runner.assert_equal(request_2, received_req_2, "wrong second received request");
        conn_2.write_response(response);
        runner.assert_equal(response_str, mock_conn_2->written(), "wrong second response");
    } {
        HttpConnection conn_3 = http_listener.accept();
        HttpRequest received_req_3 = conn_3.read_request();
        runner.assert_equal(request_3, received_req_3, "wrong third received request");
        conn_3.write_response(response);
        runner.assert_equal(response_str, mock_conn_3->written(), "wrong third response");
    } {
        // TODO: test accept when no more connections are remaining?
    }
}

void test_http_server(TestRunner&) {
    HttpRequest request_1{"GET", "/foo/bar", HTTP_VERSION_1_0, vector<HttpHeader>{{"MyHeader", "myval"}}, ""};
    HttpRequest request_2{"GET", "/", HTTP_VERSION_1_1, vector<HttpHeader>{{"MyHeader", "myval2"}}, ""};
    HttpRequest request_3{"GET", "/foo/bar/baz", HTTP_VERSION_0_9, vector<HttpHeader>{{"MyHeader", "myval3"}}, ""};
    MockConnection* mock_conn_1 = new MockConnection(request_1.pack().serialize());
    MockConnection* mock_conn_2 = new MockConnection(request_2.pack().serialize());
    MockConnection* mock_conn_3 = new MockConnection(request_3.pack().serialize());
    MockListener* mock_listener = new MockListener(vector<Connection*> {mock_conn_3, mock_conn_2, mock_conn_1});

    HttpResponse response{HTTP_VERSION_1_1, OK_STATUS, vector<HttpHeader>{HttpHeader{"OtherKey", "othervalue"}}, ""};
    HttpHandler* mock_handler = new MockHttpHandler(response);

    HttpServer server(HttpListener(mock_listener), mock_handler);

    // TODO: implement halting of serve() so that assertions can be performed afterwards
    // server.serve();
}

int main() {
    TestRunner runner;

    test_split(runner);
    test_mock_connection(runner);
    test_mock_listener(runner);
    test_mock_handler(runner);
    test_buffered_connection(runner);
    test_http_connection(runner);
    test_http_listener(runner);
    test_http_server(runner);

    runner.print_results(cout);
    return 0;
}

