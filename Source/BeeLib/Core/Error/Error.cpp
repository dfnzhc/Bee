/**
 * @File Error.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/21
 * @Brief This file is part of Bee.
 */

#include "./Error.hpp"

namespace Bee
{
    constexpr std::string_view GetErrorDomainName(ErrorDomain domain) noexcept
    {
        #define DOMAIN_CASE(name) case ErrorDomain::name: return #name

        switch (domain)
        {
            DOMAIN_CASE(NoError);
            DOMAIN_CASE(System);
            DOMAIN_CASE(Memory);
            DOMAIN_CASE(FileSystem);
            DOMAIN_CASE(Threading);
            DOMAIN_CASE(Core);
            DOMAIN_CASE(Platform);
            DOMAIN_CASE(Graphics);
            DOMAIN_CASE(Engine);
            DOMAIN_CASE(Application);
            DOMAIN_CASE(Physics);
            DOMAIN_CASE(Animation);
            DOMAIN_CASE(Resource);
            DOMAIN_CASE(Editor);
            DOMAIN_CASE(Unknown);
        }

        #undef DOMAIN_CASE

        return "";
    }

    std::string ErrorId::hex() const
    {
        return std::format("{:08X}", value());
    }

    std::string ErrorId::format() const
    {
        return std::format("{}:{:04X}", GetErrorDomainName(getDomain()), code);
    }
} // namespace Bee
