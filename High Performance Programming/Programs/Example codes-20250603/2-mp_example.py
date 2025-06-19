import time
import multiprocessing as mp
from parallel_functions import *                    # Requires to have the file parallel_functions.py in the same folder


if __name__ == "__main__":
    n_cores = mp.cpu_count()                        # Get the number of "cores"
    print(f"Number of CPU cores: {n_cores}")        # Print the number of cores
    number_of_processes = n_cores                     # Create as many processes as cores                     
    processes = list()
    print(f'%%%%%%%%%%%%%%%%\nCreating {number_of_processes} processes')
    t_start = time.time()		    		        # Get an initial timestamp
    for index in range(number_of_processes):
        f = mp.Process(target=dummy_function, args=(index,))        # Map function to thread: try both sleep_function and dummy_function
        processes.append(f)                                         # Append process to list
        processes[-1].start()                                       # Start the newly appended process
    process_creation_t_elapsed = time.time()-t_start
    avg_process_creation_time = process_creation_t_elapsed/number_of_processes
    for process in processes:
        process.join()						                        # Join the processes one by one
    overall_t_elapsed = time.time()-t_start		                    # Compare actual timestamp to initial
    print('%%%%%%%%%%%%%%%%\nAll processes finished execution')
    print('The time to create the processes is {:0.3} milliseconds'.format(process_creation_t_elapsed*1e3))
    print('Average time to create a process is {:0.3} milliseconds'.format(avg_process_creation_time*1e3))
    print('The overall elapsed time is {:0.3} seconds'.format(overall_t_elapsed))
