#!/usr/bin/env python3
from collections import namedtuple
from http.client import HTTPResponse
import os
import random
import signal
import socket
import subprocess
from telnetlib import Telnet
import time
import unittest
from wsgiref.handlers import format_date_time

import requests
import requests.models

SLEEP_TIMEOUT = 0.5

WORLD_READABLE_FLAG = 0x4

FakeResponse = namedtuple("FakeResponse", ["status_code", "reason", "headers", "content"])

HttpStatus = namedtuple("HttpStatus", ["code", "reason"])

STATUS_OK = HttpStatus(200, "OK")
STATUS_BAD_REQUEST = HttpStatus(400, "Bad Request")
STATUS_FORBIDDEN = HttpStatus(403, "Forbidden")
STATUS_NOT_FOUND = HttpStatus(404, "Not Found")
STATUS_INTERNAL_SERVER_ERROR = HttpStatus(500, "Internal Server Error")


class HttpServerTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        self.host = "localhost"
        self.port = 6060
        self.base_path = "itest_files/"
        self.base_url = "http://" + self.host + ":" + str(self.port)
        self.default_headers = {
            'Server': 'TritonHTTP/0.1',
            'Content-Length': '0',
        }

        self.daemon = subprocess.Popen(["./httpd", str(self.port), self.base_path])
        time.sleep(SLEEP_TIMEOUT)
        if self.daemon.poll():
            raise Exception("failed to init server!")

    @classmethod
    def tearDownClass(self):
        os.kill(self.daemon.pid, signal.SIGTERM)
        self.daemon.wait()

    def get_world_readable(self, path):
        return os.stat(self.base_path + path).st_mode & WORLD_READABLE_FLAG

    def get_last_modified(self, path):
        return format_date_time(os.stat(self.base_path + path).st_mtime)

    def get_contents(self, path):
        with open(self.base_path + path, "rb") as infile:
            return infile.read()

    def assert_response(self, resp, status, headers, body):
        self.assertEqual(status.code, resp.status_code)
        self.assertEqual(status.reason, resp.reason)
        self.assertEqual(body, resp.content)
        self.assertDictEqual(headers, dict(resp.headers))

    def assert_request(self, path, status, headers, body):
        resp = requests.get(self.base_url + "/" + path, timeout=1)
        self.assert_response(resp, status, headers, body)

    def assert_good_file(self, path, content_type):
        self.assertTrue(self.get_world_readable(path))
        body = self.get_contents(path)
        headers = {
            'Server': 'TritonHTTP/0.1',
            'Content-Length': str(len(body)),
            'Content-Type': content_type,
            'Last-Modified': self.get_last_modified(path),
        }

        self.assert_request(path, STATUS_OK, headers, body)

    def assert_bad_request(self, path):
        self.assert_request(path, STATUS_BAD_REQUEST, self.default_headers, b"")

    def assert_forbidden(self, path):
        self.assert_request(path, STATUS_FORBIDDEN, self.default_headers, b"")

    def assert_forbidden_file(self, path):
        self.assertFalse(self.get_world_readable(path))
        self.assert_forbidden(path)

    def assert_not_found(self, path):
        self.assert_request(path, STATUS_NOT_FOUND, self.default_headers, b"")

    def test_empty_plain(self):
        self.assert_good_file("good_empty", "text/plain")
        self.assert_good_file("subdir/good_empty", "text/plain")

    def test_plain(self):
        self.assert_good_file("good_cat", "text/plain")
        self.assert_good_file("subdir/good_cat", "text/plain")

    def test_html(self):
        self.assert_good_file("foo.html", "text/html")
        self.assert_good_file("subdir/foo.html", "text/html")

    def test_jpeg(self):
        self.assert_good_file("cat.jpg", "image/jpeg")
        self.assert_good_file("subdir/cat.jpg", "image/jpeg")

    def test_png(self):
        self.assert_good_file("cat.png", "image/png")
        self.assert_good_file("subdir/cat.png", "image/png")

    def test_not_found(self):
        self.assert_not_found("foobar")
        self.assert_good_file("base_only", "text/plain")
        self.assert_not_found("subdir/base_only")
        self.assert_not_found("subdir_only")
        self.assert_good_file("subdir/subdir_only", "text/plain")
        self.assert_not_found("subdir/../../README.md")

    def test_forbidden(self):
        self.assert_forbidden_file("some_forbidden")
        self.assert_forbidden_file("subdir/some_forbidden")

    def test_index_html(self):
        # TODO
        pass

    def read_response(self, sock):
        response = HTTPResponse(sock)

        response.begin()
        content = response.read()
        status_code = response.status
        reason = response.reason
        headers = dict(response.getheaders())
        response.close()

        return FakeResponse(status_code, reason, headers, content)

    def make_raw_request(self, request):
        try:
            conn = Telnet(self.host, self.port)
            conn.write(request.encode("UTF-8"))
            return self.read_response(conn.get_socket())
        finally:
            conn.close()

    def test_no_host_header(self):
        resp = self.make_raw_request("GET /cat.png HTTP/1.1\r\n\r\n")
        self.assert_response(resp, STATUS_BAD_REQUEST, self.default_headers, b"")

        resp = self.make_raw_request("GET /cat.png HTTP/1.1\r\nOtherHeader: foo\r\n\r\n")
        self.assert_response(resp, STATUS_BAD_REQUEST, self.default_headers, b"")

    def test_malformed_request_line(self):
        # TODO: test more permutations of this?

        resp = self.make_raw_request("GET /cat.png\r\n\r\n")
        self.assert_response(resp, STATUS_BAD_REQUEST, self.default_headers, b"")

        resp = self.make_raw_request("GET /cat.png\r\nOtherHeader: foo\r\n\r\n")
        self.assert_response(resp, STATUS_BAD_REQUEST, self.default_headers, b"")

    def test_malformed_header(self):
        resp = self.make_raw_request("GET /cat.png\r\nOtherHeaderfoo\r\n\r\n")
        self.assert_response(resp, STATUS_BAD_REQUEST, self.default_headers, b"")

    def test_pipelined_request(self):
        two_requests = "GET /foo.html HTTP/1.1\r\nHost: bar\r\n\r\nGET /good_cat HTTP/1.1\r\nHost: baz\r\n\r\n"
        expected = (b"HTTP/1.1 200 OK\r\nServer: TritonHTTP/0.1\r\nContent-Length: 37\r\n"
                 + b"Content-Type: text/html\r\nLast-Modified: Sat, 21 Jan 2017 23:59:32 GMT\r\n\r\n"
                 + b"<h1> hi</h1>\n<p>\nthis is things\n</p>\n"
                 + b"HTTP/1.1 200 OK\r\nServer: TritonHTTP/0.1\r\nContent-Length: 5\r\n"
                 + b"Content-Type: text/plain\r\nLast-Modified: Sat, 21 Jan 2017 23:56:17 GMT\r\n\r\nmeow\n")
        try:
            conn = Telnet(self.host, self.port)
            conn.write(two_requests.encode("UTF-8"))
            responses = conn.read_until(b"kldjsflskdfjsdlkfj", timeout=SLEEP_TIMEOUT)
            self.assertEqual(expected, responses)
        finally:
            conn.close()

    def test_concurrent_request(self):
        blocker = Telnet(self.host, self.port)
        try:
            self.assert_good_file("good_cat", "text/plain")
        except requests.exceptions.ReadTimeout:
            self.fail("Unable to make a concurrent request")
        finally:
            blocker.close()


if __name__ == "__main__":
    unittest.main()
