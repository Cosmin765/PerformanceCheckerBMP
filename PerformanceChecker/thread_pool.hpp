#include <Windows.h>
#include <vector>
#include <algorithm>    
#pragma once

class ThreadPool
{
public:
	ThreadPool(DWORD workersCount) : workersCount(std::max(int(workersCount), 64)) {}
	~ThreadPool()
	{
		this->Shutdown();
	}

	BOOL Submit(DWORD(*target)(LPVOID), LPVOID arg, LPHANDLE handle = NULL, BOOL block = TRUE)
	{
		while (this->GetRunningTasks() >= this->workersCount)
		{
			if (!block)
				return FALSE;
			
			this->Wait();
		}

		HANDLE thread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)target, arg, NULL, NULL);
		this->tasksQueue.push_back(thread);

		if (handle)
			*handle = thread;

		return TRUE;
	}

	BOOL Wait(DWORD timeout = INFINITE, LPHANDLE handle = NULL)
	{
		// returns TRUE if a task finished while waiting, FALSE otherwise
		if (this->GetRunningTasks() == 0)
			return FALSE;

		DWORD value = WaitForMultipleObjects(this->tasksQueue.size(), this->tasksQueue.data(), FALSE, timeout);

		if (value >= WAIT_OBJECT_0 && value < this->tasksQueue.size())
		{
			if (handle)
				*handle = *(this->tasksQueue.data() + value);

			this->tasksQueue.erase(this->tasksQueue.begin() + value);
			return TRUE;
		}
		
		return FALSE;
	}

	DWORD GetRunningTasks()
	{
		return this->tasksQueue.size();
	}

	VOID Shutdown()
	{
		while (this->GetRunningTasks() > 0)
			this->Wait();
	}

private:
	DWORD workersCount;
	std::vector<HANDLE> tasksQueue;
};
