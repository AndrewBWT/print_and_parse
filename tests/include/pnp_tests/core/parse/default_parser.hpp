#include "pnp/support.hpp"

struct not_parseable {};
static_assert(not pnp::default_parseable_c<not_parseable>);