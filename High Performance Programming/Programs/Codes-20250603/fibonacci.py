def fib(n):
    if n<=1:
        print(n)
        return n
    result = fib(n-1) + fib(n-2)
    print(result)
    return result
    
    
if __name__=="__main__":
    
    n = 16          # Number to obtain fibonacci sequence
    print(f'Fibonacci sequence of length {n} is\n')
    f = fib(n)
    
    
