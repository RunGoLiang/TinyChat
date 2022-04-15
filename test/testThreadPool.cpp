#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include "../base/ThreadPool.h"

using namespace base;
using namespace std;

void taskFunc(void)
{
    int i=0;
    while(i<1000*1000) ++i;
    while(i>0)         --i;
    while(i<1000*1000) ++i;
    while(i>0)         --i;
    while(i<1000*1000) ++i;
    while(i>0)         --i;
    while(i<1000*1000) ++i;
    while(i>0)         --i;
    while(i<1000*1000) ++i;
    while(i>0)         --i;
    while(i<1000*1000) ++i;
    while(i>0)         --i;
    while(i<1000*1000) ++i;
    while(i>0)         --i;
}

int main()
{
    ThreadPool pool(5);

    for(int i=0;i<300;++i)
    {
        cout << "尝试启动manage线程..." << endl;
        pool.startPool();
        cout << "manage线程已启动!!!" << endl;

        // 工作
        for(int i=0;i < 30;++i)
        {
            pool.addTask(taskFunc);
        }
        sleep(3);

        cout << "第" << i << "次尝试停止manage线程..." << endl;
        pool.stopPool();
        cout << "manage线程已停止!!!" << endl;

        sleep(1);
    }


    exit(0);
}