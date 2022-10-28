/** @file
 *****************************************************************************
 * @author     This file is part of libsnark, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_CURVE_SCALAR_MULTIPLICATION_HPP_
#define LIBSNARK_GADGETLIB1_GADGETS_CURVE_SCALAR_MULTIPLICATION_HPP_

#include "libsnark/gadgetlib1/gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/basic_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/pairing_params.hpp"

#include <libff/algebra/curves/public_params.hpp>
#include <memory>

namespace libsnark
{

/// Wrapper for group variable, adding a flag that indicates whether the
/// variable is the identity.
///
/// For use in 2 possible configurations:
///
///   1) Construct with variable_or_identity(pb, annotation_prefix), which
///      internally allocates variables for the group element and flag.
///      Values can later be set via generate_r1cs_witness() calls.
///
///   2) Construct using a group element value, which defines values as linear
///      combinations (no variable allocation). generate_r1cs_witness() calls
///      should NOT be made to such objects.
template<typename ppT, typename groupT, typename groupVariableT>
class variable_or_identity : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;

    groupVariableT value;
    pb_linear_combination<FieldT> is_identity;

    // Internally allocates the variable and flag, to be set later via the
    // generate_r1cs_witness calls.
    variable_or_identity(
        protoboard<FieldT> &pb, const std::string &annotation_prefix);

    // Initialize the variable and flag with constants. Do not try to call
    // generate_r1cs_witness().
    variable_or_identity(
        protoboard<FieldT> &pb,
        const groupT &P,
        const std::string &annotation_prefix);

    /// Constrains the `is_identity` flag to be boolean. Do not call this if
    /// the setter can guarantee boolean-ness in some other way.
    void generate_r1cs_constraints();

    /// If elt == 0, this sets `value` to 1 and `is_identity` to true,
    /// otherwise it sets `value` to `elt` and `is_identity` to false.
    void generate_r1cs_witness(const groupT &elt);

    /// For the case where `value` has been set by other gadgets, and we simply
    /// wish to set whether this variable should be considered the identity.
    void generate_r1cs_witness(bool is_identity);

    groupT get_element() const;

protected:
    // Only allocated if necessary, in which case it should be set using
    // generate_r1cs_witness().
    pb_variable<FieldT> is_identity_var;
};

/// Selector gadget for variable_or_identity
template<
    typename ppT,
    typename groupT,
    typename variableT,
    typename variableSelectorT>
class variable_or_identity_selector : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using variableOrIdentity = variable_or_identity<ppT, groupT, variableT>;

    // result.value = variableSelectorT(
    //     selector, zero_case.value, one_case.value)
    variableSelectorT value_selector;

    // result.is_identity = (1 - selector) * zero_case + selector * one_case
    //                    = selector * (one_case - zero_case) + zero_case
    // <=> result.is_identity - zero_case = selector * (one_case - zero_case)
    pb_linear_combination<FieldT> zero_case_is_identity;
    pb_linear_combination<FieldT> one_case_is_identity;
    pb_linear_combination<FieldT> result_is_identity;

    variable_or_identity_selector(
        protoboard<FieldT> &pb,
        const pb_linear_combination<FieldT> &selector,
        const variableOrIdentity &zero_case,
        const variableOrIdentity &one_case,
        const variableOrIdentity &result,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/// Selector gadget for a variable_or_identity, and a variable
template<
    typename ppT,
    typename groupT,
    typename variableT,
    typename variableSelectorT>
class variable_and_variable_or_identity_selector : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using variableOrIdentity = variable_or_identity<ppT, groupT, variableT>;

    // result.value = select(selector, zero_case.value, one_case)
    variableSelectorT value_selector;

    // result.is_identity = (1 - selector) * zero_case
    pb_linear_combination<FieldT> zero_case_is_identity;
    pb_linear_combination<FieldT> one_case_is_identity;
    variableOrIdentity result;

    variable_and_variable_or_identity_selector(
        protoboard<FieldT> &pb,
        const pb_linear_combination<FieldT> &selector,
        const variableOrIdentity &zero_case,
        const variableT &one_case,
        const variableOrIdentity &result,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/// Wrap an add gadget, extending it to support adding variable_or_identity
/// elements. At most one element may be the identity, otherwise witness
/// generation fails due to the implementation of the underlying add gadgets.
template<
    typename ppT,
    typename groupT,
    typename variableT,
    typename variableSelectorT,
    typename addGadgetT>
class add_variable_or_identity : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using variableOrIdentity = variable_or_identity<ppT, groupT, variableT>;

    // Recall the form of the variable selector gadget parameters:
    //   variableSelectorT(<flag>, <zero_case>, <one_case>)
    //
    // The `is_identity` flags of A, B and result are known to be boolean by
    // the constraints in variable_or_identity.
    //
    // The output from this gadget should therefore be:
    //
    //   result.is_identity = A.is_identity * B.is_identity
    //
    //   result.value = variableSelectorT(  // selector_A
    //       A.is_identity,
    //       variableSelectorT(             // selector_A_not_identity
    //           B.is_identity,
    //           add_result,                   // A != 0 && B != 0
    //           A.value),                     // A != 0 && B == 0
    //       variableSelectorT(             // selector_A_is_identity
    //           B.is_identity,
    //           B.value,                      // A == 0 && B != 0
    //           groupT::one())                // A == 0 && B == 0
    //
    // Note, this can be reduced to:
    //
    //   result.is_identity = A.is_identity * B.is_identity
    //
    //   result.value = variableSelectorT(  // selector_A
    //       A.is_identity,
    //       variableSelectorT(             // selector_A_not_identity
    //           B.is_identity,
    //           add_result,                   // A != 0 && B != 0
    //           A.value),                     // A != 0 && B == 0
    //       B.value)

    variableT add_result;
    addGadgetT add;

    // A_not_identity_result = variableSelectorT(
    //     B.is_identity, add_result, A.value)
    variableT A_not_identity_result;
    variableSelectorT selector_A_not_identity;

    // result = variableSelectorT(
    //     A.is_identity, A_not_identity_result, B.value)
    variableSelectorT selector_A;

    variableOrIdentity result;

    add_variable_or_identity(
        protoboard<FieldT> &pb,
        const variableOrIdentity &A,
        const variableOrIdentity &B,
        const variableOrIdentity &result,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();

    void generate_r1cs_witness();
};

/// Wrap an add gadget for group variables, extending it to support adding a
/// variable_or_identity to a variable.
template<
    typename ppT,
    typename groupT,
    typename variableT,
    typename variableSelectorT,
    typename addGadgetT>
class add_variable_and_variable_or_identity : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using variableOrIdentity = variable_or_identity<ppT, groupT, variableT>;

    // Recall the form of the variable selector gadget parameters:
    //   variableSelectorT(<flag>, <zero_case>, <one_case>)
    //
    // The `is_identity` flag of A is known to be boolean by the constraints in
    // variable_or_identity.
    //
    // The output from this gadget should therefore be:
    //
    //   add_result = add(A.value, B)
    //
    //   result.value = variableSelectorT(
    //       A.is_identity,
    //       B,                       // A == 0 case
    //       add_result)              // A != 0 case

    variableT result;
    variableT add_result;
    addGadgetT add;
    variableSelectorT selector_A;

    add_variable_and_variable_or_identity(
        protoboard<FieldT> &pb,
        const variableOrIdentity &A,
        const variableT &B,
        const variableT &result,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();

    void generate_r1cs_witness();
};

/// Wrap a dbl gadget, extending it to support doubling variable_or_identity
/// elements.
template<
    typename ppT,
    typename groupT,
    typename groupVariableT,
    typename dblGadgetT>
class dbl_variable_or_identity : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using variableOrIdentity =
        variable_or_identity<ppT, groupT, groupVariableT>;

    // The goal is:
    //
    //     result = select(A.is_identity, 0, double(A))
    //
    // which is achieved via:
    //
    //     result.value = double(A)
    //     result.is_identity = A.is_identity
    //
    // Note: result.is_identity does not need to be allocated, but the current
    // variable_or_identity interface always allocates. Ideally it would simply
    // be set to pb_linear_combination(A.is_identity), which costs nothing

    pb_linear_combination<FieldT> A_is_identity;
    variableOrIdentity result;
    dblGadgetT double_gadget;

    dbl_variable_or_identity(
        protoboard<FieldT> &pb,
        const variableOrIdentity &A,
        const variableOrIdentity &result,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/// Generic gadget to perform scalar multiplication of group variables. Used by
/// the individual group element implementations.
template<
    typename groupT,
    typename groupVariableT,
    typename add_gadget,
    typename dbl_gadget,
    typename scalarT>
class point_mul_by_const_scalar_gadget
    : public gadget<typename groupT::base_field>
{
public:
    using FieldT = typename groupT::base_field;

    const scalarT _scalar;
    const groupVariableT _result;
    std::vector<std::shared_ptr<add_gadget>> _add_gadgets;
    std::vector<std::shared_ptr<dbl_gadget>> _dbl_gadgets;

    point_mul_by_const_scalar_gadget(
        protoboard<FieldT> &pb,
        const scalarT &scalar,
        const groupVariableT &P,
        const groupVariableT &result,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();
    void generate_r1cs_witness();
    const groupVariableT &result() const;
};

/// Generic gadget to perform scalar multiplication of group variables by
/// variables. Used by the individual group element implementations.
template<
    typename ppT,
    typename groupT,
    typename groupVarT,
    typename selectorGadgetT,
    typename addGadgetT,
    typename dblGadgetT>
class point_mul_by_scalar_gadget : public gadget<typename groupT::base_field>
{
public:
    using Field = libff::Fr<ppT>;
    using nFr = libff::Fr<other_curve<ppT>>;
    using groupVarOrIdentity = variable_or_identity<ppT, groupT, groupVarT>;
    using selectVarOrVarIdentityGadget =
        variable_and_variable_or_identity_selector<
            ppT,
            groupT,
            groupVarT,
            selectorGadgetT>;
    using addVarAndVarOrIdentityGadget = add_variable_and_variable_or_identity<
        ppT,
        groupT,
        groupVarT,
        selectorGadgetT,
        addGadgetT>;
    using dblVarOrIdentityGadget =
        dbl_variable_or_identity<ppT, groupT, groupVarT, dblGadgetT>;

    packing_gadget<Field> scalar_unpacked;
    std::vector<dblVarOrIdentityGadget> dbl_gadgets;
    std::vector<addVarAndVarOrIdentityGadget> add_gadgets;
    std::vector<selectVarOrVarIdentityGadget> selector_gadgets;

    // Use to avoid adding P + -P in the final loop.
    //
    // For scalar = -1, the final loop can result in a call to addGadgetT(P,
    // -P), which is not supported (results in a division by zero since the X
    // coordinates of P and -P are the same). This is because: after the final
    // double, scalar * P = -P has been computed, and the final (lowest order)
    // bit of scalar is 0 (i.e. no further addition is required). However,
    // since the constraints must hold for all scalar values, the final
    // addition is always performed (but filtered out).
    //
    // To handle this edge case, we "select" either the correct intermediate
    // values (if the 0-th bit is 1), or some hard-coded values which are known
    // to work with addGadgetT (if 0-th bit is 0). This situation should not
    // arise in other cases, since all intermediate multiple of P are
    // necessarily multiples strictly smaller than the (prime) order of the
    // group - 1.

    std::shared_ptr<selectVarOrVarIdentityGadget> final_add_input_left;
    std::shared_ptr<selectorGadgetT> final_add_input_right;

    point_mul_by_scalar_gadget(
        protoboard<Field> &pb,
        const pb_linear_combination<Field> &scalar,
        const groupVarT &P,
        const groupVarOrIdentity &result,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();
    void generate_r1cs_witness();
    const groupVarOrIdentity &result() const;

protected:
    static pb_variable_array<Field> create_variable_array_for_bits(
        protoboard<Field> &pb, const std::string &annotation_prefix);
};

} // namespace libsnark

#include "libsnark/gadgetlib1/gadgets/curves/scalar_multiplication.tcc"

#endif // LIBSNARK_GADGETLIB1_GADGETS_CURVE_SCALAR_MULTIPLICATION_HPP_
