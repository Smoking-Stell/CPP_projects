#include "big_integer.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <stdexcept>

// module + sign

static const uint64_t base = (UINT32_MAX + 1ul);
static const uint32_t DI = 10;

big_integer::big_integer() : ranks({}), sign(false) {
}

big_integer::big_integer(big_integer const& other) = default;

big_integer::big_integer(unsigned long long a) : sign(false), ranks({}){
    if (a != 0) {
        ranks.push_back(static_cast<uint32_t>(a));
    }
    a >>= 32;
    push_el(static_cast<uint32_t>(a));
}

big_integer::big_integer(int a) : big_integer(static_cast<signed long long>(a)) {
}

big_integer::big_integer(signed long a) : big_integer(static_cast<signed long long>(a)){
}

big_integer::big_integer(unsigned long a) : big_integer(static_cast<unsigned long long>(a)){
}

// we don't know pb quantity
big_integer::big_integer(signed long long a): sign(a < 0), ranks({}){
    unsigned long long x;
    if (a < 0) {
        a++;
        x = static_cast<unsigned long long>(-a) + 1ULL;
    } else {
        x = static_cast<unsigned long long>(a);
    }
    if (x != 0) {
        ranks.push_back(static_cast<uint32_t>(x));
    }
    x >>= 32;
    push_el(static_cast<uint32_t>(x));
}

big_integer::big_integer(uint32_t a) : big_integer(static_cast<unsigned long long>(a)){
}

big_integer::big_integer(std::string const& str) : big_integer(){
    size_t len = str.length();
    size_t start = ((str[0] == '-') || (str[0] == '+') ? 1 : 0);
    if (start >= len) {
        throw std::invalid_argument("Wrong string");
    }
    for (size_t i = start; i < len; i += 9) {
        uint32_t degr = 1;
        uint32_t pul_new = 0;
        for (size_t j = i; j < std::min(len, i + 9); j++) {
            if ((str[j] < '0') || (str[j] > '9')) {
                throw std::invalid_argument("Wrong string");
            }
            pul_new *= 10;
            pul_new += static_cast<uint32_t>(str[j] - '0');
            degr *= 10;
        }
        *this *= degr;
        *this += pul_new;
    }

    sign = static_cast<bool>(start);
    if (is_zero()) {
        sign = false;
    }
}

big_integer::~big_integer() = default;

big_integer& big_integer::operator=(big_integer const& other) = default;

big_integer& big_integer::operator+=(big_integer const& rhs) {
    if (sign != rhs.sign) {
        if (sign) {
            sign = false;
            *this -= rhs;
            sign = !sign;
        } else {
            *this -= -rhs;
        }
        return *this;
    }
    size_t n = ranks.size();
    size_t m = rhs.ranks.size();
    size_t max_len = std::max(n, m);
    ranks.resize(max_len);

    uint64_t trans = 0;
    for (size_t i = 0; i < max_len; i++) {
        uint64_t t = 0;
        if (i < m) {
            t += rhs.ranks[i];
        }
        if (i < n) {
            t += ranks[i];
        }
        t += trans;
        trans = t >> 32ul;
        ranks[i] = static_cast<uint32_t>(t);
    }
    push_el(static_cast<uint32_t>(trans));
    pull_zero();
    return *this;
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
    if (sign != rhs.sign) {
        sign = !sign;
        *this += rhs;
        sign = !sign;
        return *this;
    }
    big_integer b(rhs);
    bool sign_flag = sign;
    if ((sign && b.sign) ^ (*this < b)) {
        sign_flag = !sign_flag;
        sign = false;
        b.sign = false;
        swap(b);
    }
    size_t n = ranks.size();
    size_t m = b.ranks.size();

    uint64_t trans = 0;
    for (size_t i = 0; i < n; i++) {
        uint64_t sub = (m > i ? static_cast<uint64_t>(b.ranks[i]) + trans : trans);
        if (sub > static_cast<uint64_t>(ranks[i])) {
            sub = base - sub + ranks[i];
            trans = 1;
        } else {
            sub = ranks[i] - sub;
            trans = 0;
        }
        ranks[i] = (static_cast<uint32_t>(sub));
    }
    pull_zero();
    sign = sign_flag;
    return *this;
}

big_integer& big_integer::operator*=(big_integer const& rhs) {
    big_integer ans;
    if (is_zero() || rhs.is_zero()) {
        *this = ans;
        return *this;
    }
    size_t n = ranks.size();
    size_t m = rhs.ranks.size();
    ans.ranks.resize(n + m, 0);
    uint64_t trans = 0;
    for (size_t i = 0; i < n; i++) {
        trans = 0;
        for (size_t j = 0; j < m; j++) {
            uint64_t x = static_cast<uint64_t>(ranks[i]) * rhs.ranks[j]
                       + static_cast<uint64_t>(ans.ranks[i + j]) + trans;
            trans = (x >> 32);
            ans.ranks[i + j] = static_cast<uint32_t>(x & UINT32_MAX);
        }
        ans.ranks[i + m] += static_cast<uint32_t>(trans);
    }
    ans.sign = (sign != rhs.sign);
    ans.pull_zero();
    *this = ans;
    return *this;
}

