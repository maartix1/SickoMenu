#pragma once

#include "RenderCmd.hpp"

#include <utility>
#include <assert.h>

class ImGuiRenderer
{
public:
	using RenderCommandFunc = void(*)(void*);

	template<typename Function>
	static void Submit(Function&& function)
	{
		auto renderCmd = [](void* ptr) {
			(*static_cast<Function*>(ptr))();
		};
		auto destroyCmd = [](void* ptr) noexcept {
			try
			{
				static_cast<Function*>(ptr)->~Function();
			}
			catch (...)
			{
				// Cleanup must not unwind through the render worker.
			}
		};
		};

		auto& commandQueue = GetCmdQueue();
		auto storageBuffer = commandQueue.Allocate(sizeof(function), alignof(Function));
		if (storageBuffer == nullptr)
			return;

		try
		{
			new (storageBuffer) Function(std::forward<Function>(function));
		}
		catch (...)
		{
			commandQueue.RollbackAllocation(storageBuffer);
			return;
		}

		if (!commandQueue.Enqueue(renderCmd, destroyCmd, storageBuffer))
			destroyCmd(storageBuffer);
		}
	}

	static void ExecuteQueue();
private:
	static RenderCmdQueue& GetCmdQueue();
};