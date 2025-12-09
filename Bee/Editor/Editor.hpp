/**
 * @File Editor.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/2
 * @Brief This file is part of Bee.
 */

#pragma once

namespace bee
{

class Editor
{
public:
    Editor();
    ~Editor();

    // TODO: 删除拷贝的宏

    bool init(int argc, char* argv[]);
    int run();

private:
    bool _running = false;
};

} // namespace bee
