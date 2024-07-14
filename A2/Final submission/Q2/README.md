P2. Multithreaded HTTP Server

Objective: Implement a simple HTTP server that supports GET and POST requests, serving static web pages and handling form submissions.

- launching a new thread for each client request from the browsers.
- sending HTTP headers with appropriate content type
- sending the file contents to the client
- for the POST request, we are finding the string enclosed between 2 occurrences of "%**%" in the received POST request. This is the content entered in the text box.
- using the above data, we find the necessary counts.