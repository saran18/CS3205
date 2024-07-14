P3. Multithreaded Chat Server with Timeout

Objective: Implement a multithreaded chat server with a timeout mechanism to remove inactive clients and allow chatroom interactions.

- for each client, create a separate thread
- there is also a single thread for checking the timeout status for each active client at regular intervals
- if a client is timed out, they are informed before their session is terminated. From ttheir end, they need to press any character and ENTER to terminate their end.
- if client enters an already existing username, their thread is terminated, and they have to try again with a new username