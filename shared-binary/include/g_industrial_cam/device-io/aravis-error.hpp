#include <exception>
#include <system_error>
#include <cstdint>
#include <string>

#include <arv.h>

namespace g_industrial_cam
{

    class aravis_error
    {
    public:
        aravis_error();
        ~aravis_error();
        operator GError **();
        static void check(const aravis_error &err);
        std::string message() const;
        int32_t code() const;

    private:
        GError *m_err;
    };

    class aravis_error_exception : public std::system_error
    {
    public:
        aravis_error_exception(const aravis_error &e);
    };

}