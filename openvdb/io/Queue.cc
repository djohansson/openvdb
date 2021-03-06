///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2018 DreamWorks Animation LLC
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DreamWorks Animation nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
// LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
//
///////////////////////////////////////////////////////////////////////////

/// @file Queue.cc
/// @author Peter Cucka

#include "Queue.h"

#include "File.h"
#include "Stream.h"
#include <openvdb/Exceptions.h>
#include <openvdb/util/logging.h>
#ifdef OPENVDB_USE_TBB
#include <tbb/concurrent_hash_map.h>
#include <tbb/task.h>
#endif
#include <atomic>
#include <mutex>
#include <algorithm> // for std::max()
#include <iostream>
#include <map>
#include <thread>
#include <unordered_map>


namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {
namespace io {

namespace {

using Mutex = std::mutex;
using Lock = std::lock_guard<Mutex>;

// Abstract base class for queuable TBB tasks that adds a task completion callback
class Task
#ifdef OPENVDB_USE_TBB
	: public tbb::task
#endif
{
public:
#ifdef OPENVDB_USE_TBB
	using TaskPtr = tbb::task*;
#define TASKBASE_OVVERRIDE override
#else
	using TaskPtr = Task*;
#define TASKBASE_OVVERRIDE
#endif

    Task(Queue::Id id): mId(id) {}
	~Task() TASKBASE_OVVERRIDE
	{}

	virtual TaskPtr execute() TASKBASE_OVVERRIDE = 0;

    Queue::Id id() const { return mId; }

    void setNotifier(Queue::Notifier& notifier) { mNotify = notifier; }

protected:
    void notify(Queue::Status status) { if (mNotify) mNotify(this->id(), status); }

private:
    Queue::Id mId;
    Queue::Notifier mNotify;
};


// Queuable TBB task that writes one or more grids to a .vdb file or an output stream
class OutputTask: public Task
{
public:
    OutputTask(Queue::Id id, const GridCPtrVec& grids, const Archive& archive,
        const MetaMap& metadata)
        : Task(id)
        , mGrids(grids)
        , mArchive(archive.copy())
        , mMetadata(metadata)
    {}

    TaskPtr execute() TASKBASE_OVVERRIDE
    {
        Queue::Status status = Queue::FAILED;
        try {
            mArchive->write(mGrids, mMetadata);
            status = Queue::SUCCEEDED;
        } catch (std::exception& e) {
            if (const char* msg = e.what()) {
                OPENVDB_LOG_ERROR(msg);
            }
        } catch (...) {
        }
        this->notify(status);
        return nullptr; // no successor to this task
    }

private:
    GridCPtrVec mGrids;
    SharedPtr<Archive> mArchive;
    MetaMap mMetadata;
};
#undef TASKBASE_OVVERRIDE
} // unnamed namespace


////////////////////////////////////////


// Private implementation details of a Queue
struct Queue::Impl
{
    using NotifierMap = std::map<Queue::Id, Queue::Notifier>;
    /// @todo Provide more information than just "succeeded" or "failed"?
#ifdef OPENVDB_USE_TBB
    using StatusMap = tbb::concurrent_hash_map<Queue::Id, Queue::Status>;
#else
	using StatusMap = std::unordered_map<Queue::Id, Queue::Status>;
#endif


    Impl()
        : mTimeout(Queue::DEFAULT_TIMEOUT)
        , mCapacity(Queue::DEFAULT_CAPACITY)
        , mNextId(1)
        , mNextNotifierId(1)
    {
        mNumTasks = 0; // note: must explicitly zero-initialize atomics
    }
    ~Impl() {}

    // Disallow copying of instances of this class.
    Impl(const Impl&);
    Impl& operator=(const Impl&);

    // This method might be called from multiple threads.
    void setStatus(Queue::Id id, Queue::Status status)
    {
#ifdef OPENVDB_USE_TBB
        StatusMap::accessor acc;
        mStatus.insert(acc, id);
        acc->second = status;
#else
		std::lock_guard<std::mutex> lock(mStatusMutex);
		mStatus[id] = status;
#endif
    }

    // This method might be called from multiple threads.
    void setStatusWithNotification(Queue::Id id, Queue::Status status)
    {
        const bool completed = (status == SUCCEEDED || status == FAILED);

        // Update the task's entry in the status map with the new status.
        this->setStatus(id, status);

        // If the client registered any callbacks, call them now.
        bool didNotify = false;
        {
            // tbb::concurrent_hash_map does not support concurrent iteration
            // (i.e., iteration concurrent with insertion or deletion),
            // so we use a mutex-protected STL map instead.  But if a callback
            // invokes a notifier method such as removeNotifier() on this queue,
            // the result will be a deadlock.
            /// @todo Is it worth trying to avoid such deadlocks?
            Lock lock(mNotifierMutex);
            if (!mNotifiers.empty()) {
                didNotify = true;
                for (NotifierMap::const_iterator it = mNotifiers.begin();
                    it != mNotifiers.end(); ++it)
                {
                    it->second(id, status);
                }
            }
        }
        // If the task completed and callbacks were called, remove
        // the task's entry from the status map.
        if (completed) {
            if (didNotify) {
#ifdef OPENVDB_USE_TBB
                StatusMap::accessor acc;
                if (mStatus.find(acc, id)) {
                    mStatus.erase(acc);
                }
#else
				std::lock_guard<std::mutex> lock(mStatusMutex);
				mStatus.erase(id);
#endif
            }
            --mNumTasks;
        }
    }

    bool canEnqueue() const { return mNumTasks < Int64(mCapacity); }

    void enqueue(Task& task)
    {
		using hires_clock = std::chrono::high_resolution_clock;
		using time_point = hires_clock::time_point;

		time_point start = hires_clock::now();
        while (!canEnqueue())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (std::chrono::duration_cast<std::chrono::seconds>(hires_clock::now() - start).count() > double(mTimeout))
			{
                OPENVDB_THROW(RuntimeError,
                    "unable to queue I/O task; " << mTimeout << "-second time limit expired");
            }
        }

        Queue::Notifier notify = std::bind(&Impl::setStatusWithNotification, this,
            std::placeholders::_1, std::placeholders::_2);
        task.setNotifier(notify);
        this->setStatus(task.id(), Queue::PENDING);
#ifdef OPENVDB_USE_TBB
        tbb::task::enqueue(task);
#else
		task.execute(); // [djo]: todo: move to std::packaged_task/std::thread
#endif
        ++mNumTasks;
    }

    Index32 mTimeout;
    Index32 mCapacity;
    std::atomic<Int32> mNumTasks;
    Index32 mNextId;
    StatusMap mStatus;
#ifndef OPENVDB_USE_TBB
	std::mutex mStatusMutex;
#endif
    NotifierMap mNotifiers;
    Index32 mNextNotifierId;
    Mutex mNotifierMutex;
};


////////////////////////////////////////


Queue::Queue(Index32 capacity): mImpl(new Impl)
{
    mImpl->mCapacity = capacity;
}


Queue::~Queue()
{
    // Wait for all queued tasks to complete (successfully or unsuccessfully).
    /// @todo Allow the queue to be destroyed while there are uncompleted tasks
    /// (e.g., by keeping a static registry of queues that also dispatches
    /// or blocks notifications)?
    while (mImpl->mNumTasks > 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}


////////////////////////////////////////


bool Queue::empty() const { return (mImpl->mNumTasks == 0); }
Index32 Queue::size() const { return Index32(std::max<Int32>(0, mImpl->mNumTasks)); }
Index32 Queue::capacity() const { return mImpl->mCapacity; }
void Queue::setCapacity(Index32 n) { mImpl->mCapacity = std::max<Index32>(1, n); }

/// @todo void Queue::setCapacity(Index64 bytes);

/// @todo Provide a way to limit the number of tasks in flight
/// (e.g., by enqueueing tbb::tasks that pop Tasks off a concurrent_queue)?

/// @todo Remove any tasks from the queue that are not currently executing.
//void clear() const;

Index32 Queue::timeout() const { return mImpl->mTimeout; }
void Queue::setTimeout(Index32 sec) { mImpl->mTimeout = sec; }


////////////////////////////////////////


Queue::Status
Queue::status(Id id) const
{
#ifdef OPENVDB_USE_TBB
    Impl::StatusMap::const_accessor acc;
    if (mImpl->mStatus.find(acc, id))
#else
	const auto acc = mImpl->mStatus.find(id);
	if (acc != mImpl->mStatus.end())
#endif
	{
        const Status status = acc->second;
        if (status == SUCCEEDED || status == FAILED) {
            mImpl->mStatus.erase(acc);
        }
        return status;
    }

    return UNKNOWN;
}


Queue::Id
Queue::addNotifier(Notifier notify)
{
    Lock lock(mImpl->mNotifierMutex);
    Queue::Id id = mImpl->mNextNotifierId++;
    mImpl->mNotifiers[id] = notify;
    return id;
}


void
Queue::removeNotifier(Id id)
{
    Lock lock(mImpl->mNotifierMutex);
    Impl::NotifierMap::iterator it = mImpl->mNotifiers.find(id);
    if (it != mImpl->mNotifiers.end()) {
        mImpl->mNotifiers.erase(it);
    }
}


void
Queue::clearNotifiers()
{
    Lock lock(mImpl->mNotifierMutex);
    mImpl->mNotifiers.clear();
}


////////////////////////////////////////


Queue::Id
Queue::writeGrid(GridBase::ConstPtr grid, const Archive& archive, const MetaMap& metadata)
{
    return writeGridVec(GridCPtrVec(1, grid), archive, metadata);
}


Queue::Id
Queue::writeGridVec(const GridCPtrVec& grids, const Archive& archive, const MetaMap& metadata)
{
    const Queue::Id taskId = mImpl->mNextId++;
    // From the "GUI Thread" chapter in the TBB Design Patterns guide
#ifdef OPENVDB_USE_TBB
	auto taskMem = tbb::task::allocate_root();
#else
	char taskMem[sizeof(OutputTask)]; // we will call execute directly in enqueue, so fine to allocate on stack
#endif

    OutputTask* task =
        new(taskMem) OutputTask(taskId, grids, archive, metadata);

    try {
        mImpl->enqueue(*task);
    } catch (openvdb::RuntimeError&) {
#ifdef OPENVDB_USE_TBB
        // Destroy the task if it could not be enqueued, then rethrow the exception.
        tbb::task::destroy(*task);
#endif
        throw;
    }
    return taskId;
}

} // namespace io
} // namespace OPENVDB_VERSION_NAME
} // namespace openvdb

// Copyright (c) 2012-2018 DreamWorks Animation LLC
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
