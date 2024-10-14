### MINI PROJECT-2
## PART-A[XV6]
# SYSTEMCALLS
# 1 "GETSYSCOUNT" SYSTEMCALL

The `getSysCount` system call allows users to track how many times a specific system call has been invoked by a process (and its child processes) during its execution. The system call takes a bitmask as an argument, where each bit represents a different system call. The user program `syscount` leverages this system call to monitor system calls made by another program during its runtime.

## Files Changed

1. **kernel/syscall.h**: Added the `SYS_getSysCount` system call number.
2. **kernel/syscall.c**: Declared the new system call and updated the `syscalls[]` array.
3. **kernel/sysproc.c**: Implemented the `sys_getSysCount` function that returns the count of a specific system call based on the bitmask.
4. **kernel/proc.h**: Modified the `proc` structure to include an array for tracking system call counts.
5. **kernel/proc.c**: Initialized the syscall count array in the `allocproc()` function.
6. **kernel/syscall.c**: Updated the `syscall()` function to increment the count whenever a system call is made.
7. **user/syscount.c**: Created a user program that forks a process, runs a command, and reports the number of times a specified system call was invoked.
8. **Makefile**: Updated to include the `syscount` program in the list of compiled user programs.
9. **user/user.h**: Declared the new `getSysCount` system call for use in user programs.
10. **user/usys.pl**: Added an entry for `getSysCount` to make it accessible from user space.

## System Call: `getSysCount`

The `getSysCount` system call allows users to monitor how often a particular system call is executed by a process. It accepts a bitmask as its argument, where each bit represents a system call. For example:
- Bit 0: `fork`
- Bit 1: `exit`
- Bit 2: `wait`
- ...
- Bit 21: `getSysCount`

The system call returns the number of times the corresponding system call has been invoked by the current process or its children.

### Prototype
```c
int getSysCount(int mask);
```

- **mask**: A bitmask where each bit represents a system call. Only the first bit set in the mask is considered.
- **Returns**: The number of times the corresponding system call was invoked.

### 5.Conclusion
With the `getSysCount` system call and the `syscount` program, users can monitor the system calls made by processes, enabling better debugging, profiling, and performance analysis in xv6.


##  `sigalarm` System Call Implementation

## Overview
This implementation introduces two new system calls in xv6: `sigalarm` and `sigreturn`. These system calls allow a process to receive periodic alerts based on the CPU time used. The process can specify a handler function that will be executed when the alarm triggers. After the handler is executed, the process can resume its normal execution.

### System Calls:
- **sigalarm(int ticks, void (*handler)())**:
  - Sets up an alarm to go off after the specified number of clock ticks (`ticks`). When the alarm goes off, the specified handler function (`handler`) is executed.
  - If `ticks` is set to 0, the alarm is disabled.
  
- **sigreturn()**:
  - Restores the process's state after the alarm handler is executed and resumes normal process execution.

## Key Files Modified
### 1. `kernel/proc.h`
- Added the following fields to the `proc` structure:
  - `alarm_interval`: The interval for the alarm in clock ticks.
  - `alarm_handler`: A pointer to the alarm handler function.
  - `ticks_count`: Counter to track ticks since the last alarm.
  - `alarm_on`: Flag to indicate if the alarm is enabled.
  - `alarm_tf`: Stores the process's trapframe when the handler is called.

### 2. `kernel/trap.c`
- Updated `usertrap` to check if the alarm is enabled and, if so, increment the `ticks_count`. Once the interval is reached, the trapframe is saved, and the process switches to execute the handler.

### 3. `kernel/sysproc.c`
- `sys_sigalarm`: Sets the alarm interval, handler, and initializes the alarm state.
- `sys_sigreturn`: Restores the process state (trapframe) after the handler is executed and resets the tick counter.

### 4. `user/user.h`
- Added the declarations for the system calls `sigalarm` and `sigreturn`.

### 5. `user/usys.pl`
- Added entries for the `sigalarm` and `sigreturn` system calls.

### 6. `kernel/syscall.h` & `kernel/syscall.c`
- Added system call numbers and corresponding function pointers for `sigalarm` and `sigreturn`.

### CONCLUSION:
This implementation allows a process to receive periodic alerts based on CPU time and handle the alert with custom logic while ensuring proper process state restoration.

### SCHEDULING :
## LBS[Lottery Based System]:

This project implements a lottery-based scheduling algorithm for xv6, where processes are assigned a number of tickets, and the process that gets scheduled is chosen randomly based on the ticket count.

## Features

