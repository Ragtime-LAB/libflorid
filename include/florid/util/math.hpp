#ifndef FLORID_MATH_HPP
#define FLORID_MATH_HPP

namespace florid::comptime
{
    constexpr float MATH_PI = 3.14159265358979323846f;
    constexpr float MATH_PI_2 = 1.57079632679489661923f; // PI/2
    constexpr float MATH_PI_MUL_2 = 6.28318530717958647692f; // 2*PI


    int compile_time_error();

    constexpr float fabsf(const float x) {
        return x < 0.0f ? -x : x;
    }

    constexpr float powf(float x, int n) {
        if (n == 0) return 1.0f;
        if (n < 0) return 1.0f / powf(x, -n);
        float res = 1.0f;
        while (n > 0) {
            if (n % 2 != 0) res *= x;
            x *= x;
            n /= 2;
        }
        return res;
    }

    constexpr float floorf(float x) {
        const long long i = x;
        const float d = static_cast<float>(i);
        // 注意处理负数：-3.14 截断为 -3，但 floor 应该是 -4
        if (d == x || x >= 0.0f) return d;
        return d - 1.0f;
    }

    constexpr float ceilf(const float x) {
        const long long i = x;
        const float d = static_cast<float>(i);
        if (d == x || x < 0.0f) return d;
        return d + 1.0f;
    }

    constexpr float fmod_pi(const float x) {
        const float quotient = x / MATH_PI_MUL_2;
        const float int_part = x >= 0.0f ? floorf(quotient) : ceilf(quotient);
        float res = x - int_part * MATH_PI_MUL_2;
        if (res > MATH_PI) res -= MATH_PI_MUL_2;
        else if (res < -MATH_PI) res += MATH_PI_MUL_2;
        return res;
    }

    constexpr float sqrtf(const float x) {
        if (x < 0.0f) return compile_time_error(); // 禁止对负数开方
        if (x == 0.0f) return 0.0f;
        float curr = x;
        for (int i = 0; i < 15; ++i) {
            curr = 0.5f * (curr + x / curr);
        }
        return curr;
    }


    constexpr float sinf_core(const float x) {
        const float x2 = x * x;
        // float 精度展开到 11 次方足够了
        // x - x^3/3! + x^5/5! - x^7/7! + x^9/9! - x^11/11!
        return x * (1.0f - x2 * (0.16666666666f - x2 * (0.00833333333f - x2 * (0.00019841269f - x2 * (0.00000275573f - x2 * 0.00000002505f)))));
    }

    constexpr float sinf(float x) {
        x = fmod_pi(x); // 归约到 [-PI, PI]
        // 进一步归约到 [-PI/2, PI/2] 提高精度
        if (x > MATH_PI_2) x = MATH_PI - x;
        else if (x < -MATH_PI_2) x = -MATH_PI - x;
        return sinf_core(x);
    }

    constexpr float cosf(const float x) {
        return sinf(x + MATH_PI_2);
    }

    constexpr float tanf(const float x) {
        return sinf(x) / cosf(x);
    }

    constexpr float asinf(const float x) {
        if (x >= 1.0f) return MATH_PI_2;
        if (x <= -1.0f) return -MATH_PI_2;

        float y = x; // 初始猜测值
        // 8 次迭代足以让 float 达到机器精度 (epsilon)
        for (int i = 0; i < 8; ++i) {
            y = y - (sinf(y) - x) / cosf(y);
        }
        return y;
    }

    constexpr float acosf(const float x) {
        return MATH_PI_2 - asinf(x);
    }

    constexpr float atan2f(const float y, const float x) {
        if (x == 0.0f && y == 0.0f) return 0.0f; // 避免编译期由于未定义行为报错

        // 转化为计算距离
        const float r = sqrtf(x * x + y * y);
        const float angle = asinf(y / r);

        if (x < 0.0f) {
            if (y >= 0.0f) return MATH_PI - angle;
            return -MATH_PI - angle;
        }
        return angle;
    }


}

#endif //FLORID_MATH_HPP
