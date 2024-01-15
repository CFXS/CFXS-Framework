// [CFXS] //
#pragma once

#include <CFXS/Time.hpp>
#include <CFXS/Function.hpp>
#include <stl/initializer_list>

namespace CFXS {

    using TaskFunction = Function<void(void* userData)>;
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
        static void EnableProcessing();

        /////////////////////////////////////////////////////////////////////////////
        /// Create task group
        /// \param group task group
        /// \param capacity prealloc size and max task count in group
        /// \return bool - true if group created successfully
        static bool AddGroup(Group_t group, size_t capacity);

        static bool AddGroups(std::initializer_list<std::pair<Group_t, size_t>>&& groups) {
            bool addedAll = true;
            for (const auto& g : groups) {
                if (!AddGroup(g.first, g.second))
                    addedAll = false;
            }
            return addedAll;
        }

        /////////////////////////////////////////////////////////////////////////////
        /// Process all tasks in group
        /// \param group task group
        static void ProcessGroup(Group_t group);

        /////////////////////////////////////////////////////////////////////////////
        /// Create periodic task
        /// \param group task group
        /// \param name task label
        /// \param func function to queue
        /// \param period trigger period in ms
        /// \return Task* - pointer to created task or nullptr if creation failed
        static Task* Create(Group_t group, const char* name, const TaskFunction& func, uint32_t period);

        /////////////////////////////////////////////////////////////////////////////
        /// Queue single shot task
        /// \param group task group
        /// \param name task label
        /// \param func function to queue
        /// \param delay trigger delay from current timestamp
        /// \return bool - true if queued successfully
        static bool Queue(Group_t group, const char* name, const TaskFunction& func, uint32_t delay = 0);

        /////////////////////////////////////////////////////////////////////////////
        /// Get task currently being processed
        /// \param group task group
        /// \return Task* - pointer to current task being processed
        static Task* GetCurrentTask(Group_t group);

        const TaskFunction& GetFunction() const {
            return m_Function;
        }

        Group_t GetGroup() const {
            return m_Group;
        }
        void SetGroup(Group_t group);

        uint32_t GetPeriod() const {
            return m_Period;
        }
        void SetPeriod(uint32_t period) {
            m_Period = period;
        }

        void Delete() {
            m_Delete = true;
        }

        bool Enabled() const {
            return m_Enabled;
        }

        constexpr void SetEnabled(bool state) {
            if (!m_ProcessTime)
                m_ProcessTime = CFXS::Time::ms + m_Period;
            m_Enabled = state;
        }

        constexpr void Delay(CFXS::Time_t delay_ms) {
            m_ProcessTime += delay_ms;
        }

        constexpr void Start() {
            SetEnabled(true);
        }

        constexpr void Stop() {
            SetEnabled(false);
        }

        constexpr const char* GetName() const {
#if CFXS_TASK_NAME_FIELD == 1
            return m_Name;
#else
            return "N/A";
#endif
        }

    private:
        Task(Group_t group, const char* name, const TaskFunction& func, Type type, uint32_t period = 0);

    private:
#if CFXS_TASK_NAME_FIELD == 1
        const char* m_Name;
#endif
        Group_t m_Group;
        Type m_Type;
        bool m_Enabled = false;
        bool m_Delete  = false;
        TaskFunction m_Function;
        Time_t m_ProcessTime;
        uint32_t m_Period;
    };

} // namespace CFXS