#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/core_language/nullptr.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/token_or_value.hpp"
#include "pnp/support/internal/utility.hpp"

#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unicode_bridge.hpp>
#include <utility>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// ============================================================ creating
// pointers

// How to make a pointer of type P from a parsed pointee. Users specialise this
// for their own pointer types; without a specialisation, a pointer type is
// print-only.
template <typename P>
struct smart_pointer_factory;

// std::unique_ptr with the default deleter: std::make_unique.
template <typename T>
requires (not std::is_array_v<T>)
struct smart_pointer_factory<std::unique_ptr<T>>
{
    static std::unique_ptr<T>
        make(
            std::remove_cv_t<T>&& value_arg
        )
    {
        return std::make_unique<T>(std::move(value_arg));
    }
};

// std::shared_ptr: std::make_shared.
template <typename T>
requires (not std::is_array_v<T>)
struct smart_pointer_factory<std::shared_ptr<T>>
{
    static std::shared_ptr<T>
        make(
            std::remove_cv_t<T>&& value_arg
        )
    {
        return std::make_shared<T>(std::move(value_arg));
    }
};

template <typename P>
concept smart_pointer_factory_c
    = requires (std::remove_cv_t<typename P::element_type>&& value) {
          {
              smart_pointer_factory<P>::make(std::move(value))
          } -> std::same_as<P>;
      };

// The type a smart pointer's pointee is printed and parsed as.
template <typename P>
using pointee_t = std::remove_cv_t<typename P::element_type>;

// ============================================================ policies and
// options

// Null pointers print as "nullptr"; others as their pointee in parentheses:
// (5). The parentheses keep a pointer to something that can itself print
// "nullptr" (a nested smart pointer, say) distinguishable from a null pointer.
struct smart_pointer_pnp_default
{
    using null_defaults                                 = nullptr_pnp_default;
    static constexpr std::u8string_view open_delimiter  = u8"smart_pointer(";
    static constexpr std::u8string_view close_delimiter = u8")";
};

struct unique_ptr_pnp_default : smart_pointer_pnp_default
{
    static constexpr std::u8string_view open_delimiter = u8"unique_ptr(";
};

struct shared_ptr_pnp_default : smart_pointer_pnp_default
{
    static constexpr std::u8string_view open_delimiter = u8"shared_ptr(";
};

struct weak_ptr_pnp_default : smart_pointer_pnp_default
{
    static constexpr std::u8string_view open_delimiter = u8"weak_ptr(";
};

// The null part, the pointee part, and the delimiters around the pointee. The
// parts validated themselves when they were built; first_error() checks the
// delimiters, and that the opening delimiter and the null token can't be
// confused.
//
// The parser decides between a null pointer and a pointee by which one the
// input starts with: the null token or the opening delimiter. So the two must
// not be prefixes of each other.
//
// Empty delimiters are allowed, for output like 5 rather than (5), but then the
// parser can't tell a null pointer from a pointee whose own text starts with
// the null token: a std::unique_ptr<std::unique_ptr<int>> whose inner pointer
// is null would read back as null. Use them only when the pointee can never
// print that way.
template <typename Pointee_Pnp, typename Smart_Pointer_Pnp_Defaults>
struct smart_pointer_pnp_options
{
    using null_part_type
        = nullptr_pnp<typename Smart_Pointer_Pnp_Defaults::null_defaults>;
    using pointee_part_type = Pointee_Pnp;

    null_part_type null     = {};
    std::u8string  open_delimiter
        = std::u8string(Smart_Pointer_Pnp_Defaults::open_delimiter);
    pointee_part_type pointee = {};
    std::u8string     close_delimiter
        = std::u8string(Smart_Pointer_Pnp_Defaults::close_delimiter);

    [[nodiscard]]
    constexpr PNP_INTERNAL_NAMESPACE::token_or_value_text
        text() const noexcept
    {
        return {
            .token             = null.token(),
            .token_description = u8"null token",
            .open_delimiter    = open_delimiter,
            .close_delimiter   = close_delimiter,
            .value_description = u8"the pointee of a smart pointer",
        };
    }

