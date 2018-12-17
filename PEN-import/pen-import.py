import os
import glob
from shutil import copyfile

input_directory = '/Users/alles/ExtensionHD/PEN/TKY-2004'
image_pattern = '/**/*y18*.jpg'

output_directory = '../PEN/TKY-2004'

files = glob.glob(input_directory + image_pattern, recursive=True)
for index, file in enumerate(files):
  output_file = os.path.join(output_directory, file.split('/')[-1])
  copyfile(file, os.path.join(output_directory,file.split('/')[-1]))
  print('Copied file ' + file + ' to ' + output_file + ' (' + str(index) + '/' + str(len(files)-1) + ')')
