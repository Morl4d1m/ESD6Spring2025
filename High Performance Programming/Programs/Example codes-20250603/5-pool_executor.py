import time
import concurrent.futures
import os
from parallel_functions import *


if __name__ == "__main__":
    n_cores = os.cpu_count()                        # Get the number of cores
    print(f"Number of CPU cores: {n_cores}")        # Print the number of cores
    time_to_live = 1                                # Time to live for each task
    sleep = False                             # True to sleep and False to execute dummy instruction
    t_start = time.time()				# Get an initial timestamp
    results = list()
    with concurrent.futures.ProcessPoolExecutor(max_workers=n_cores) as executor:
        for result in executor.map(return_process_id, [1,2,3,4,5]):
            results.append(result)
    overall_t_elapsed = time.time()-t_start		# Compare actual timestamp to initial
    print('%%%%%%%%%%%%%%%%\nAll processes finished execution')
    print(f'Process IDs:')
    print(results)
    print(f'The overall elapsed time is {overall_t_elapsed}')