    // The first rule these options break, or nullptr if they're valid.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;
};

// The policy is valid for this pointee: its default options pass first_error().
// Checked on the real options, so every rule applies, including the one between
// the null token and the opening delimiter.
template <typename Pointee_Pnp, typename Smart_Pointer_Pnp_Defaults>
concept valid_smart_pointer_pnp_c = valid_default_options_c<
    smart_pointer_pnp_options<Pointee_Pnp, Smart_Pointer_Pnp_Defaults>>;

PNP_INTERNAL_NS_BEGIN
// Prints a pointer that can be compared with nullptr and dereferenced: the null
// token, or the pointee between the delimiters. Shared by the smart pointer pnp
// and the weak pointer printer.
template <typename Options, typename Pointer>
constexpr void
    print_smart_pointer(
        const Options& options_arg,
        const Pointer& pointer_arg,
        text_sink&     sink_obj_arg
    );
PNP_INTERNAL_NS_END

// ============================================================ the pnps

// Prints and parses an owning smart pointer: unique_ptr, shared_ptr, or a
// user's own. Parsing needs a smart_pointer_factory for P, and creates the
// pointee only once it has parsed.
//
//   pnp::generic_smart_pointer_pnp<std::unique_ptr<int>,
//   pnp::default_printer<int>> p;   // (5)
template <typename P, typename Pointee_Pnp, typename Smart_Pointer_Pnp_Defaults = smart_pointer_pnp_default>
    requires (not std::is_array_v<typename P::element_type>) && valid_smart_pointer_pnp_c<Pointee_Pnp, Smart_Pointer_Pnp_Defaults>
&& (printer_c<Pointee_Pnp, pointee_t<P>> || parser_c<Pointee_Pnp, pointee_t<P>>)
struct generic_smart_pointer_pnp : public pnp_configuration<smart_pointer_pnp_options<Pointee_Pnp, Smart_Pointer_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<
        smart_pointer_pnp_options<Pointee_Pnp, Smart_Pointer_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using null_part_type    = typename base_t::options_type::null_part_type;
    using pointee_part_type = Pointee_Pnp;
    constexpr void
        run_printer(
            const std::same_as<P> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const
    requires printer_c<Pointee_Pnp, pointee_t<P>>;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            P&             object_answer,
            parser_run_key run_key_arg
        ) const
    requires parser_c<Pointee_Pnp, pointee_t<P>>
             && parse_constructible_c<pointee_t<P>>
             && smart_pointer_factory_c<P>;
};

// Prints a std::weak_ptr (or anything with lock()) through the pointer it locks
// to: its pointee while it's alive, and the null token once it has expired.
// Print-only.
template <
    typename W,
    typename Pointee_Pnp,
    typename Smart_Pointer_Pnp_Defaults = smart_pointer_pnp_default>
requires printer_c<Pointee_Pnp, pointee_t<W>>
         && valid_smart_pointer_pnp_c<Pointee_Pnp, Smart_Pointer_Pnp_Defaults>
struct generic_weak_pointer_printer
    : public pnp_configuration<
          smart_pointer_pnp_options<Pointee_Pnp, Smart_Pointer_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<
        smart_pointer_pnp_options<Pointee_Pnp, Smart_Pointer_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using null_part_type    = typename base_t::options_type::null_part_type;
    using pointee_part_type = Pointee_Pnp;
    constexpr void
        run_printer(
            const std::same_as<W> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const;
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

PNP_INTERNAL_NS_BEGIN
template <typename Options, typename Pointer>
constexpr void
    print_smart_pointer(
        const Options& options_arg,
        const Pointer& pointer_arg,
        text_sink&     sink_obj_arg
    )
{
    print_token_or_value(
        options_arg.text(),
        pointer_arg != nullptr,
        [&]
        {
            options_arg.pointee.run_printer(*pointer_arg, sink_obj_arg);
        },
        sink_obj_arg
    );
}

PNP_INTERNAL_NS_END

// ------------------------------------------------------------ options

template <typename Pointee_Pnp, typename Smart_Pointer_Pnp_Defaults>
constexpr const char*
    smart_pointer_pnp_options<Pointee_Pnp, Smart_Pointer_Pnp_Defaults>::
        first_error() const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    for (const std::u8string_view token :
         {std::u8string_view(open_delimiter),
          std::u8string_view(close_delimiter)})
    {
        if (not valid_utf8(token))
        {
            return "smart_pointer_pnp_options delimiters must be valid UTF-8";
        }
        if (not no_surrounding_whitespace(token))
        {
            return "smart_pointer_pnp_options delimiters must not start or end "
                   "with whitespace";
        }
    }

    // A rule across the null part and the pointer's own field: the parser
    // decides which it's reading by which one the input starts with.
    if (not token_and_delimiter_distinguishable(null.token(), open_delimiter))
    {
        return "smart_pointer_pnp_options open delimiter and null token must "
               "not be prefixes of each other";
    }
    return nullptr;
}

