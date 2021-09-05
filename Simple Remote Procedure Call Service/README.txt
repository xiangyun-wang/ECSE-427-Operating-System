Simple RPC Server

Instruction to run server and client 
1. Navigate to the readme file folder in the terminal
2. Run “make clean”, and then run “make rpc”
3. Now you could run ./backend <server ip> <host> to start the server
4. Open another terminal and run ./frontend <server ip> <host> to start client

*When closing the client, please properly shut down client using “exit”, “quit” or “shutdown”.
(Better not to use “ctrl c”…) 
Otherwise, the server might not properly close when user want to shut it down. 