#pragma once

#include "arch.h"
#include <deque>
#include <mutex>
#include <future>
#include <thread>
#include <vector>
#include <cassert>
#include <condition_variable>

namespace nano
{
        using future_t = std::future<void>;
        using tpool_task_t = std::packaged_task<void()>;

        ///
        /// \brief enqueue tasks to be run in a thread pool.
        ///
        class tpool_queue_t
        {
        public:
                ///
                /// \brief constructor
                ///
                tpool_queue_t() = default;

                ///
                /// \brief enqueue a new task to execute
                ///
                template <typename tfunction>
                future_t enqueue(tfunction&& f)
                {
                        auto task = tpool_task_t(f);
                        auto fut = task.get_future();

                        const std::lock_guard<std::mutex> lock(m_mutex);
                        m_tasks.emplace_back(std::move(task));
                        m_condition.notify_all();

                        return fut;
                }

                // attributes
                std::deque<tpool_task_t>        m_tasks;                ///< tasks to execute
                mutable std::mutex              m_mutex;                ///< synchronization
                mutable std::condition_variable m_condition;            ///< signaling
                bool                            m_stop{false};          ///< stop requested
        };

        ///
        /// \brief worker to process tasks enqueued in a thread pool.
        ///
        class tpool_worker_t
        {
        public:
                ///
                /// \brief constructor
                ///
                explicit tpool_worker_t(tpool_queue_t& queue) : m_queue(queue) {}

                ///
                /// \brief execute tasks when available
                ///
                void operator()() const
                {
                        while (true)
                        {
                                tpool_task_t task;

                                // wait for a new task to be available in the queue
                                {
                                        std::unique_lock<std::mutex> lock(m_queue.m_mutex);

                                        m_queue.m_condition.wait(lock, [&]
                                        {
                                                return m_queue.m_stop || !m_queue.m_tasks.empty();
                                        });

                                        if (m_queue.m_stop)
                                        {
                                                m_queue.m_tasks.clear();
                                                m_queue.m_condition.notify_all();
                                                break;
                                        }

                                        task = std::move(m_queue.m_tasks.front());
                                        m_queue.m_tasks.pop_front();
                                }

                                // execute the task
                                task();
                        }
                }

        private:

                // attributes
                tpool_queue_t&          m_queue;        ///< task queue to process
        };

        ///
        /// \brief RAII object to wait for a given set of futures (aka barrier).
        ///
        template <typename tfuture>
        class tpool_section_t
        {
        public:
                ///
                /// \brief destructor
                ///
                ~tpool_section_t()
                {
                        // block until all futures are done
                        for (const auto& future : m_futures)
                        {
                                future.wait();
                        }
                }

                ///
                /// \brief add a new future to wait for.
                ///
                void push_back(tfuture future)
                {
                        m_futures.emplace_back(std::move(future));
                }

        private:

                // attributes
                std::vector<tfuture>    m_futures;
        };

        ///
        /// \brief thread pool.
        /// NB: this is heavily copied/inspired by http://progsch.net/wordpress/?p=81
        ///
        class tpool_t
        {
        public:

                ///
                /// \brief single instance
                ///
                static tpool_t& instance()
                {
                        static tpool_t the_pool;
                        return the_pool;
                }

                ///
                /// \brief disable copying
                ///
                tpool_t(const tpool_t&) = delete;
                tpool_t& operator=(const tpool_t&) = delete;

                ///
                /// \brief disable moving
                ///
                tpool_t(tpool_t&&) = delete;
                tpool_t& operator=(tpool_t&&) = delete;

                ///
                /// \brief destructor
                ///
                ~tpool_t()
                {
                        stop();
                }

                ///
                /// \brief enqueue a new task to execute
                ///
                template <typename tfunction>
                auto enqueue(tfunction f)
                {
                        return m_queue.enqueue(std::move(f));
                }

                ///
                /// \brief number of available worker threads
                ///
                std::size_t workers() const
                {
                        return m_workers.size();
                }

                ///
                /// \brief number of tasks still enqueued
                ///
                std::size_t tasks() const
                {
                        const std::lock_guard<std::mutex> lock(m_queue.m_mutex);
                        return m_queue.m_tasks.size();
                }

        private:

                tpool_t()
                {
                        const auto n_workers = static_cast<std::size_t>(physical_cpus());

                        m_workers.reserve(n_workers);
                        for (size_t i = 0; i < n_workers; ++ i)
                        {
                                m_workers.emplace_back(m_queue);
                        }
                        for (size_t i = 0; i < n_workers; ++ i)
                        {
                                m_threads.emplace_back(std::ref(m_workers[i]));
                        }
                }

                void stop()
                {
                        // stop & join
                        {
                                const std::lock_guard<std::mutex> lock(m_queue.m_mutex);
                                m_queue.m_stop = true;
                                m_queue.m_condition.notify_all();
                        }

                        for (auto& thread : m_threads)
                        {
                                thread.join();
                        }
                }

        private:

                // attributes
                std::vector<std::thread>        m_threads;      ///<
                std::vector<tpool_worker_t>     m_workers;      ///<
                tpool_queue_t                   m_queue;        ///< tasks to execute + synchronization
        };

        ///
        /// \brief split a loop computation of the given size using a thread pool.
        /// NB: the operator receives the range [begin, end) to process and the assigned thread index:
        ///     op(begin, end, thread)
        ///
        template <typename tsize, typename toperator>
        void loopit(const tsize size, const tsize max_thread_chunk, const toperator& op)
        {
                auto& pool = tpool_t::instance();

                const auto workers = static_cast<tsize>(pool.workers());
                const auto thread_chunk = (size + workers - 1) / workers;
                if (thread_chunk > tsize(0))
                {
                        tpool_section_t<future_t> section;
                        for (tsize thread = 0; thread < workers; ++ thread)
                        {
                                const auto begin = thread * thread_chunk;
                                const auto end = std::min(begin + thread_chunk, size);
                                const auto chunk = std::min(thread_chunk, max_thread_chunk);

                                if (begin >= end)
                                {
                                        // not enough data to split to all threads
                                        break;
                                }

                                assert(begin < end && chunk > 0);
                                section.push_back(pool.enqueue([&, begin, end, chunk, thread]()
                                {
                                        for (auto ibegin = begin; ibegin < end; ibegin = std::min(ibegin + chunk, end))
                                        {
                                                op(ibegin, std::min(ibegin + chunk, end), thread);
                                        }
                                }));
                        }
                        // NB: the section is destroyed here waiting for all tasks to finish!
                }
        }

        ///
        /// \brief split a loop computation of the given size using a thread pool.
        /// NB: the operator receives the range [begin, end) to process:
        ///     op(begin, end)
        ///
        template <typename tsize, typename toperator>
        void loopi(const tsize size, const tsize max_thread_chunk, const toperator& op)
        {
                loopit(size, max_thread_chunk, [&] (const tsize begin, const tsize end, const tsize thread)
                {
                        NANO_UNUSED1(thread);
                        op(begin, end);
                });
        }
}
