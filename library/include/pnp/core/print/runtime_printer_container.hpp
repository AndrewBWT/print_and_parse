#pragma once
#include "pnp/core/concepts.hpp"
#include "pnp/core/print/text_sink.hpp"

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

PNP_NS_BEGIN

/*!
 * @brief An object that represents an overridable printer. It holds a copy of
 * any printer for T under one type.
 *
 * It is designed to allow the user to change a printer during runtime.
 *
 * Copying a container copies the printer it holds using the printer's own copy
 * constructor.
 *
 * No move operations are declared deliberately: an rvalue is copied, so a
 * container always holds a printer. Defaulted moves would leave an empty one.
 *
 * @tparam T The type being printed.
 */
    template <typename T>
struct runtime_printer_container
{
public:
    /*!
     * @brief Default constructor. It holds the default_printer for T.
     *
     * There must be a default_printer for T for this constructor to be
     * available.
     */
    runtime_printer_container()
        requires default_printable_c<T>;
    /*!
     * @brief Constructor creating an object that holds printer_arg internally.
     *
     * The printer cannot be a runtime_printer_container itself, must be copy
     * constructible, and must adhere to the printer_c concept.
     *
     * @tparam Printer The type of the printer.
     * @param printer_arg The Printer to initialise the object with.
     */
    template <typename Printer>
        requires (not std::same_as<Printer, runtime_printer_container<T>>)
    && printer_c<Printer, T>&& std::copy_constructible<Printer>
        runtime_printer_container(Printer printer_arg);
    /*!
     * @brief Copy constructor.
     * @param other_arg The object to be copied into this object.
     */
    runtime_printer_container(const runtime_printer_container& other_arg);
    /*!
     * @brief Copy assignment operator.
     * @param other_arg The object to be copied to this object.
     * @return *this.
     */
    runtime_printer_container&
        operator=(const runtime_printer_container& other_arg);
    /*!
     * @brief Function to run the internal printer.
     * @param value_arg The object to print.
     * @param sink_arg The text_sink to print to.
     */
    void
        run_printer(
            const T& value_arg,
            text_sink& sink_arg
        ) const;
private:
    /*!
     * @brief Abstract object which holds the printer itself. It is extended
     * with a concrete object below.
     */
    struct abstract_internal_printer
    {
        virtual ~abstract_internal_printer() = default;
        /*!
         * @brief Function to run the printer.
         * @param value_arg The object to print.
         * @param sink_arg The text_sink to print to.
         */
        virtual void
            run_printer(const T& value_arg, text_sink& sink_arg) const
            = 0;
        /*!
         * @brief Function that copies the internal printer.
         * @return A copy of the printer.
         */
        virtual std::unique_ptr<abstract_internal_printer>
            clone() const = 0;
    };

    /*!
     * @brief Concrete object containing the printer itself.
     * @tparam Printer The type of the printer.
     */
    template <typename Printer>
    struct concrete_internal_printer final : abstract_internal_printer
    {
        /*!
         * @brief Main constructor.
         * @param printer_arg The object used to initialise the printer.
         */
        explicit concrete_internal_printer(Printer printer_arg);
        /*!
         * @brief Runs the printer.
         * @param value_arg The object to print.
         * @param sink_arg The text_sink to print to.
         */
        void
            run_printer(const T& value_arg, text_sink& sink_arg) const override;
        /*!
         * @brief Creates a copy of the printer.
         * @return A copied value containing _printer.
         */
        std::unique_ptr<abstract_internal_printer>
            clone() const override;
        //! The Printer object itself.
        Printer _printer;
    };

    //! Contains the printer itself.
    std::unique_ptr<abstract_internal_printer> _printer;
};

PNP_NS_END

PNP_NS_BEGIN

template <typename T>
runtime_printer_container<T>::runtime_printer_container()
    requires default_printable_c<T>
: runtime_printer_container(default_printer<T>{})
{
}

template <typename T>
template <typename Printer>
    requires (not std::same_as<Printer, runtime_printer_container<T>>)
&& printer_c<Printer, T>&& std::copy_constructible<Printer>
runtime_printer_container<T>::runtime_printer_container(
    Printer printer_arg
)
    : _printer(std::make_unique<concrete_internal_printer<Printer>>(
        std::move(printer_arg)
    ))
{
}

template <typename T>
runtime_printer_container<T>::runtime_printer_container(
    const runtime_printer_container& other_arg
)
    : _printer(other_arg._printer->clone())
{
}

template <typename T>
runtime_printer_container<T>&
runtime_printer_container<T>::operator=(
    const runtime_printer_container& other_arg
    )
{
    if (this != &other_arg)
    {
        _printer = other_arg._printer->clone();
    }
    return *this;
}

template <typename T>
void
runtime_printer_container<T>::run_printer(
    const T& value_arg,
    text_sink& sink_arg
) const
{
    _printer->run_printer(value_arg, sink_arg);
}

template <typename T>
template <typename Printer>
runtime_printer_container<T>::concrete_internal_printer<Printer>::
concrete_internal_printer(
    Printer printer_arg
)
    : _printer(std::move(printer_arg))
{
}

template <typename T>
template <typename Printer>
void
runtime_printer_container<T>::concrete_internal_printer<Printer>::
run_printer(
    const T& value_arg,
    text_sink& sink_arg
) const
{
    _printer.run_printer(value_arg, sink_arg);
}

template <typename T>
template <typename Printer>
std::unique_ptr<
    typename runtime_printer_container<T>::abstract_internal_printer>
    runtime_printer_container<T>::concrete_internal_printer<Printer>::clone(
    ) const
{
    return std::make_unique<concrete_internal_printer>(_printer);
}

PNP_NS_END