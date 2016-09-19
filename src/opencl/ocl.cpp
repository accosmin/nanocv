#include "ocl.h"
#include "logger.h"
#include "kernels.h"
#include "math/numeric.hpp"
#include <map>
#include <fstream>
#include <sstream>

namespace nano
{
        struct manager_t
        {
                manager_t();

                bool select(const cl_device_type);
                void select(const cl::Device&);

                std::vector<cl::Platform>       m_platforms;    ///< available platforms
                std::vector<cl::Device>         m_devices;      ///< available devices for all platforms
                cl::Device                      m_device;       ///< selected device
                cl::Context                     m_context;      ///< context for the selected device
                cl::CommandQueue                m_queue;        ///< command queue for the selected context
                cl::Program                     m_program;      ///< built-in kernels
        };

        static manager_t theocl;

        manager_t::manager_t()
        {
                const cl_int ret = cl::Platform::get(&m_platforms);
                if (m_platforms.empty() || ret != CL_SUCCESS)
                {
                        log_error() << "cannot find any OpenCL platform!";
                        throw cl::Error(ret);
                }

                for (size_t i = 0; i < m_platforms.size(); ++ i)
                {
                        const cl::Platform& platform = m_platforms[i];

#define LOG() \
                        log_info() << "OpenCL platform [" << (i + 1) << "/" << m_platforms.size() << "]: "
                        LOG() << "CL_PLATFORM_NAME: " << platform.getInfo<CL_PLATFORM_NAME>();
                        LOG() << "CL_PLATFORM_VENDOR: " << platform.getInfo<CL_PLATFORM_NAME>();
                        LOG() << "CL_PLATFORM_VERSION: " << platform.getInfo<CL_PLATFORM_NAME>();
#undef LOG

                        std::vector<cl::Device> devices;
                        const cl_int ret = platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
                        if (devices.empty() || ret != CL_SUCCESS)
                        {
                                log_error() << "cannot find any OpenCL device for the current platform!";
                                throw cl::Error(ret);
                        }

                        m_devices.insert(m_devices.end(), devices.begin(), devices.end());

                        for (size_t j = 0; j < devices.size(); j ++)
                        {
                                const cl::Device& device = devices[j];

                                const size_t gmemsize = device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
                                const size_t lmemsize = device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
                                const size_t amemsize = device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
                                const size_t cmemsize = device.getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>();
                                const size_t maxcus = device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                                const size_t maxwgsize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
                                const size_t maxwidims = device.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>();
                                const std::vector<size_t> maxwisizes = device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
                                const size_t maxkparams = device.getInfo<CL_DEVICE_MAX_PARAMETER_SIZE>();

#define LOG() \
                                log_info() << "-> OpenCL device [" << (j + 1) << "/" << devices.size() << "]: "
                                LOG() << "CL_DEVICE_NAME: " << device.getInfo<CL_DEVICE_NAME>();
                                LOG() << "CL_DEVICE_VENDOR:" << device.getInfo<CL_DEVICE_VENDOR>();
                                LOG() << "CL_DRIVER_VERSION: " << device.getInfo<CL_DRIVER_VERSION>();
                                LOG() << "CL_DEVICE_VERSION: " << device.getInfo<CL_DEVICE_VERSION>();
                                LOG() << "CL_DEVICE_TYPE: " << ocl::device_type_string(device.getInfo<CL_DEVICE_TYPE>());
                                LOG() << "CL_DEVICE_GLOBAL_MEM_SIZE: " << gmemsize << "B = "
                                           << (gmemsize / 1024) << "KB = " << (gmemsize / 1024 / 1024) << "MB";
                                LOG() << "CL_DEVICE_LOCAL_MEM_SIZE: " << lmemsize << "B = "
                                           << (lmemsize / 1024) << "KB = " << (lmemsize / 1024 / 1024) << "MB";
                                LOG() << "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE: " << cmemsize << "B = "
                                           << (cmemsize / 1024) << "KB = " << (cmemsize / 1024 / 1024) << "MB";
                                LOG() << "CL_DEVICE_MAX_MEM_ALLOC_SIZE: " << amemsize << "B = "
                                           << (amemsize / 1024) << "KB = " << (amemsize / 1024 / 1024) << "MB";
                                LOG() << "CL_DEVICE_MAX_COMPUTE_UNITS: " << maxcus;
                                LOG() << "CL_DEVICE_MAX_WORK_GROUP_SIZE: " << maxwgsize;
                                LOG() << "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: " << maxwidims;
                                LOG() << "CL_DEVICE_MAX_WORK_ITEM_SIZES: "
                                           << (maxwisizes.size() > 0 ? maxwisizes[0] : 0) << " / "
                                           << (maxwisizes.size() > 1 ? maxwisizes[1] : 0) << " / "
                                           << (maxwisizes.size() > 2 ? maxwisizes[2] : 0);
                                LOG() << "CL_DEVICE_MAX_PARAMETER_SIZE: " << maxkparams;
#undef LOG
                        }
                }
        }

