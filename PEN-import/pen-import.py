import os
import glob
from shutil import copyfile

input_directory = '/Users/alles/ExtensionHD/PEN/TKY/TKY_/public_html/original/dc/dc_2003'
image_pattern = '/**/*.jpg'

output_directory = '../PEN/TKY/'

try:
  os.mkdir(output_directory)
except:
  pass

files = glob.glob(input_directory + image_pattern, recursive=True)
for file in files:
  output_file = os.path.join(output_directory, file.split('/')[-1])
  copyfile(file, os.path.join(output_directory,file.split('/')[-1]))
  print('Copied file ' + file + ' to ' + output_file)
