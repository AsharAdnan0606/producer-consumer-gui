# producer-consumer-gui
## Hospital Patient Queue Management System

## Overview

This project is a multithreaded simulation of a hospital patient management system built to demonstrate the Producer–Consumer problem in operating systems.

It models real-world hospital workflows where multiple receptionist desks handle incoming patients while different departments process them concurrently. The system uses POSIX threads, mutex locks, and semaphores to manage synchronization between producer and consumer threads, ensuring safe access to a shared bounded buffer.

A Qt-based GUI is used to visualize queue activity, thread execution, and system performance in real time.



## Key Features

- Implementation of Producer–Consumer problem using POSIX pthreads
- Thread-safe bounded circular buffer using mutex and semaphores
- Multiple producer and consumer threads simulating hospital departments
- Qt 6 GUI for real-time visualization of queue state
- Event logging system with timestamps
- Configurable buffer size, thread count, and processing delays
- Performance monitoring (throughput, wait time, thread utilization)



## System Architecture

The system consists of three main components:

1. Producers (Receptionists)
   - Generate patient entries
   - Insert them into a shared bounded buffer

2. Shared Buffer
   - Circular queue
   - Protected using mutex and semaphores

3. Consumers (Departments)
   - Remove and process patients
   - Simulate department-specific processing delays

The GUI runs on the main thread and receives updates from worker threads using thread-safe signals.



## Technologies Used

- C++
- POSIX Threads (pthreads)
- POSIX Semaphores
- Qt 6 (GUI framework)
- Linux (Ubuntu 22.04+)



## Synchronization Strategy

- Mutex locks are used to protect shared buffer access
- `sem_wait(empty)` ensures producers wait when buffer is full
- `sem_wait(full)` ensures consumers wait when buffer is empty
- Circular buffer prevents memory overhead and supports continuous operation
  


## Testing and Evaluation

The system was tested under multiple configurations:

- Different producer/consumer ratios
- Varying buffer sizes
- High-load stress conditions

Metrics observed:

- System throughput (patients per minute)
- Average waiting time
- Thread utilization
- Stability under concurrent execution