        bool manager_t::select(const cl_device_type type)
        {
                // <number of compute units, index>
                std::map<size_t, size_t, std::greater<size_t>> cuByIndex;
                for (size_t i = 0; i < m_devices.size(); ++ i)
                {
                        const cl::Device& device = m_devices[i];
                        cuByIndex[device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>()] = i;
                }

                // select the device with the maximum number of compute units and the given type (if possible)
                for (const auto& cu : cuByIndex)
                {
                        const cl::Device& device = m_devices[cu.second];
                        if (    type == CL_DEVICE_TYPE_ALL ||
                                device.getInfo<CL_DEVICE_TYPE>() == type)
                        {
                                select(device);
                                return true;
                        }
                }

                if (!cuByIndex.empty())
                {
                        const auto& cu = *cuByIndex.begin();
                        select(m_devices[cu.second]);
                        return true;
                }
                else
                {
                        return false;
                }
        }

        void manager_t::select(const cl::Device& device)
        {
                log_info()
                        << "selected OpenCL device " << device.getInfo<CL_DEVICE_NAME>()
                        << " of type " << ocl::device_type_string(device.getInfo<CL_DEVICE_TYPE>()) << ".";

                m_device = device;
                m_context = cl::Context({m_device});
                m_queue = cl::CommandQueue(m_context, m_device, 0);
                m_program = ocl::make_program_from_text(opencl_kernels());
        }

        bool ocl::select(const cl_device_type type)
        {
                return theocl.select(type);
        }

        static cl::CommandQueue& queue()
        {
                return theocl.m_queue;
        }

        cl::Program ocl::make_program_from_file(const std::string& filepath)
        {
                std::ifstream file(filepath, std::ios::in);

                if (file.is_open())
                {
                        std::ostringstream oss;
                        oss << file.rdbuf();
                        return ocl::make_program_from_text(oss.str());
                }

                else
                {
                        return ocl::make_program_from_text("cannot load file!");
                }
        }

        cl::Program ocl::make_program_from_text(const std::string& source)
        {
                cl::Program::Sources sources(1, source);
                cl::Program program = cl::Program(theocl.m_context, sources);

                try
                {
                        program.build({theocl.m_device}, "-cl-mad-enable -Werror");//, "-cl-fast-relaxed-math");
                }
                catch (cl::Error& e)
                {
                        // load compilation errors
                        const cl::Device& device = theocl.m_device;
                        log_error() << "OpenCL program build status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(device);
                        log_error() << "OpenCL program build options: " << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(device);
                        log_error() << "OpenCL program build log: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);

                        // and re-throw the exception
                        throw cl::Error(e.err());
                }

                return program;
        }

        cl::Kernel ocl::make_kernel(const char* name)
        {
                return ocl::make_kernel(theocl.m_program, name);
        }

