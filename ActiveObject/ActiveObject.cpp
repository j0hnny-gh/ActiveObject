// ActiveObject.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <functional>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>

class cActiveObject
{
public:
	typedef std::function<void(void)> tJob;

public:
	cActiveObject()
	{
		Working = true;
		Thread = std::thread(&cActiveObject::Loop, this);
	}
	~cActiveObject()
	{
		{
			tLock l(Mutex);
			Working = false;
		}
		ConditionVar.notify_one();
		Thread.join();
	}

	void AddJob( tJob j )
	{
		{
			tLock l(Mutex);
			Queue.push(j);
		}
		ConditionVar.notify_one();
	}

private:
	typedef std::unique_lock<std::mutex> tLock;

private:
	void Loop()
	{
		while (true)
		{
			tJob j;
			while (Queue.size())
			{
				{
					tLock l(Mutex);
					j = Queue.front();
					Queue.pop();
				}
				j();
			}
			{
				tLock l(Mutex);
				if (!Working)
					break;
				ConditionVar.wait(l, [this]{ return !Working || Queue.size(); });
			}
		}
	}

	std::mutex Mutex;
	std::condition_variable ConditionVar;
	std::thread Thread;
	std::queue< tJob > Queue;
	bool Working;
};

struct iFoo
{
	virtual void Test(int param) = 0;
};

class cFoo : public iFoo
{
	// iFoo:
	virtual void Test(int param)
	{
		printf("%d OK!\n", param);
	}
	// iFoo.
};

int _tmain(int argc, _TCHAR* argv[])
{
	cActiveObject ao;

	iFoo* f = new cFoo;
	for (int i = 0; i < 100; ++i)
		ao.AddJob(std::bind(&iFoo::Test, f, i));

	getchar();
	return 0;
}

