/**
 * @File Bee.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/13
 * @Brief This file is part of Bee.
 */

#pragma once

#include <vector>
#include <string>
#include <unordered_set>

#include <iostream>
#include <memory>
#include <thread>
#include <expected>
#include <mutex>
#include <optional>
#include <functional>
#include <source_location>
#include <algorithm>
#include <utility>
#include <ranges>
#include <any>
#include <queue>
#include <variant>

#include "Config.hpp"

#include "Core/Defines.hpp"
#include "Core/Error.hpp"
#include "Core/Macros.hpp"
#include "Core/Portability.hpp"
#include "Core/Thirdparty.hpp"
#include "Core/Version.hpp"
#include "Core/Logger.hpp"

#include "Core/App/Application.hpp"
#include "Core/App/Property.hpp"
#include "Core/Concepts/NonCopyable.hpp"

#include "Math/Math.hpp"
#include "Math/Constant.hpp"
#include "Math/Common.hpp"
#include "Math/Hash.hpp"
#include "Math/Polynomial.hpp"