        cl::Kernel ocl::make_kernel(const cl::Program& program, const char* name)
        {
                try
                {
                        return cl::Kernel(program, name);
                }
                catch (cl::Error& e)
                {
                        // load kernel errors
                        log_error() << "OpenCL kernel error: " << error_string(e.err());

                        // and re-throw the exception
                        throw cl::Error(e.err());
                }
        }

        template <>
        cl::Buffer ocl::make_buffer<size_t>(const size_t& bytesize, cl_mem_flags flags)
        {
                return cl::Buffer(theocl.m_context, flags, bytesize);
        }

        const char* ocl::device_type_string(const cl_device_type type)
        {
                switch (type)
                {
                case CL_DEVICE_TYPE_DEFAULT:    return "DEFAULT";
                case CL_DEVICE_TYPE_CPU:        return "CPU";
                case CL_DEVICE_TYPE_GPU:        return "GPU";
                case CL_DEVICE_TYPE_ACCELERATOR:return "ACCELERATOR";
                case CL_DEVICE_TYPE_ALL:        return "ALL";
                default:                        return "UNKNOWN";
                }
        }

        const char* ocl::error_string(const cl_int error)
        {
                static const char* errorString[] =
                {
                        "CL_SUCCESS",                                   // 0
                        "CL_DEVICE_NOT_FOUND",                          // -1
                        "CL_DEVICE_NOT_AVAILABLE",                      // -2
                        "CL_COMPILER_NOT_AVAILABLE",                    // -3
                        "CL_MEM_OBJECT_ALLOCATION_FAILURE",             // -4
                        "CL_OUT_OF_RESOURCES",                          // -5
                        "CL_OUT_OF_HOST_MEMORY",                        // -6
                        "CL_PROFILING_INFO_NOT_AVAILABLE",              // -7
                        "CL_MEM_COPY_OVERLAP",                          // -8
                        "CL_IMAGE_FORMAT_MISMATCH",                     // -9
                        "CL_IMAGE_FORMAT_NOT_SUPPORTED",                // -10
                        "CL_BUILD_PROGRAM_FAILURE",                     // -11
                        "CL_MAP_FAILURE",                               // -12
                        "CL_MISALIGNED_SUB_BUFFER_OFFSET",              // -13
                        "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST", // -14
                        "CL_COMPILE_PROGRAM_FAILURE",                   // -15
                        "CL_LINKER_NOT_AVAILABLE",                      // -16
                        "CL_LINK_PROGRAM_FAILURE",                      // -17
                        "CL_DEVICE_PARTITION_FAILED",                   // -18
                        "CL_KERNEL_ARG_INFO_NOT_AVAILABLE",             // -19
                        "",
                        "",
                        "",
                        "",
                        "",
                        "",
                        "",
                        "",
                        "",
                        "",
                        "CL_INVALID_VALUE",                             // -30
                        "CL_INVALID_DEVICE_TYPE",                       // -31
                        "CL_INVALID_PLATFORM",                          // -32
                        "CL_INVALID_DEVICE",                            // -33
                        "CL_INVALID_CONTEXT",                           // -34
                        "CL_INVALID_QUEUE_PROPERTIES",                  // -35
                        "CL_INVALID_COMMAND_QUEUE",                     // -36
                        "CL_INVALID_HOST_PTR",                          // -37
                        "CL_INVALID_MEM_OBJECT",                        // -38
                        "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",           // -39
                        "CL_INVALID_IMAGE_SIZE",                        // -40
                        "CL_INVALID_SAMPLER",                           // -41
                        "CL_INVALID_BINARY",                            // -42
                        "CL_INVALID_BUILD_OPTIONS",                     // -43
                        "CL_INVALID_PROGRAM",                           // -44
                        "CL_INVALID_PROGRAM_EXECUTABLE",                // -45
                        "CL_INVALID_KERNEL_NAME",                       // -46
                        "CL_INVALID_KERNEL_DEFINITION",                 // -47
                        "CL_INVALID_KERNEL",                            // -48
                        "CL_INVALID_ARG_INDEX",                         // -49
                        "CL_INVALID_ARG_VALUE",                         // -50
                        "CL_INVALID_ARG_SIZE",                          // -51
                        "CL_INVALID_KERNEL_ARGS",                       // -52
                        "CL_INVALID_WORK_DIMENSION",                    // -53
                        "CL_INVALID_WORK_GROUP_SIZE",                   // -54
                        "CL_INVALID_WORK_ITEM_SIZE",                    // -55
                        "CL_INVALID_GLOBAL_OFFSET",                     // -56
                        "CL_INVALID_EVENT_WAIT_LIST",                   // -57
                        "CL_INVALID_EVENT",                             // -58
                        "CL_INVALID_OPERATION",                         // -59
                        "CL_INVALID_GL_OBJECT",                         // -60
                        "CL_INVALID_BUFFER_SIZE",                       // -61
                        "CL_INVALID_MIP_LEVEL",                         // -62
                        "CL_INVALID_GLOBAL_WORK_SIZE",                  // -63
                        "CL_INVALID_PROPERTY",                          // -64
                        "CL_INVALID_IMAGE_DESCRIPTOR",                  // -65
                        "CL_INVALID_COMPILER_OPTIONS",                  // -66
                        "CL_INVALID_LINKER_OPTIONS",                    // -67
                        "CL_INVALID_DEVICE_PARTITION_COUNT",            // -68
                        "CL_INVALID_PIPE_SIZE",                         // -69
                        "CL_INVALID_DEVICE_QUEUE"                       // -70
                };

                static const int errorCount = sizeof(errorString) / sizeof(errorString[0]);

                const int index = -error;
                return (index >= 0 && index < errorCount) ? errorString[index] : "UNKNOWN";
        }

