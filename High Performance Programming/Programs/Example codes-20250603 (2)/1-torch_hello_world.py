import torch


if torch.cuda.is_available():
    device = torch.device("cuda")
else:
    device = torch.device("cpu")

print(f'Using {device}')
print(f'Hello world from device {torch.cuda.get_device_name()}')  
print(torch.cuda.get_device_properties(torch.cuda.current_device))
