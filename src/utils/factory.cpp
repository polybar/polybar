#include "utils/factory.hpp"

POLYBAR_NS

factory_util::detail::null_deleter factory_util::null_deleter{};
factory_util::detail::fd_deleter factory_util::fd_deleter{};

POLYBAR_NS_END