        cl_int ocl::read(const cl::Buffer& buffer, const cl_bool blocking, const size_t bytes, void* ptr)
        {
                return queue().enqueueReadBuffer(buffer, blocking, 0, bytes, ptr);
        }

        cl_int ocl::write(const cl::Buffer& buffer, const cl_bool blocking, const size_t bytes, const void* ptr)
        {
                return queue().enqueueWriteBuffer(buffer, blocking, 0, bytes, ptr);
        }

        static size_t gsize(const size_t dims, const size_t wgsize)
        {
                return ((dims + wgsize - 1) / wgsize) * wgsize;
        }

        template <>
        cl::Event ocl::enqueue<size_t>(const cl::Kernel& kernel, const size_t dims1)
        {
                const auto maxwgsize = theocl.m_device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
                const auto wgsize1 = maxwgsize; //(dims1 % maxwgsize) ? 1 : maxwgsize;

                const auto offset = cl::NullRange;
                const auto global = cl::NDRange(idiv(idiv(dims1, 4), wgsize1) * wgsize1);
                const auto local = cl::NDRange(wgsize1);

                cl::Event event;
                queue().enqueueNDRangeKernel(kernel, offset, global, local, nullptr, &event);
                return event;
        }

        template <>
        cl::Event ocl::enqueue<size_t>(const cl::Kernel& kernel, const size_t dims1, const size_t dims2)
        {
                const auto maxwgsize = theocl.m_device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() / 8;
                const auto wgsize1 = static_cast<size_t>(std::sqrt(maxwgsize));
                const auto wgsize2 = maxwgsize / wgsize1;//static_cast<size_t>(std::sqrt(maxwgsize));

                const auto offset = cl::NullRange;
                const auto global = cl::NDRange(gsize(dims1, wgsize1), gsize(dims2, wgsize2));
                const auto local = cl::NDRange(wgsize1, wgsize2);

                cl::Event event;
                queue().enqueueNDRangeKernel(kernel, offset, global, local, nullptr, &event);
                return event;
        }

        void ocl::wait()
        {
                queue().finish();
        }

        void ocl::wait(const cl::Event& event)
        {
                event.wait();
        }
}

