import numpy as np
import gym
import random
import time


def timer(f):        # Decorator to calculate execution time of a funciton
    def wrapper(*args, **kw):                   # Needed to decorate a function with input arguments
        t_start = time.time()
        result = f(*args, **kw)                 # Calling function
        t_end = time.time()
        return result, t_end-t_start            # Return the result AND the execution time
    return wrapper
    
@timer    
def training(env, hyper_params, n_episodes):
    q_table = np.zeros([env.observation_space.n, env.action_space.n])
    alpha = hyper_params[0] 
    gamma = hyper_params[1]  
    epsilon = hyper_params[2] 
    print(f'Your monkey will begin training with hyper parameters\n alpha  = {alpha}\n gamma = {gamma}\n epsilon = {epsilon}\n ') 
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

        if i % 1000 == 0:
            print(f"Episode: {i}")
        
    print('Now you have a trained monkey driver')
    return q_table


def evaluation(env, q_table, n_episodes):

    total_epochs, total_penalties = 0, 0

    for i in range(n_episodes):
        if i % 10 == 0:
            print(f"Episode: {i}")
        state = env.reset()
        epochs, penalties, reward = 0, 0, 0
        
        done = False
        
        while not done:
            action = np.argmax(q_table[state])
            state, reward, done, info = env.step(action)
            if i==0:
                env.s = state
                env.render()        # To render the environment in the first episode
                input('')
            if reward == -10:
                penalties += 1

            epochs += 1

        total_penalties += penalties
        total_epochs += epochs
    avg_timesteps = total_epochs / n_episodes
    avg_penalties = total_penalties / n_episodes
    print(f"Results after {n_episodes} episodes:")
    print(f"Average timesteps per episode: {avg_timesteps}")
    print(f"Average penalties per episode: {avg_penalties}")
    return avg_timesteps, avg_penalties
    
if __name__=="__main__":
    
    env = gym.make("Taxi-v3").env

    # Hyperparameters
    alpha = 0.9
    gamma = 0.6
    epsilon = 0.3

    print("Action Space {}".format(env.action_space))
    print("State Space {}".format(env.observation_space))

    state = env.encode(3, 1, 2, 0) # (taxi row, taxi column, passenger index, destination index)
    print("State:", state)

    env.s = state
    env.render()
    input('Press enter to start')

    n_training_episodes = int(1e4)
    n_evaluation_episodes = int(1e3)



    frames = [] # for animation

    q_table, training_time = training(env, [alpha, gamma, epsilon], n_training_episodes)
    print('Training took {:0.3f} milliseconds'.format(training_time*1e3))
    input('Press enter to set him loose')

    """ Testing the monkey driver """

    avg_timesteps, avg_penalties = evaluation(env, q_table, n_evaluation_episodes)

