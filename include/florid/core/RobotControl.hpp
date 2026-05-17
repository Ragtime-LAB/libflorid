#ifndef FLORID_ROBOTCONTROL_HPP
#define FLORID_ROBOTCONTROL_HPP

namespace florid
{
    class RobotControl
    {
    public:
        RobotControl(bool* stop_flag) noexcept : m_stop_flag(stop_flag) {}

        void finishMotion() noexcept
        {
            if (m_finish_flag) *m_finish_flag = true;
        }

        void stopControl() noexcept
        {
            if (m_stop_flag) *m_stop_flag = true;
        }

        void setFinishFlag(bool* flag) noexcept { m_finish_flag = flag; }

        bool isStopped() const noexcept { return m_stop_flag && *m_stop_flag; }
        bool isFinished() const noexcept { return m_finish_flag && *m_finish_flag; }

    private:
        bool* m_finish_flag{nullptr};
        bool* m_stop_flag{nullptr};
    };

} // namespace florid

#endif // FLORID_ROBOTCONTROL_HPP
