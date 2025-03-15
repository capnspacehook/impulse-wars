import numpy as np

PUFF_BG = (6, 24, 24)
PUFF_CYAN = (0, 187, 187)
PUFF_RED = (187, 0, 0)
PUFF_YELLOW = (160, 160, 0)
PUFF_GREEN = (0, 187, 0)

img = np.zeros((256, 256, 3), dtype=np.uint8)
img[:] = PUFF_BG

b = 4

img[:128, :b] = PUFF_CYAN
img[:128, 128-b:128] = PUFF_CYAN
img[:b, :128] = PUFF_CYAN
img[128-b:128, :128] = PUFF_CYAN

img[:128, 128:128+b] = PUFF_RED
img[:128, 256-b:256] = PUFF_RED
img[:b, 128:256] = PUFF_RED
img[128-b:128, 128:256] = PUFF_RED

img[128:256, :b] = PUFF_YELLOW
img[128:256, 128-b:128] = PUFF_YELLOW
img[128:128+b, :128] = PUFF_YELLOW
img[256-b:256, :128] = PUFF_YELLOW

img[128:256, 128:256] = (0, 40, 0)
img[128:256, 128:128+b] = PUFF_GREEN
img[128:256, 256-b:256] = PUFF_GREEN
img[128:128+b, 128:256] = PUFF_GREEN
img[256-b:256, 128:256] = PUFF_GREEN

from PIL import Image
Image.fromarray(img).save('../docker/texture.png')

