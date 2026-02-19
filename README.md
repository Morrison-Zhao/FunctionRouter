# 函数路由器


### 线程池
1. worker封装线程，一个线程对应多个任务（任务队列）
2. 去中心化，均匀分发任务给worker
3. 空闲时偷任务执行


### 函数路由器
1. 支持注册多种类型函数结构（lambda, 成员函数, 普通函数）
2. 支持在主线程、当前线程、调度线程、线程池执行
3. 支持跨组件通过id映射调用函数


### 编译
```shell
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j4
    make install                          
```