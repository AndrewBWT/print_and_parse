#include "pnp/support/internal/std/string_view.hpp"

PNP_NS_BEGIN
template <typename CharT, typename Traits>
    requires unicode_bridge::is_char_type_c<CharT>
struct default_printer<std::basic_string_view<CharT, Traits>>
    : public PNP_SUPPORT_NAMESPACE::generic_string_view_printer<std::basic_string_view<CharT, Traits>,
    PNP_SUPPORT_NAMESPACE::string_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_string_view_printer<std::basic_string_view<CharT, Traits>,
        PNP_SUPPORT_NAMESPACE::string_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};


PNP_NS_END