- **Ticket Allocation:** Each process can have a different number of tickets. More tickets increase the chances of getting CPU time.
- **Ticket Inheritance:** When a process forks, the child inherits the parent's tickets.
- **Randomized Scheduling:** Processes are chosen based on a random lottery system.
- **System Call:** A `settickets()` system call allows a user to set the number of tickets for a process.

## Steps to Implement

### 1. Modify the Makefile
Enable switching between round-robin (RR) and lottery-based scheduling (LBS):

```makefile
ifndef SCHEDULER
SCHEDULER := RR
endif

CFLAGS += -D$(SCHEDULER)
```

### 2. Update `proc.h`
Add ticket and creation time fields to the `proc` structure:

```c
struct proc {
  // ... existing fields ...
  int tickets;           // Number of tickets for lottery scheduling
  uint creation_time;     // Process creation time (for tie-breaking)
};
```

### 3. Modify `allocproc()` in `proc.c`
Set default tickets and track the process's creation time:

```c
p->tickets = 1;           // Default to 1 ticket
p->creation_time = ticks;  // Store the creation time
```

Add this system call to `syscall.c`, `syscall.h`, and `sysproc.c`.

### 5. Modify `scheduler()` Function in `proc.c`
Implement lottery scheduling logic:

```c
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    intr_on();

#ifdef LBS
    // LBS Scheduler code to be implemented here
#else
    // Default round-robin scheduler
    
#endif
  }
}
```

### 6. Add Random Number Generator
Include a simple random number generator:
```c
static unsigned long rand_seed = 1;

static unsigned int rand(void) {
  rand_seed = rand_seed * 1664525 + 1013904223;
  return rand_seed;
}
```
### 8. Compile with Lottery Scheduling
To enable lottery scheduling, run:
```bash
make qemu SCHEDULER=LBS 
```
## Testing
Run the schedulertest command in the xv6 to get average runtime(rtime)and average waittime(wtime).
## Conclusion
This simple lottery-based scheduler allocates CPU time based on process tickets, providing a probabilistic but fair scheduling mechanism. The default behavior is round-robin, and you can switch to lottery scheduling using the `SCHEDULER` macro.
## Performance Comparision:
### Performance Comparison: Round Robin vs. Lottery-Based Scheduling (LBS)

- **Round Robin (RR):** RR provides a uniform time slice to each process, ensuring fairness regardless of process behavior or priority. It is simple and predictable, but it doesn't account for process priority or varying needs for CPU time. RR can lead to inefficiencies when high-priority tasks must wait for their turn, potentially causing delays in system responsiveness.

- **Lottery-Based Scheduling (LBS):** LBS is a priority-driven scheduler, where the number of tickets determines the likelihood of a process being scheduled. Processes with more tickets (higher priority) get more CPU time. This improves performance for important or CPU-intensive tasks, while still offering a chance for low-priority tasks to run. However, randomness can introduce variability in response times, and overhead increases with more frequent random number generation and ticket calculations.

- **In Summary:** RR is better suited for simple, equal-priority workloads with minimal overhead, while LBS offers flexible prioritization, improving performance for high-priority tasks but with added complexity and potential scheduling variability.

### Implication of Adding Arrival Time in Lottery-Based Scheduling
Including arrival time in lottery scheduling ensures that when two processes have the same number of tickets, the one that arrived earlier is given priority. This helps prevent newer processes from unfairly monopolizing CPU time over older processes with the same ticket count. However, a pitfall is that processes arriving early could dominate the CPU if the system frequently has processes with equal tickets, potentially causing older processes to receive excessive priority.

### What Happens if All Processes Have the Same Number of Tickets?
If all processes have the same number of tickets, the lottery becomes purely random, and the selection is based solely on chance. This can lead to uneven scheduling where some processes may be selected more frequently than others in the short term, despite all having equal priority. Over time, the distribution should even out, but short-term fairness may suffer, leading to inconsistent performance for some processes.
### Pitfalls to Watch Out for in Lottery-Based Scheduling

1. **Starvation Risk:** If processes with many tickets dominate the lottery, low-ticket processes may get very few chances to run, leading to starvation. Over time, this can reduce fairness in the system.
   
2. **Randomness Variability:** Since the scheduling relies on random number generation, short-term variability may cause unpredictable performance, with some processes running more often than others by chance, even if they have similar ticket counts.

3. **Overhead:** Calculating the total tickets and running the lottery can introduce scheduling overhead, especially in systems with many processes.

4. **Priority Inversion:** High-priority tasks could be delayed if they receive fewer random selections than expected, potentially impacting real-time applications or critical tasks.

