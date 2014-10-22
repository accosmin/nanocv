#include "io_arch.h"
#include "io_bzip.h"
#include "io_gzip.h"
#include "logger.h"
#include <fstream>
#include <cmath>
#include <cstdint>
#include <boost/algorithm/string.hpp>

namespace ncv
{
        // http://techoverflow.net/blog/2013/03/29/reading-tar-files-in-c/

        enum class archive_type : int
        {
                tar,
                tar_gz,
                tar_bz2,
                gz,
                bz2,
                unknown
        };

        static archive_type decode_archive_type(const std::string& path)
        {
                if (    boost::algorithm::iends_with(path, ".tar.gz") ||
                        boost::algorithm::iends_with(path, ".tgz"))
                {
                        return archive_type::tar_gz;
                }

                else if (boost::algorithm::iends_with(path, ".tar.bz2") ||
                         boost::algorithm::iends_with(path, ".tbz") ||
                         boost::algorithm::iends_with(path, ".tbz2") ||
                         boost::algorithm::iends_with(path, ".tb2"))
                {
                        return archive_type::tar_bz2;
                }

                else if (boost::algorithm::iends_with(path, ".tar"))
                {
                        return archive_type::tar;
                }

                else if (boost::algorithm::iends_with(path, ".gz"))
                {
                        return archive_type::gz;
                }

                else
                {
                        return archive_type::unknown;
                }
        }

        using std::int64_t;
        using std::uint64_t;

        ///
        /// \brief convert an ascii digit to the corresponding number (assuming it is an ASCII digit)
        ///
        static uint64_t ascii_to_number(unsigned char num)
        {
                return ((num) - 48);
        }

        ///
        /// \brief decode a TAR octal number. ignores everything after the first NUL or space character.
        ///
        static uint64_t decode_tar_octal(const char* data, size_t size = 12)
        {
                const unsigned char* currentPtr = (const unsigned char*)data + size;

                const unsigned char* checkPtr = currentPtr;
                for (; checkPtr >= (const unsigned char*) data; checkPtr --)
                {
                        if ((*checkPtr) == 0 || (*checkPtr) == ' ')
                        {
                                currentPtr = checkPtr - 1;
                        }
                }

                uint64_t sum = 0;
                uint64_t currentMultiplier = 1;
                for (; currentPtr >= (const unsigned char*)data; currentPtr --)
                {
                        sum += ascii_to_number(*currentPtr) * currentMultiplier;
                        currentMultiplier *= 8;
                }

                return sum;
        }

        struct TARFileHeader
        {
                char filename[100];
                char mode[8];
                char uid[8];
                char gid[8];
                char fileSize[12];
                char lastModification[12];
                char checksum[8];
                char typeFlag;
                char linkedFileName[100];
                char ustarIndicator[6];
                char ustarVersion[2];
                char ownerUserName[32];
                char ownerGroupName[32];
                char deviceMajorNumber[8];
                char deviceMinorNumber[8];
                char filenamePrefix[155];
                char padding[12];

                size_t filesize() const
                {
                        return decode_tar_octal(fileSize);
                }
        };

        static bool io_decode(const io::data_t& orig_data, const std::string& path, io::data_t& data)
        {
                const archive_type type = decode_archive_type(path);

                switch (type)
                {
                case archive_type::tar_gz:
                case archive_type::gz:
                        return io::uncompress_gzip(orig_data, data);

                case archive_type::tar_bz2:
                case archive_type::bz2:
                        return io::uncompress_bzip2(orig_data, data);

                case archive_type::tar:
                default:
                        return !(data = orig_data).empty();
                }
        }

        static bool io_decode(std::istream& istream, const std::string& path, io::data_t& data)
        {
                const archive_type type = decode_archive_type(path);

                switch (type)
                {
                case archive_type::tar_gz:
                case archive_type::gz:
                        return io::uncompress_gzip(istream, data);

                case archive_type::tar_bz2:
                case archive_type::bz2:
                        return io::uncompress_bzip2(istream, data);

                case archive_type::tar:
                default:
                        return io::load_binary(istream, data);
                }
        }

