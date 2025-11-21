#pragma once

#include "Frame.h"

class FramePool
{
public:
    FramePool()
    {
        m_FreeFrames.reserve(1024);
        m_AllFrames.reserve(1024);
    }

    inline Frame* Acquire(size_t numLocals)
    {
        if (!m_FreeFrames.empty())
        {
            Frame* frame = m_FreeFrames.back();
            m_FreeFrames.pop_back();
            frame->Reset(numLocals);
            return frame;
        }

        // Allocate new if pool is empty
        Frame* frame = new Frame(numLocals);
        m_AllFrames.push_back(frame);
        return frame;
    }

    inline void Release(Frame* frame)
    {
        m_FreeFrames.push_back(frame);
    }

    ~FramePool()
    {
        for (Frame* f : m_AllFrames)
            delete f;
    }

private:
    std::vector<Frame*> m_FreeFrames;
    std::vector<Frame*> m_AllFrames;
};