#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

using byte = uint8_t;

class RenderCmdQueue
{
public:
    using RenderCmdExecuteFunc = void(*)(void*);
    using RenderCmdDestroyFunc = void(*)(void*) noexcept;

    RenderCmdQueue();
    ~RenderCmdQueue();

    void* Allocate(uint32_t size, size_t alignment);
    bool Enqueue(RenderCmdExecuteFunc execute, RenderCmdDestroyFunc destroy, void* storage);
    bool RollbackAllocation(void* storage);

    void Execute();

private:
    struct Command
    {
        RenderCmdExecuteFunc execute;
        RenderCmdDestroyFunc destroy;
        void* storage;
    };

    static constexpr size_t CmdBufferCapacity = 10 * 1024 * 1024;

    byte* m_CmdBuffer = nullptr;
    size_t m_CmdBufferOffset = 0;
    void* m_LastAllocationStorage = nullptr;
    size_t m_LastAllocationPreviousOffset = 0;
    size_t m_LastAllocationEnd = 0;
    std::vector<Command> m_Commands;
};