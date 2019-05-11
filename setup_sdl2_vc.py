import os
import urllib.request
import zipfile

version = '2.0.9'
file_name = 'SDL2-devel-' + version + '-VC.zip'
url = 'https://www.libsdl.org/release/' + file_name
dir_name = 'SDL2-' + version

urllib.request.urlretrieve(url, file_name)
zipfile.ZipFile(file_name).extractall()
os.remove(file_name)
os.makedirs(name='third_party', exist_ok=True)
os.rename(dir_name, 'third_party/SDL2')
