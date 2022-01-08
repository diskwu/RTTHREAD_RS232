from building import *

cwd = GetCurrentDir()
path = [cwd+'/inc']
src  = Glob('src/*.c')
 
group = DefineGroup('rs232', src, depend = ['PKG_USING_RS232'], CPPPATH = path)

Return('group')