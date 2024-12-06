/**
 * @File FileDialog.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"

#include <string>
#include <vector>

namespace bee {

struct BEE_API FileDialogFilter
{
    FileDialogFilter(std::string ext_, std::string desc_ = {}) : desc(std::move(desc_)), ext(std::move(ext_)) { }

    std::string desc; // The description ("Portable Network Graphics")
    std::string ext;  // The extension, without the `.` ("png")
};

using FileDialogFilterVec = std::vector<FileDialogFilter>;

} // namespace bee