big_integer& big_integer::operator/=(big_integer const& rhs) {
    if (rhs.is_zero()) {
        throw std::overflow_error("Zero division");
    }
    bool sign_flag = (rhs.sign ^ sign);
    big_integer b(rhs);
    big_integer a(*this);
    b.sign = false;
    a.sign = false;
    size_t n = a.ranks.size();
    size_t m = b.ranks.size();
    big_integer ans;

    if (a < b) {
        return *this = 0;
    }
    if (m == 1) {
        divide_by_small(b.ranks[0]);
        sign = sign_flag;
        pull_zero();
        return *this;
    }
    uint32_t norm = (1ULL << 32) / (b.ranks.back() + 1);
    a *= norm;
    b *= norm;
    a.ranks.push_back(0);
    n = a.ranks.size();
    m = b.ranks.size();
    ans.ranks.resize(n - m);

    for (size_t i = n - m; i > 0; i--) {
        size_t ind = m + i - 1;
        uint64_t ans_i = static_cast<uint64_t>(a.ranks[ind]) * base + static_cast<uint64_t>(a.ranks[ind - 1]);
        ans_i /= static_cast<uint64_t>(b.ranks[m - 1]);
        uint32_t t = std::min(static_cast<uint32_t>(ans_i), UINT32_MAX);

        big_integer del = b * t;
        while (t > 0 && compare(a, del, m, i - 1)) {
            t--;
            del -= b;
        }

        uint32_t trans = 0;
        ans.ranks[i - 1] = t;
        for (size_t j = 0; j < m; j++) {
            uint64_t sub = ((del.ranks.size() > j) ? static_cast<uint64_t>(del.ranks[j]) + trans : trans);
            if (sub > static_cast<uint64_t>(a.ranks[j + i - 1])) {
                sub = base - sub + a.ranks[j + i - 1];
                trans = 1;
            } else {
                sub = a.ranks[j + i - 1] - sub;
                trans = 0;
            }
            a.ranks[j + i - 1] = (static_cast<uint32_t>(sub));
        }
    }
    ans.pull_zero();
    ans.sign = sign_flag;
    return *this = ans;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
    return *this -= (*this / rhs) * rhs;
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
    general_bit_operation(rhs,
                            [](uint32_t x, uint32_t y) {return x & y;});
    return *this;
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
    general_bit_operation(rhs,
                          [](uint32_t x, uint32_t y) {return x | y;});
    return *this;
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
    general_bit_operation(rhs,
                          [](uint32_t x, uint32_t y) {return x ^ y;});
    return *this;
}

big_integer& big_integer::operator<<=(int rhs) {
    size_t b32 = rhs / 32;
    rhs = rhs % 32;
    std::reverse(ranks.begin(), ranks.end());
    while ((!ranks.empty()) && (b32 > 0)) {
        ranks.push_back(0);
        b32--;
    }
    std::reverse(ranks.begin(), ranks.end());
    uint32_t trans = 0;
    for (size_t i = 0; i < ranks.size(); i++) {
        uint64_t x = static_cast<uint64_t>(ranks[i]);
        x <<= rhs;
        ranks[i] = x + trans;
        trans = x >> 32u;
    }
    push_el(trans);
    return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
    size_t b32 = rhs / 32;
    rhs = rhs % 32;
    std::reverse(ranks.begin(), ranks.end());
    while ((!ranks.empty()) && (b32 > 0)) {
        ranks.pop_back();
        b32--;
    }
    std::reverse(ranks.begin(), ranks.end());

    uint32_t trans = 0;
    for (size_t i = ranks.size(); i > 0; i--) {
        uint64_t x = static_cast<uint64_t>(ranks[i - 1]);
        ranks[i - 1] = (x >> rhs) + trans;
        trans = x << (32u - rhs);
    }
    if (sign) {
        *this -= 1;
    }
    pull_zero();
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    if (is_zero()) {
        return *this;
    }
    big_integer ans(*this);
    ans.sign = !ans.sign;
    return ans;
}

big_integer big_integer::operator~() const {
    big_integer ans(*this);
    ans.sign = !ans.sign;
    ans -= 1;
    return ans;
}

big_integer& big_integer::operator++() {
    return (*this += 1);
}

big_integer big_integer::operator++(int) {
    big_integer old(*this);
    *this += 1;
    return old;
}

big_integer& big_integer::operator--() {
    return (*this -= 1);
}

big_integer big_integer::operator--(int) {
    big_integer old(*this);
    *this -= 1;
    return old;
}

big_integer operator+(big_integer a, big_integer const& b) {
    return a += b;
}

