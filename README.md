# Retransmission-Congestion-control
- This is the project of Computer Network 2022 of NTU.
- In this project, I implement Go-Back-N protocol with congestion control. 
There are 3 components in this project: sender, receiver and agent.
## Role Explaination
1. The agent's job is to forward the data, however, it will randomly drop or corrupt the data packet to simulate the real Internet's behavior. 
2. The receiver has a buffer to store the data, it will only flush the buffer when the buffer is full or all data are received. The data will be droped if the buffer is full.
3. The sender will perform TCP congestion control algorithm, it will do the slow start until it reaches the threshold, 
then reset the window size to 1 and update the threshold after packet loss or time out.
## Compile
To compile the code, go to terminal and run the following code.
```
make sender // To compile sender code
make agent // To compile agent code
make receiver // To compile receiver code
```

## Execute
To execute the code, run the following commands in different terminals and in sequence.
```
./agent <agent port> <sender IP>:<sender port> <receiver IP>:<receiver port> <error rate>
./receiver <receiver port> <agent IP>:<agent port>
./sender <sender port> <agent IP>:<agent port> <.mpg filename>
```

