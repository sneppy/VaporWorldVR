#pragma once

#include <utility>


#define FORWARD(x) ::std::forward<decltype((x))>((x))