big_integer operator-(big_integer a, big_integer const& b) {
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b) {
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b) {
   return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b) {
    return a %= b;
}

void big_integer::general_bit_operation(big_integer const& b,
                                  const std::function<uint32_t (uint32_t, uint32_t)>& bit_oper) {
    size_t max_len = std::max(ranks.size(), b.ranks.size());
    bool sign_flag = static_cast<bool>(
        bit_oper(static_cast<uint32_t>(sign), static_cast<uint32_t>(b.sign)));
    big_integer bit_b(b);
    bitcast(max_len);
    bit_b.bitcast(max_len);
    for (size_t i = 0; i < max_len; i++) {
        ranks[i] = bit_oper(ranks[i], bit_b.ranks[i]);
    }
    // if we need to return negative value, we use two's complement
    if (sign_flag) {
        for (size_t i = 0; i < ranks.size(); i++) {
            ranks[i] = ~ranks[i];
        }
        *this += 1;
        sign = true;
    }
    pull_zero();
}

big_integer operator&(big_integer a, big_integer const& b) {
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b) {
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b) {
    return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
    return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
    return a >>= b;
}

bool operator==(big_integer const& a, big_integer const& b){
    if (a.is_zero() && b.is_zero()) {
        return true;
    }
    size_t n = a.ranks.size();
    size_t m = b.ranks.size();
    if (n == m && (a.sign == b.sign)) {
        for (size_t i = 0; i < n; i++) {
            if (a.ranks[i] != b.ranks[i])
                return false;
        }
        return true;
    }
    return false;
}

bool operator!=(big_integer const& a, big_integer const& b) {
    return !(a == b);
}

bool operator<(big_integer const& a, big_integer const& b) {
    if (a.is_zero() && b.is_zero()) {
        return false;
    }
    size_t n = a.ranks.size();
    size_t m = b.ranks.size();
    if (a.sign != b.sign) {
        return a.sign;
    }
    bool ans = true;
    if (a.sign && b.sign) {
        ans = !ans;
    }
    if (n == m) {
        for (size_t i = n; i > 0; i--) {
            if (a.ranks[i - 1] < b.ranks[i - 1])
                return ans;
            if (a.ranks[i - 1] > b.ranks[i - 1])
                return !ans;
        }
        return !ans;
    }
    return (n < m ? ans : !ans);
}

bool operator>(big_integer const& a, big_integer const& b) {
    return b < a;
}

bool operator<=(big_integer const& a, big_integer const& b) {
    return !(a > b);
}

bool operator>=(big_integer const& a, big_integer const& b) {
    return !(a < b);
}

std::string to_string(big_integer const& a) {
    std::string ans;
    if (a.is_zero()) {
        return "0";
    }
    // need because, we change copy_a, but a is constant
    big_integer copy_a(a);
    copy_a.pull_zero();

    while (!copy_a.ranks.empty()){
        uint32_t x = copy_a.divide_by_small(DI);
        ans += std::to_string(x);
    }
    std::reverse(ans.begin(), ans.end());
    if (a.sign && !(a.is_zero())) {
        ans = "-" + ans;
    }
    return ans;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a) {
    return s << to_string(a);
}

void big_integer::swap(big_integer &other) {
    std::swap(ranks, other.ranks);
    std::swap(sign, other.sign);
}

bool big_integer::is_zero() const {
    return ranks.empty();
}

void big_integer::pull_zero() {
    while (!ranks.empty() && ranks.back() == 0) {
        ranks.pop_back();
    }
    if (ranks.empty()) {
        sign = false;
    }
}

//stolbik
uint32_t big_integer::divide_by_small(uint32_t x) {
    size_t n = ranks.size();
    uint32_t trans = 0;
    for (size_t i = n; i > 0; i--) {
        uint64_t t = static_cast<uint64_t>(ranks[i - 1]) + base * trans;
        ranks[i - 1] = static_cast<uint32_t>(t / x);
        trans = static_cast<uint32_t>(t % x);
    }
    pull_zero();
    return trans;
}

size_t big_integer::bitcast(size_t need_size) {
    ranks.resize(need_size);
    if (sign && !is_zero()) {
        sign = false;
        for (size_t i = 0; i < ranks.size(); i++) {
            ranks[i] = ~ranks[i];
        }
        *this += 1;
    }
    return ranks.size();
}

void big_integer::push_el(uint32_t x) {
    if (x != 0) {
        ranks.push_back(x);
    }
}

bool big_integer::compare(big_integer &a, big_integer &b, size_t lenb, size_t up) {
    for (size_t j = lenb; j > 0; j--) {
        uint32_t t = (j < b.ranks.size() ? b.ranks[j] : 0);
        if (a.ranks[j + up] < t) {
            return true;
        }
        if (a.ranks[j + up] > t) {
            return false;
        }
    }
    return false;
}