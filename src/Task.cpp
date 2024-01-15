// [CFXS] //
#include <CFXS/Task.hpp>
#include <CFXS/CPU.hpp>
#include <CFXS/Debug.hpp>
#include <CFXS/Time.hpp>

#include <stl/array>

namespace CFXS {

    using Group_t = Task::Group_t;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    static bool g_global_enable = false;

    struct TaskGroupEntry {
        Task* current_task{};
        Task** tasks{};
        size_t capacity{};
        bool exists = false;
    };
    static stl::array<TaskGroupEntry, Task::MAX_GROUP_INDEX> g_task_groups;
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool group_exists(Group_t group) {
        if (group >= Task::MAX_GROUP_INDEX) {
            CFXS_ERROR("Invalid task group index");
            return false;
        }
        return g_task_groups[group].exists;
     }

     static bool group_full(Group_t group) {
        if (group >= Task::MAX_GROUP_INDEX) {
            CFXS_ERROR("Invalid task group index");
            return false;
        }

        auto& task_group = g_task_groups[group];
        for (size_t i = 0; i < task_group.capacity; i++) {
            if (task_group.tasks[i] == nullptr)
                return false;
        }
        return true;
     }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Static

    void Task::enable_processing() {
         g_global_enable = true;
    }

    // TODO: placement option - pass preallocated region
    bool Task::add_group(Group_t group, size_t capacity) {
        CFXS_ASSERT(group < Task::MAX_GROUP_INDEX, "Invalid task group index");
        if (g_global_enable) {
             CFXS_ERROR("[Task] Task processing is enabled - group add not allowed");
             return false;
        }

        if (group_exists(group)) {
            CFXS_ERROR("[Task] Group %u already exists", group);
            return false;
        }

        auto& task_group = g_task_groups[group];
        task_group.tasks = new Task*[capacity];
        if (task_group.tasks) {
            memset(task_group.tasks, 0, sizeof(Task*) * capacity);
            task_group.capacity = capacity;
            task_group.exists   = true;
            // CFXS_Task_printf(DebugLevel::TRACE, "Add task group [%u] with capacity %u\n", group, capacity);
            return true;
        }

        CFXS_ERROR("Failed to create task group %u with capacity %u\n", group, capacity);
        return false;
    }

    void Task::process_group(Group_t group) {
        if (!g_global_enable) {
            // CFXS_Task_printf(DebugLevel::WARNING, "Task processing not enabled (group %d)\n", group);
            return;
        }

        if (!group_exists(group)) {
            CFXS_ERROR("[Task] Group %u does not exist\n", group);
            return;
        }

        auto& task_group = g_task_groups[group];
        for (size_t i = 0; i < task_group.capacity; i++) {
            auto* task = task_group.tasks[i];
            if (!task || !task->m_enabled)
                 continue;

            if (CFXS::Time::ms >= task->m_process_time) {
                task_group.current_task = task;

                switch (task->m_type) {
                    case Type::PERIODIC: {
                        task->m_process_time = CFXS::Time::ms + task->m_period;
                        if (task->m_function)
                            task->m_function();
                        if (task->m_remove) {
                            task_group.tasks[i] = nullptr;
                            delete task;
                        }
                        break;
                    }
                    case Type::SINGLE_SHOT: {
                        if (task->m_function)
                            task->m_function();
                        task_group.tasks[i] = nullptr;
                        delete task;
                        break;
                    }
                    default: CFXS_ERROR("[Task] Unknown type %u (group %u)\n", task->m_type, group);
                }
            }
        }
    }

    static void insert_task(Group_t group, Task* task) {
        auto& task_group = g_task_groups[group];
        for (size_t i = 0; i < task_group.capacity; i++) {
            if (task_group.tasks[i] == nullptr) {
                task_group.tasks[i] = task;
                return;
            }
        }
    }

    Task* Task::create(Group_t group, const char* name, const TaskFunction& func, uint32_t period) {
        if (!group_exists(group)) {
            CFXS_ERROR("[Task::Create] Group %u does not exist\n", group);
            return nullptr;
        }
        if (group_full(group)) {
            CFXS_ERROR("[Task::Create] Group %u is full\n", group);
            return nullptr;
        }

        Task* task = nullptr;
        {
            CFXS::CPU::NoInterruptScope _;
            task = new Task(group, name, func, Type::PERIODIC, period);
            insert_task(group, task);
        }

        if (task) {
            task->m_process_time = 0;
            // CFXS_Task_printf(DebugLevel::TRACE, "Create task \"%s\" @ %lums in group [%u]\n", name, period, group);
        } else {
            CFXS_ERROR("[Task] Failed to create task");
        }

        return task;
    }

    bool Task::queue(Group_t group, const char* name, const TaskFunction& func, uint32_t delay) {
        if (!group_exists(group)) {
            CFXS_ERROR("[Task::Queue] Group %u does not exist\n", group);
            return false;
        }
        if (group_full(group)) {
            CFXS_ERROR("[Task::Queue] Group %u is full\n", group);
            return false;
        }

        Task* task = nullptr;
        {
            CFXS::CPU::NoInterruptScope _;
            task = new Task(group, name, func, Type::SINGLE_SHOT, delay);
            insert_task(group, task);
        }

        task->m_enabled = true;

        return task;
    }

    Task* Task::get_current_task(Group_t group) {
        if (!group_exists(group)) {
            CFXS_ERROR("[Task] Group %u does not exist\n", group);
            return nullptr;
        }

        return g_task_groups[group].current_task;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Member

    Task::Task(Group_t group, __maybe_unused const char* name, const TaskFunction& func, Type type, uint32_t period) :
#if CFXS_TASK_NAME_FIELD == 1
        m_Name(name),
#endif
        m_group(group),
        m_type(type),
        m_function(func),
        m_process_time(Time::ms + period),
        m_period(period) {
    }

    void Task::Task::set_group(Group_t group) {
        if (group_exists(group)) {
            m_group = group;
        } else {
            CFXS_ERROR("[Task] Group %u does not exist\n", group);
        }
    }

} // namespace CFXS