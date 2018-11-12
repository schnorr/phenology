from PIL import Image
from colors import hsv2rgb, rgb2hsv
import sys

if len(sys.argv):
  print('No source image path provided. Using default:')
  sourceImagePath = '../Output_PEG/2014/2014_091_13_1.jpg'
  print(sourceImagePath)
else:
  sourceImagePath = sys.argv[1]

# Open the source image
sourceImage = Image.open(sourceImagePath)
sourcePixels = sourceImage.load()

# Create a new one
destImage = Image.new('RGB', (sourceImage.size[0], sourceImage.size[1]), 'black')
destPixels = destImage.load()

for i in range(sourceImage.size[0]):
  for j in range(sourceImage.size[1]):
    r, g, b = sourcePixels[i,j]
    h, s, v = rgb2hsv(r, g, b)
    destR, destG, destB = hsv2rgb(h, 1, 1)
    destPixels[i, j] = (destR, destG, destB)

destImage.save('hsv-image.jpg', 'JPEG')
print('Saved new image to "./hsv-image.jpg"')