# Balancing ticket distribution and managing randomness are key to minimizing these pitfalls.

# xv6 MLFQ Scheduler with Priority Boosting

This project implements a simplified Multilevel Feedback Queue (MLFQ) scheduler in the xv6 operating system. The MLFQ scheduler allows for dynamic process prioritization based on CPU time usage, featuring multiple priority queues and a priority boosting mechanism to prevent process starvation.

## Key Features

- **Four Priority Queues**: The scheduler uses four priority levels, with processes starting at the highest priority and potentially moving to lower priorities if they consume their time slice.
  
- **Dynamic Time Slices**: Each queue has an associated time slice, which increases exponentially as the priority decreases (i.e., `1 << priority`).
  
- **Priority Boosting**: To prevent starvation, all processes are periodically boosted to the highest priority queue. This occurs every 48 ticks (`BOOST_TICKS`).

- **Round-Robin Scheduling**: Processes within the same priority level are scheduled in a round-robin fashion using a queue for each priority.

## Files Modified
- `proc.c`: Major modifications for initializing and managing the MLFQ structure, updating process states, and implementing scheduling behavior.
- `trap.c`: Modifications to handle timer interrupts for updating process CPU usage and enforcing time slice limits.
- `proc.h`: Process structures modified to include `priority`, `time_slice`, and `total_ticks` fields for each process.
- `defs.h`: Function prototypes for the new `enqueue`, `dequeue`, and MLFQ-related functions.
  
## System Constants
- `NUM_QUEUES`: Defines the number of priority queues (4 in this case).
- `BOOST_TICKS`: Defines how often priority boosting occurs (every 48 ticks).

## How It Works

### Process Creation and Initialization

- When a new process is created (`allocproc`), it is assigned the highest priority (priority 0) and a time slice of 1 tick.
- The first user process (`userinit`) is enqueued in the highest priority queue, and subsequent processes are enqueued as they become runnable.

### Scheduler Function

- The `scheduler` function selects the highest-priority process that is ready to run by checking the queues from the highest to the lowest priority.
- When a process runs, its `total_ticks` is incremented, and if it exhausts its time slice, it is moved to a lower priority queue with an updated time slice.
- After every `BOOST_TICKS` (48 ticks), all processes are moved back to the highest priority queue to prevent long-term starvation of lower-priority processes.

### Yield and Timer Interrupts

- In the `yield` function, the process' `total_ticks` is incremented, and it is re-enqueued based on its updated priority and time slice.
- Timer interrupts check if the running process has exhausted its time slice. If so, the `yield` function is invoked, moving the process to the appropriate priority queue.

### MLFQ Queue Management

- `enqueue(int priority, struct proc *p)`: Adds a process to the tail of the specified priority queue.
- `dequeue(int priority)`: Removes and returns a process from the head of the specified priority queue.

### Boosting Mechanism

- Every 48 ticks (`BOOST_TICKS`), all runnable processes are boosted to the highest priority queue. Their time slices are reset accordingly.

## Compilation and Execution

1. **Rebuild xv6**: After implementing the changes, you can rebuild the kernel using:
   ```bash
   make clean && make qemu SCHEDULING=MLFQ
   ```

2. **Testing the MLFQ**: 
    Run the schedulertest command in the xv6 to get average runtime(rtime)and average waittime(wtime).
## Code Overview

- **proc.c**:
  - MLFQ data structure and queue operations (`enqueue`, `dequeue`).
  - Modified `procinit` to initialize MLFQ queues.
  - Modified `allocproc` to assign default priorities and time slices to processes.
  - Modified `scheduler` to implement the MLFQ logic with priority boosting.
  - Updated `yield` to manage time slices and re-enqueue processes.
  
- **trap.c**:
  - Timer interrupt handling to enforce time slices and trigger the scheduler when needed.
