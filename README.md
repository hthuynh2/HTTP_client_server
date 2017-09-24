# HTTP_client_server
Introduction:

This project is about implementing a simple HTTP client and server. 
The client will be able to GET correctly from standard web servers, and browsers will be able to GET correctly from the server. The test setup will be two VMs, one server and one client. Each test will use client or wget, and server or thttpd.    Server program supports concurrent connections: if one client is downloading a 10MB object, another client that comes looking for a 10KB object shouldn’t have to wait for the first to finish.



Install: \n
make: to compile all programs



How to run:

Run server: ./http_server <Port>
  
Run client: ./http_client ./http_client http://hostname[:port]/path/to/file

If port is not specified, assume port 80 – the standard HTTP port.

