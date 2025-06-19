import matplotlib.pyplot as plt
import numpy as np
import scipy as sp
from PIL import Image

#Load image
im = np.array(Image.open('Profile_ILM.jpg'))

# Make grayscale
gryim = np.mean(im[:,:,0:2],2)

thresh = 110
plt.imshow(im)

plt.figure()
plt.imshow(gryim)

bw = np.multiply(255, gryim>thresh)

plt.figure()
plt.imshow(bw)
plt.show()