CPU=1
|         RR         |       LBS          |       MLFQ          |
|--------------------|--------------------|---------------------|
| rtime=20,wtime=185 | rtime=20,wtime=156 | rtime=20,wtime= 113 |
| rtime=20,wtime=180 | rtime=21,wtime=157 | rtime=20,wtime=119  |
CPU=3
|        RR          |         LBS         | 
|--------------------|---------------------|
| rtime=27,wtime=71  |  rtime=11,wtime=67  |
| rtime=28,wtime=72  |  rtime=16,wtime=79  | 
## PART-B[NETWORKING]
# Tic-Tac-Toe Multiplayer Game
This is a simple multiplayer Tic-Tac-Toe game implemented with networking concepts. The game includes a server to manage the game state and two clients (players) who compete against each other.
## Overview
The game can be played using either TCP or UDP sockets. The server handles communication between the two players, manages the game board, and determines the winner or if the game ends in a draw.
### Server
- Manages the game state (the 3x3 Tic-Tac-Toe board).
- Ensures valid moves and alternates turns between players.
- Determines the game outcome (win or draw).
- Handles reconnection requests and game restarts.
### Clients
- Two clients connect to the server (Player 1 and Player 2).
- Players send their moves (row and column numbers) to the server.
- Receive the updated game board from the server after every move.
## Game Rules
- **Board:** A 3x3 grid with indexing {0,1,2}.
- **Players:** Player 1 uses 'X', Player 2 uses 'O'.
- **Moves:** Players take turns entering their moves by specifying row and column numbers (e.g., `1 1` for top-left).
- **Winning:** A player wins by placing three of their symbols in a row, column, or diagonal.
- **Draw:** If the board is full and no one wins, the game ends in a draw.
- **Post-Game:** After a win or a draw, players are asked if they want to play again. If both agree, the board resets and a new game starts. Otherwise, the game ends.
## Features
- **Game Flow:** The server alternates between players and ensures valid moves.
- **Win/Draw Detection:** The server checks for winning conditions or if the game ends in a draw after each move.
- **Restart:** Players can choose to play again after a game ends.
## How to Play
1. Once both clients are connected, the server sends the initial empty game board.
2. The server assigns Player 1 to use 'X' and Player 2 to use 'O'.
3. Players take turns entering their moves by specifying the row and column numbers (e.g., `0 2` for top-left corner).
4. The server validates the move, updates the game board, and sends the updated board to both clients.
5. The game continues until one player wins or the board is full (draw).
6. At the end of the game, players are prompted to decide if they want to play again.

# TCP Functionality over UDP
This project implements basic TCP-like functionality using UDP sockets, simulating data sequencing and reliable transmission through acknowledgments and retransmissions. It demonstrates how to handle data sequencing and loss recovery using UDP, which does not natively support such features.
## Features
### 1. Data Sequencing:
- **Sender** splits a message (text) into smaller chunks.
- Each chunk is assigned a sequence number, and the total number of chunks is sent along with the data.
- **Receiver** reassembles the chunks based on sequence numbers to form the complete message.
### 2. Retransmissions:
- After receiving a chunk, the receiver sends an **ACK** (Acknowledgment) message with the corresponding sequence number.
- If the sender doesn’t receive an ACK within a timeout (e.g., 0.1 seconds), it retransmits the missing chunk.
- The sender continues sending new chunks while waiting for ACKs, ensuring efficient data flow.
### 3. Simulated Loss (for Testing):
- The receiver simulates a packet loss scenario by deliberately skipping every third ACK to test retransmission functionality.
- This behavior should be commented out before the final submission.
## File Structure
- `client.c`: Implements the client-side logic for sending data, handling retransmissions, and receiving acknowledgments.
- `server.c`: Implements the server-side logic for receiving chunks, sending acknowledgments, and reassembling the message.
## Non-blocking Sockets
This project makes use of non-blocking sockets (or alternatively, threads) to allow the sender to transmit new chunks without waiting for the ACK of the previous chunk.
## Notes
- The project simulates a single client-server communication.
- For testing purposes, ACKs are randomly skipped (e.g., every third chunk) to demonstrate the retransmission mechanism. Be sure to comment out the ACK skipping code before submitting.

### AI Tools links
![Screenshot from 2024-10-14 22-16-26](https://github.com/user-attachments/assets/2bf3e217-eee2-46eb-bfa8-9b9478154ba6)
![Screenshot from 2024-10-14 22-16-44](https://github.com/user-attachments/assets/5520a0e4-d039-484b-b1ba-948b69d6982d)
![Screenshot from 2024-10-14 22-19-53](https://github.com/user-attachments/assets/3c644611-0de5-4e5d-ab9c-c5c4edfb3ee9)
![Screenshot from 2024-10-14 22-20-15](https://github.com/user-attachments/assets/a8d431b5-986d-4148-a537-f2231bb42d8f)
![Screenshot from 2024-10-14 22-25-26](https://github.com/user-attachments/assets/f69f15df-ecc2-41ef-ad65-ff8ee59a9ce7)
![Screenshot from 2024-10-14 22-28-00](https://github.com/user-attachments/assets/29b0c282-82db-44e5-b9e0-5ee901a3c05c)