        static bool io_untar(const io::data_t& data, const std::string& log_header, const io::data_callback_t& callback)
        {
                char zeroBlock[512];
                memset(zeroBlock, 0, 512);

                bool nextEntryHasLongName = false;

                for (size_t pos = 0; pos < data.size(); )
                {
                        TARFileHeader header;
                        if (!io::load_struct(data, header, pos))
                        {
                                log_error() << log_header << "failed to read TAR header!";
                                return false;
                        }
                        if (memcmp(&header, zeroBlock, 512) == 0)
                        {
                                log_info() << log_header << "found TAR end.";
                                break;
                        }

                        // compose the filename
                        std::string filename(header.filename, std::min((size_t)100, strlen(header.filename)));
                        const size_t prefixLength = strlen(header.filenamePrefix);
                        if (prefixLength > 0)
                        {
                                filename =
                                std::string(header.filenamePrefix, std::min((size_t)155, prefixLength)) +
                                "/" +
                                filename;
                        }

                        if (header.typeFlag == '0' || header.typeFlag == 0)
                        {
                                // handle GNU TAR long filenames
                                if (nextEntryHasLongName)
                                {
                                        filename = std::string(header.filename);
                                        if (!io::load_struct(data, header, pos))
                                        {
                                                log_error() << log_header << "failed to read TAR header!";
                                                return false;
                                        }
                                        nextEntryHasLongName = false;
                                }

                                const size_t filesize = header.filesize();
                                log_info() << log_header << "found file <" << filename << "> (" << filesize << " bytes).";

                                // read the file into memory
                                io::data_t orig_filedata;
                                if (!io::load_data(data, filesize, orig_filedata, pos))
                                {
                                        log_error() << log_header << "failed to read TAR data!";
                                        return false;
                                }

                                io::data_t filedata;
                                if (!io_decode(orig_filedata, filename, filedata))
                                {
                                        log_error() << log_header << "failed to decode TAR data!";
                                        return false;
                                }

                                callback(filename, filedata);

                                // ignore padding
                                const size_t paddingBytes = (512 - (filesize % 512)) % 512;
                                if (!io::load_skip(data, paddingBytes, pos))
                                {
                                        log_error() << log_header << "failed to skip TAR padding!";
                                        return false;
                                }
                        }

                        else if (header.typeFlag == '5')
                        {
                                log_info() << log_header << "found directory <" << filename << ">.";
                        }

                        else if(header.typeFlag == 'L')
                        {
                                nextEntryHasLongName = true;
                        }

                        else
                        {
                                log_info() << log_header << "found unhandled TAR entry type <" << header.typeFlag << ">.";
                        }
                }

                // OK
                return true;
        }

        bool io::decode(const std::string& path, const std::string& log_header, const data_callback_t& callback)
        {
                std::ifstream in(path.c_str(), std::ios_base::in | std::ios_base::binary);
                if (!in.is_open())
                {
                        log_error() << log_header << "failed to open file <" << path << ">!";
                        return false;
                }

                data_t data;
                if (!io_decode(in, path, data))
                {
                        log_error() << log_header << "failed to load file <" << path << ">!";
                        return false;
                }

                const archive_type type = decode_archive_type(path);
                switch (type)
                {
                case archive_type::tar:
                case archive_type::tar_gz:
                case archive_type::tar_bz2:
                        return io_untar(data, log_header, callback);

                case archive_type::gz:
                        callback(path, data);
                        return true;

                case archive_type::bz2:
                        callback(path, data);
                        return true;

                case archive_type::unknown:
                default:
                        callback(path, data);
                        log_error() << log_header << "unknown file suffix <" << path << ">!";
                        return false;
                }
        }
}
