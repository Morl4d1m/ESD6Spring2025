import os
import time

def sleep_function(index):
    print(f'Thread/Process with index {index} began execution')
    time.sleep(2)
    print(f'Thread/Process with index {index} finished execution {time.thread_time()}')

def dummy_function(index):			# Function to check execution time
    print(f'Thread/Process with index {index} began execution')
    while(time.thread_time()<1):
        a=0                         # Dummy instruction
    print(f'Thread/Process with index {index} finished execution {time.thread_time()}')
    
def get_process_id(index,execution_time, global_array):
    pid = os.getpid()
    print(f'{pid} began execution for {execution_time} seconds')
    while(time.thread_time()<execution_time):
        a=0                         # Dummy instruction
    print(f'{pid} finished execution after {time.thread_time()} seconds')
    global_array[index] = pid
    
    
def return_process_id(execution_time):
    pid = os.getpid()
    print(f'{pid} began sleeping for {execution_time} seconds')
    while(time.thread_time()<execution_time):
        a=0                         # Dummy instruction
    print(f'{pid} finished execution after {time.thread_time()} seconds')
    return pid