// ------------------------------------------------------------ the smart
// pointer pnp

template <typename P, typename Pointee_Pnp, typename Smart_Pointer_Pnp_Defaults>
    requires (not std::is_array_v<typename P::element_type>) && valid_smart_pointer_pnp_c<Pointee_Pnp, Smart_Pointer_Pnp_Defaults>
&& (printer_c<Pointee_Pnp, pointee_t<P>> || parser_c<Pointee_Pnp, pointee_t<P>>)
constexpr void
generic_smart_pointer_pnp<P, Pointee_Pnp, Smart_Pointer_Pnp_Defaults>::run_printer(
    const std::same_as<P> auto& object_arg,
    text_sink& sink_obj_arg
) const requires printer_c<Pointee_Pnp, pointee_t<P>>
{
    PNP_INTERNAL_NAMESPACE::print_smart_pointer(
        _options, object_arg, sink_obj_arg
    );
}

template <typename P, typename Pointee_Pnp, typename Smart_Pointer_Pnp_Defaults>
    requires (not std::is_array_v<typename P::element_type>) && valid_smart_pointer_pnp_c<Pointee_Pnp, Smart_Pointer_Pnp_Defaults>
&& (printer_c<Pointee_Pnp, pointee_t<P>> || parser_c<Pointee_Pnp, pointee_t<P>>)
constexpr basic_parser_return_value
generic_smart_pointer_pnp<P, Pointee_Pnp, Smart_Pointer_Pnp_Defaults>::run_parser(
    parser_input& parser_input_arg,
    P& object_answer,
    parser_run_key run_key_arg
) const
    requires parser_c<Pointee_Pnp, pointee_t<P>>&& parse_constructible_c<pointee_t<P>>&& smart_pointer_factory_c<P>
{
    // The pointee is parsed into a temporary, so nothing is allocated unless it
    // succeeds.
    PNP_INTERNAL_NAMESPACE::parse_target<pointee_t<P>> target;
    bool                                               is_null = false;
    if (auto result = PNP_INTERNAL_NAMESPACE::parse_token_or_value(
            _options.text(),
            parser_input_arg,
            is_null,
            [&](parser_input& input_arg)
            {
                return input_arg.run_parser(_options.pointee, target.value);
            }
        );
        result.failed())
    {
        return result;
    }

    if (is_null)
    {
        object_answer = nullptr;
    }
    else
    {
        object_answer = smart_pointer_factory<P>::make(std::move(target.value));
    }
    return {};
}

// ------------------------------------------------------------ the weak pointer
// printer

template <typename W, typename Pointee_Pnp, typename Smart_Pointer_Pnp_Defaults>
requires printer_c<Pointee_Pnp, pointee_t<W>>
         && valid_smart_pointer_pnp_c<Pointee_Pnp, Smart_Pointer_Pnp_Defaults>
constexpr void
    generic_weak_pointer_printer<W, Pointee_Pnp, Smart_Pointer_Pnp_Defaults>::
        run_printer(
            const std::same_as<W> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const
{
    // Locked for the duration of the print, so the pointee can't expire
    // part-way through.
    PNP_INTERNAL_NAMESPACE::print_smart_pointer(
        _options, object_arg.lock(), sink_obj_arg
    );
}

PNP_SUPPORT_NS_END
PNP_NS_END