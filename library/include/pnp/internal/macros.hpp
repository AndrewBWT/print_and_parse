#pragma once
// Namespace for pnp.
#ifndef PNP_NAMESPACE
    #define PNP_NAMESPACE pnp
#endif
// Internal namespace for pnp.
#ifndef PNP_INTERNAL_NAMESPACE
    #define PNP_INTERNAL_NAMESPACE internal
#endif
// Support namespace.
#ifndef PNP_SUPPORT_NAMESPACE
    #define PNP_SUPPORT_NAMESPACE support
#endif
// Testing namespace.
#ifndef PNP_TESTING_NAMESPACE
#define PNP_TESTING_NAMESPACE testing
#endif

// Macro to begin pnp namespace.
#define PNP_NS_BEGIN        \
    namespace PNP_NAMESPACE \
    {
// Macro to end pnp namespace.
#define PNP_NS_END }
// Macro to begin internal pnp namespace.
#define PNP_INTERNAL_NS_BEGIN        \
    namespace PNP_INTERNAL_NAMESPACE \
    {
// Macro to end internal pnp namespace.
#define PNP_INTERNAL_NS_END }
// Macro to begin support pnp namespace.
#define PNP_SUPPORT_NS_BEGIN        \
    namespace PNP_SUPPORT_NAMESPACE \
    {
// Macro to end pnp support namespace.
#define PNP_SUPPORT_NS_END }
// Macro to begin testing pnp namespace.
#define PNP_TESTING_NS_BEGIN        \
    namespace PNP_TESTING_NAMESPACE \
    {
// Macro to end pnp testing namespace.
#define PNP_TESTING_NS_END }