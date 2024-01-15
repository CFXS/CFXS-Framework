// [CFXS] //
#include <CFXS/Task.hpp>
#include <CFXS/CPU.hpp>
#include <CFXS/Debug.hpp>
#include <CFXS/Time.hpp>

#include <stl/array>

namespace CFXS {

    using Group_t = Task::Group_t;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    static bool s_GlobalEnable = false;

    struct TaskGroupEntry {
        Task* currentTask;
        Task** tasks;
        size_t capacity;
        bool exists = false;
    };
    static stl::array<TaskGroupEntry, Task::MAX_GROUP_INDEX> s_TaskGroups;
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool GroupExists(Group_t group) {
        if (group >= Task::MAX_GROUP_INDEX) {
            CFXS_ERROR("Invalid task group index");
            return false;
        }
        return s_TaskGroups[group].exists;
    }

    static bool GroupFull(Group_t group) {
        if (group >= Task::MAX_GROUP_INDEX) {
            CFXS_ERROR("Invalid task group index");
            return false;
        }

        auto& taskGroup = s_TaskGroups[group];
        for (size_t i = 0; i < taskGroup.capacity; i++) {
            if (taskGroup.tasks[i] == nullptr)
                return false;
        }
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Static

    __used Task** s_TestTaskArray = nullptr;
    void Task::EnableProcessing() {
        asm volatile("" ::"m"(s_TestTaskArray));

        s_GlobalEnable = true;
    }

    // TODO: placement option - pass preallocated region
    bool Task::AddGroup(Group_t group, size_t capacity) {
        CFXS_ASSERT(group < Task::MAX_GROUP_INDEX, "Invalid task group index");
        if (s_GlobalEnable) {
            CFXS_ERROR("[Task] Task processing is enabled - group add not allowed");
            return false;
        }

        if (GroupExists(group)) {
            CFXS_ERROR("[Task] Group %u already exists", group);
            return false;
        } else {
            auto& taskGroup = s_TaskGroups[group];
            taskGroup.tasks = new Task*[capacity];
            if (taskGroup.tasks) {
                memset(taskGroup.tasks, 0, sizeof(Task*) * capacity);
                taskGroup.capacity = capacity;
                taskGroup.exists   = true;
                // CFXS_Task_printf(DebugLevel::TRACE, "Add task group [%u] with capacity %u\n", group, capacity);
                return true;
            } else {
                CFXS_ERROR("Failed to create task group %u with capacity %u\n", group, capacity);
                return false;
            }
        }
    }

    void Task::ProcessGroup(Group_t group) {
        if (!s_GlobalEnable) {
            // CFXS_Task_printf(DebugLevel::WARNING, "Task processing not enabled (group %d)\n", group);
            return;
        }

        if (!GroupExists(group)) {
            CFXS_ERROR("[Task] Group %u does not exist\n", group);
            return;
        }

        auto& taskGroup = s_TaskGroups[group];
        for (size_t i = 0; i < taskGroup.capacity; i++) {
            auto* task = taskGroup.tasks[i];
            if (!task || !task->m_Enabled)
                continue;

            if (CFXS::Time::ms >= task->m_ProcessTime) {
                taskGroup.currentTask = task;

                switch (task->m_Type) {
                    case Type::PERIODIC: {
                        task->m_ProcessTime = CFXS::Time::ms + task->m_Period;
                        if (task->m_Function)
                            task->m_Function();
                        if (task->m_Delete) {
                            taskGroup.tasks[i] = nullptr;
                            delete task;
                        }
                        break;
                    }
                    case Type::SINGLE_SHOT: {
                        if (task->m_Function)
                            task->m_Function();
                        taskGroup.tasks[i] = nullptr;
                        delete task;
                        break;
                    }
                    default: CFXS_ERROR("[Task] Unknown type %u (group %u)\n", task->m_Type, group);
                }
            }
        }
    }

    static void InsertTask(Group_t group, Task* task) {
        auto& taskGroup = s_TaskGroups[group];
        for (size_t i = 0; i < taskGroup.capacity; i++) {
            if (taskGroup.tasks[i] == nullptr) {
                taskGroup.tasks[i] = task;
                return;
            }
        }
    }

    Task* Task::Create(Group_t group, const char* name, const TaskFunction& func, uint32_t period) {
        if (!GroupExists(group)) {
            CFXS_ERROR("[Task::Create] Group %u does not exist\n", group);
            return nullptr;
        }
        if (GroupFull(group)) {
            CFXS_ERROR("[Task::Create] Group %u is full\n", group);
            return nullptr;
        }

        Task* task;
        {
            CFXS::CPU::NoInterruptScope _;
            task = new Task(group, name, func, Type::PERIODIC, period);
            InsertTask(group, task);
        }

        if (task) {
            task->m_ProcessTime = 0;
            // CFXS_Task_printf(DebugLevel::TRACE, "Create task \"%s\" @ %lums in group [%u]\n", name, period, group);
        } else {
            CFXS_ERROR("[Task] Failed to create task");
        }

        return task;
    }

    bool Task::Queue(Group_t group, const char* name, const TaskFunction& func, uint32_t delay) {
        if (!GroupExists(group)) {
            CFXS_ERROR("[Task::Queue] Group %u does not exist\n", group);
            return false;
        }
        if (GroupFull(group)) {
            CFXS_ERROR("[Task::Queue] Group %u is full\n", group);
            return false;
        }

        Task* task;
        {
            CFXS::CPU::NoInterruptScope _;
            task = new Task(group, name, func, Type::SINGLE_SHOT, delay);
            InsertTask(group, task);
        }

        task->m_Enabled = true;

        return task;
    }

    Task* Task::GetCurrentTask(Group_t group) {
        if (!GroupExists(group)) {
            CFXS_ERROR("[Task] Group %u does not exist\n", group);
            return nullptr;
        }

        return s_TaskGroups[group].currentTask;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Member

    Task::Task(Group_t group, const char* name, const TaskFunction& func, Type type, uint32_t period) :
#if CFXS_TASK_NAME_FIELD == 1
        m_Name(name),
#endif
        m_Group(group),
        m_Type(type),
        m_Function(func),
        m_ProcessTime(Time::ms + period),
        m_Period(period) {
    }

    void Task::Task::SetGroup(Group_t group) {
        if (GroupExists(group)) {
            m_Group = group;
        } else {
            CFXS_ERROR("[Task] Group %u does not exist\n", group);
        }
    }

} // namespace CFXS