leveldb自定义env

env.h: 增加GetSimpleEnv，GetMemEnv两个接口，便于直接在测试程序中使用自定义env
helper/memenv/memenv.cc:额外实现GetMemEnv接口

myenv文件夹
filesystem.h/filesystem.cc:实现简单的块接口 数据文件 目录文件并以此构建文件系统
simple_env.cc:利用filesystem.h实现的文件接口重写env.h中部分操作，实现GetSimpleEnv接口

test_env/test.cpp
向leveldb中写入数据并读取验证

相关博客：[leveldb自定义env](https://blog.csdn.net/freedom1523646952/article/details/130514899?spm=1001.2014.3001.5502)