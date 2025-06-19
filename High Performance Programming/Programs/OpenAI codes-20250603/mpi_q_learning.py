import numpy as np
import gym
import random
from mpi4py import MPI
import time


def timer(f):        # Decorator to calculate execution time of a funciton
    def wrapper(*args, **kw):                   # Needed to decorate a function with input arguments
        t_start = time.time()
        result = f(*args, **kw)                 # Calling function
        t_end = time.time()
        return result, t_end-t_start            # Return the result AND the execution time
    return wrapper
    
@timer    
def training(env, alpha, gamma, epsilon, n_episodes):
    q_table = np.zeros([env.observation_space.n, env.action_space.n])
    for i in range(n_episodes):
        state=env.reset()
        epochs = 0
        penalties, reward = 0, 0
        done=False
        while not done:
            if random.uniform(0, 1)<epsilon:
                action = env.action_space.sample()      # Explore action space monkey-style
            else:
                action = np.argmax(q_table[state])
                
            next_state, reward, done, info = env.step(action)
            old_q_value=q_table[state,action]
            next_max = np.max(q_table[next_state])
            new_value = (1-alpha)*old_q_value+alpha*(reward+gamma*next_max)
            q_table[state,action]=new_value
            
            if reward == -10:
                penalties += 1
            
            state=next_state
            epochs += 1

        #if i % 1000 == 0:
        #    print(f"Episode: {i}")
        
    return q_table


def evaluation(env, q_table, n_episodes):

    total_epochs, total_penalties = 0, 0

    for i in range(n_episodes):
        #if i % 10 == 0:
        #    print(f"Episode: {i}")
        state = env.reset()
        epochs, penalties, reward = 0, 0, 0
        
        done = False
        
        while not done:
            action = np.argmax(q_table[state])
            state, reward, done, info = env.step(action)

            if reward == -10:
                penalties += 1

            epochs += 1

        total_penalties += penalties
        total_epochs += epochs
    avg_timesteps = total_epochs / n_episodes
    avg_penalties = total_penalties / n_episodes
    return avg_timesteps, avg_penalties
    
if __name__=="__main__":
    print("Hi")
    env = gym.make("Taxi-v3").env
    
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    world_size = comm.Get_size()
    if rank == 0:
        # Hyperparameters
        print(f"Hello from process {rank}")
        global_time = time.time()
        gamma = 0.6
        epsilon = 0.3
        n_training_episodes = int(1e4)
        n_evaluation_episodes = int(1e3)
        alpha_min = 0.8
        global_params = [gamma, epsilon, n_training_episodes,n_evaluation_episodes]
        alpha = [alpha_min + ((1-alpha_min)*i/(world_size-1)) for i in range(world_size)] # Set value of alpha depending on the 
        print(f'Your monkey will begin training with hyper parameters\n alpha  = {alpha}\n gamma = {gamma}\n epsilon = {epsilon}\n ') 
        print("Action Space {}".format(env.action_space))
        print("State Space {}".format(env.observation_space))

        state = env.encode(3, 1, 2, 0) # (taxi row, taxi column, passenger index, destination index)
        print("State:", state)

        env.s = state
        env.render()

        frames = [] # for animation
        
    else:
        print(f"Hello from process {rank}")
        alpha = None
        global_params = None
    global_params = comm.bcast(global_params, root = 0) # Broadcast global training parameters from source 0
    alpha = comm.scatter(alpha, root = 0) # Send individual training parameters
    gamma = global_params[0]
    epsilon = global_params[1]
    n_training_episodes = global_params[2]
    n_evaluation_episodes = global_params[3]
    q_table, training_time = training(env, alpha, gamma, epsilon, n_training_episodes)
    training_times = comm.gather(training_time, root=0)
    
    
    avg_timesteps, avg_penalties = evaluation(env, q_table, n_evaluation_episodes)
    avg_timesteps_list = comm.gather(avg_timesteps, root=0)
    avg_penalties_list = comm.gather(avg_penalties, root=0)
    alpha = comm.gather(alpha, root=0)
    if rank == 0:
        global_exec_time = time.time()-global_time
        for p in range(world_size):
            print('%%%%%\nTraining in process {} with alpha = {:0.3f} took {:0.3f} milliseconds for {} episodes'.format(p,alpha[p], training_times[p]*1e3, n_training_episodes))
            print(f"Average timesteps per episode: {avg_timesteps_list[p]}")
            print(f"Average penalties per episode: {avg_penalties_list[p]}\n")
        print('The total time to complete is {:0.3f} milliseconds'.format(global_exec_time*1e3))
