#include <iostream>
#include "Tensor/Tensor.hpp"
#include "Tensor/Ops/Cast.hpp"
int main() {
    try {
        auto src = bee::Tensor::full({4}, bee::DType::F32, 2.0, bee::Device::CUDA);
        if (!src) { std::cout << "full_err: " << src.error().format() << "\n"; return 1; }
        auto f16 = bee::cast(*src, bee::DType::F16);
        if (!f16) { std::cout << "cast_err: " << f16.error().format() << "\n"; return 2; }
        auto back = bee::cast(*f16, bee::DType::F32);
        if (!back) { std::cout << "back_err: " << back.error().format() << "\n"; return 3; }
        auto cpu = back->to(bee::Device::CPU);
        if (!cpu) { std::cout << "cpu_err: " << cpu.error().format() << "\n"; return 4; }
        auto* p = static_cast<const float*>(cpu->data_ptr());
        std::cout << p[0] << "," << p[3] << "\n";
        return 0;
    } catch (...) {
        std::cout << "exception\n";
        return 5;
    }
}
