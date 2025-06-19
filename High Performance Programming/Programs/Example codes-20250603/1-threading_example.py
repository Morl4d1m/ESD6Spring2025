import time
import threading
import os
from parallel_functions import *                # Requires to have the file parallel_functions.py in the same folder


if __name__ == "__main__":
    n_cores = os.cpu_count()                    # Get the number of "cores"
    print(f"Number of CPU cores: {n_cores}")    # Print the number of cores
    number_of_threads = n_cores                 # Create as many threads as cores                     
    threads = list()
    print(f'%%%%%%%%%%%%%%%%\nCreating {number_of_threads} threads')
    t_start = time.time()		    		        # Get an initial timestamp
    for index in range(number_of_threads):
        f = threading.Thread(target=sleep_function, args=(index,))  # Map function to thread: try both sleep_function and dummy_function
        threads.append(f)                                           # Append thread to list
        threads[-1].start()                                         # Start the newly appended thread
    thread_creation_t_elapsed = time.time()-t_start
    avg_thread_creation_time = thread_creation_t_elapsed/number_of_threads
    for thread in threads:
        thread.join()						                        # Join the threads one by one
    overall_t_elapsed = time.time()-t_start		                    # Compare actual timestamp to initial
    print('%%%%%%%%%%%%%%%%\nAll threads finished execution')
    print('The time to create the threads is {:0.3f} milliseconds'.format(thread_creation_t_elapsed*1e3))
    print('Average time to create a thread is {:0.3f} milliseconds'.format(avg_thread_creation_time*1e3))
    print('The overall elapsed time is {:0.3f} seconds'.format(overall_t_elapsed))
