// [CFXS] //
#pragma once

#include <CFXS/Time.hpp>
#include <CFXS/Function.hpp>
#include <stl/initializer_list>
#include <stl/utility>

namespace CFXS {

    using TaskFunction = Function<void(void* user_data)>;
    class Task {
        enum Type : uint8_t { SINGLE_SHOT, PERIODIC };

    public:
#ifndef CFXS_TASK_MAX_GROUP_INDEX
    #define CFXS_TASK_MAX_GROUP_INDEX 4
#endif
        static constexpr auto MAX_GROUP_INDEX = CFXS_TASK_MAX_GROUP_INDEX;
        using Group_t                         = uint8_t;

        /////////////////////////////////////////////////////////////////////////////
        /// Enable processing
        /// \note call this only after creating all task groups
        static void enable_processing();

        /////////////////////////////////////////////////////////////////////////////
        /// Create task group
        /// \param group task group
        /// \param capacity prealloc size and max task count in group
        /// \return bool - true if group created successfully
        static bool add_group(Group_t group, size_t capacity);

        static bool add_groups(const std::initializer_list<stl::pair<Group_t, size_t>>& groups) {
            bool added_all = true;
            for (const auto& g : groups) {
                if (!add_group(g.first, g.second))
                    added_all = false;
            }
            return added_all;
        }

        /////////////////////////////////////////////////////////////////////////////
        /// Process all tasks in group
        /// \param group task group
        static void process_group(Group_t group);

        /////////////////////////////////////////////////////////////////////////////
        /// Create periodic task
        /// \param group task group
        /// \param name task label
        /// \param func function to queue
        /// \param period trigger period in ms
        /// \return Task* - pointer to created task or nullptr if creation failed
        static Task* create(Group_t group, const char* name, const TaskFunction& func, uint32_t period);

        /////////////////////////////////////////////////////////////////////////////
        /// Queue single shot task
        /// \param group task group
        /// \param name task label
        /// \param func function to queue
        /// \param delay trigger delay from current timestamp
        /// \return bool - true if queued successfully
        static bool queue(Group_t group, const char* name, const TaskFunction& func, uint32_t delay = 0);

        /////////////////////////////////////////////////////////////////////////////
        /// Get task currently being processed
        /// \param group task group
        /// \return Task* - pointer to current task being processed
        static Task* get_current_task(Group_t group);

        const TaskFunction& get_function() const {
            return m_function;
        }

        Group_t get_group() const {
            return m_group;
        }
        void set_group(Group_t group);

        uint32_t get_period() const {
            return m_period;
        }
        void set_period(uint32_t period) {
            m_period = period;
        }

        // Delete task
        void remove() {
            m_remove = true;
        }

        bool enabled() const {
            return m_enabled;
        }

        constexpr void set_enabled(bool state) {
            if (!m_process_time)
                m_process_time = CFXS::Time::ms + m_period;
            m_enabled = state;
        }

        constexpr void delay(CFXS::Time_t delay_ms) {
            m_process_time += delay_ms;
        }

        constexpr void start() {
            set_enabled(true);
        }

        constexpr void stop() {
            set_enabled(false);
        }

        constexpr const char* get_name() const { // NOLINT (can be made static if CFXS_TASK_NAME_FIELD is not 1)
#if CFXS_TASK_NAME_FIELD == 1
            return m_Name;
#else
            return "N/A";
#endif
        }

    private:
        Task(Group_t group, const char* name, const TaskFunction& func, Type type, uint32_t period = 0);

#if CFXS_TASK_NAME_FIELD == 1
        const char* m_Name;
#endif
        Group_t m_group;
        Type m_type;
        bool m_enabled = false;
        bool m_remove  = false;
        TaskFunction m_function;
        Time_t m_process_time;
        uint32_t m_period;
    };

} // namespace CFXS