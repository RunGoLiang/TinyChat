#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include "base/ThreadPool.h"

using namespace base;
using namespace std;

void taskFunc(int time)
{
    sleep(time);
}

int main()
{
    ThreadPool pool(5);

    cout << "尝试启动manage线程..." << endl;
    pool.startPool();
    cout << "manage线程已启动!!!..." << endl;

    // 工作
    for(int i=0;i < 10;++i)
    {
        pool.addTask(std::bind(taskFunc,static_cast<int>(rand()%10)));
        sleep(1);
    }

    cout << "尝试停止manage线程..." << endl;
    pool.stopPool();
    cout << "manage线程已停止!!!" << endl;

    sleep(3);

    cout << "尝试重新启动manage线程..." << endl;
    pool.startPool();
    cout << "manage线程已启动!!!" << endl;

    // 工作
    for(int i=0;i < 10;++i)
    {
        pool.addTask(std::bind(taskFunc,static_cast<int>(rand()%20)));
        sleep(1);
    }

    cout << "尝试停止manage线程..." << endl;
    pool.stopPool();
    cout << "manage线程已停止!!!" << endl;

    exit(0);
}