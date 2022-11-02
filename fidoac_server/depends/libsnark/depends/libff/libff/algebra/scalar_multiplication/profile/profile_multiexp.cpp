#include "libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp"
#include "libff/algebra/curves/curve_serialization.hpp"
#include "libff/algebra/scalar_multiplication/multiexp.hpp"
#include "libff/algebra/scalar_multiplication/multiexp_stream.hpp"
#include "libff/common/profiling.hpp"
#include "libff/common/rng.hpp"

#include <cstdio>
#include <fstream>
#include <sys/stat.h>
#include <vector>
using namespace libff;

constexpr size_t NUM_ITERATIONS = 10;
constexpr size_t NUM_DIFFERENT_ELEMENTS = 32;

constexpr form_t FORM = form_montgomery;
constexpr compression_t COMP = compression_off;

template<typename GroupT> using run_result_t = std::pair<long long, GroupT>;

template<typename T> using test_instances_t = std::vector<T>;

template<typename GroupT>
test_instances_t<GroupT> generate_group_elements(size_t num_elements)
{
    test_instances_t<GroupT> result;
    result.reserve(num_elements);
    assert(result.size() == 0);

    // Generating a random group element is expensive, so for now we only
    // generate NUM_DIFFERENT_ELEMENTS, and repeat them. Note, some methods
    // require input to be in special form.

    size_t i;
    for (i = 0; i < NUM_DIFFERENT_ELEMENTS; ++i) {
        GroupT x = GroupT::random_element();
        x.to_special();
        result.push_back(x);
    }
    assert(result.size() == NUM_DIFFERENT_ELEMENTS);

    for (; i < num_elements; ++i) {
        assert(result.size() == i);
        result.push_back(result[i % NUM_DIFFERENT_ELEMENTS]);
    }

    assert(result.size() == num_elements);
    return result;
}

template<typename FieldT>
test_instances_t<FieldT> generate_scalars(size_t num_elements)
{
    // Use SHA512_rng because it is much faster than FieldT::random_element()
    test_instances_t<FieldT> result;
    result.reserve(num_elements);
    for (size_t i = 0; i < num_elements; i++) {
        result.push_back(SHA512_rng<FieldT>(i));
    }

    assert(result.size() == num_elements);
    return result;
}

// template<typename GroupT, typename FieldT>
// run_result_t<GroupT> profile_multiexp(
//     test_instances_t<GroupT> group_elements,
//     test_instances_t<FieldT> scalars)
// {
//     long long start_time = get_nsec_time();

//     std::vector<GroupT> answers;
//     for (size_t i = 0; i < group_elements.size(); i++) {
//         answers.push_back(multi_exp<GroupT, FieldT, Method>(
//             group_elements[i].cbegin(), group_elements[i].cend(),
//             scalars[i].cbegin(), scalars[i].cend(),
//             1));
//     }

//     long long time_delta = get_nsec_time() - start_time;

//     return run_result_t<GroupT>(time_delta, answers);
// }

template<form_t Form, compression_t Comp>
std::string base_elements_filename(
    const std::string &tag,
    const size_t num_elements,
    const size_t precompute_c = 0)
{
    return std::string("multiexp_base_elements_") + tag +
           ((Form == form_plain) ? "_plain_" : "_montgomery_") +
           ((Comp == compression_on) ? "compressed_" : "uncompressed_") +
           std::to_string(num_elements) +
           ((precompute_c) ? ("_" + std::to_string(precompute_c)) : "") +
           ".bin";
}

template<form_t Form, compression_t Comp, typename GroupT>
void create_base_element_file_for_config(
    const std::string &tag, const test_instances_t<GroupT> &base_elements)
{
    const std::string filename =
        base_elements_filename<Form, Comp>(tag, base_elements.size());

    std::cout << "Writing file '" << filename << "' ...";
    std::flush(std::cout);

    std::ofstream out_s(
        filename.c_str(), std::ios_base::out | std::ios_base::binary);
    for (const GroupT &el : base_elements) {
        group_write<encoding_binary, Form, Comp>(el, out_s);
    }
    out_s.close();

    std::cout << " DONE\n";
}

template<form_t Form, compression_t Comp, typename GroupT>
void create_precompute_file_for_config(
    const std::string &tag, const test_instances_t<GroupT> &base_elements)
{
    using Field = typename GroupT::scalar_field;
    const size_t c = bdlo12_signed_optimal_c(base_elements.size());
    const size_t entries_per_base_element = (Field::num_bits + c - 1) / c;

    const std::string filename =
        base_elements_filename<Form, Comp>(tag, base_elements.size(), c);

    std::cout << "Writing file '" << filename << "' ...";
    std::flush(std::cout);

    std::ofstream out_s(
        filename.c_str(), std::ios_base::out | std::ios_base::binary);
    for (GroupT el : base_elements) {
        // Write the base element itself, followed by each factor
        group_write<encoding_binary, Form, Comp>(el, out_s);
        for (size_t i = 0; i < entries_per_base_element - 1; ++i) {
            // Multiply the base element by 2^c, and write
            for (size_t j = 0; j < c; ++j) {
                el = el.dbl();
            }
            group_write<encoding_binary, Form, Comp>(el, out_s);
        }
    }
    out_s.close();

    std::cout << " DONE\n";
}

template<typename GroupT>
void create_base_element_files(
    const std::string &tag, const size_t num_elements)
{
    test_instances_t<GroupT> base_elements =
        generate_group_elements<GroupT>(num_elements);
    create_base_element_file_for_config<FORM, COMP>(tag, base_elements);
    create_precompute_file_for_config<FORM, COMP>(tag, base_elements);
}

template<typename GroupT>
test_instances_t<GroupT> read_group_elements(
    const std::string &tag, const size_t num_elements)
{
    const std::string filename =
        base_elements_filename<FORM, COMP>(tag, num_elements);

    test_instances_t<GroupT> elements;
    elements.reserve(num_elements);

    std::ifstream in_s;
    in_s.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    in_s.open(filename, std::ios_base::in | std::ios_base::binary);
    for (size_t i = 0; i < num_elements; ++i) {
        GroupT v;
        group_read<encoding_binary, FORM, COMP>(v, in_s);
        elements.push_back(v);
    }

    return elements;
}

template<
    typename GroupT,
    typename FieldT,
    multi_exp_method Method,
    multi_exp_base_form BaseForm = multi_exp_base_form_normal>
run_result_t<GroupT> profile_multiexp(
    test_instances_t<GroupT> group_elements, test_instances_t<FieldT> scalars)
{
    long long start_time = get_nsec_time();

    GroupT answer;
    for (size_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
        answer = multi_exp<GroupT, FieldT, Method, BaseForm>(
            group_elements.cbegin(),
            group_elements.cend(),
            scalars.cbegin(),
            scalars.cend(),
            1);
    }

    long long time_delta = get_nsec_time() - start_time;

    return run_result_t<GroupT>(time_delta, answer);
}

template<form_t Form, compression_t Comp, typename GroupT, typename FieldT>
run_result_t<GroupT> profile_multiexp_stream(
    const std::string &tag, const std::vector<FieldT> &scalars)
{
    const size_t num_elements = scalars.size();
    const std::string filename =
        base_elements_filename<Form, Comp>(tag, num_elements);

    struct stat s;
    if (stat(filename.c_str(), &s)) {
        throw std::ifstream::failure("no file: " + filename);
    }

    std::ifstream in_s(
        filename.c_str(), std::ios_base::in | std::ios_base::binary);
    in_s.exceptions(
        std::ios_base::eofbit | std::ios_base::badbit | std::ios_base::failbit);

    GroupT answer;

    long long start_time = get_nsec_time();

    for (size_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
        in_s.seekg(0llu);
        answer = multi_exp_stream<Form, Comp, GroupT, FieldT>(in_s, scalars);
    }

    long long time_delta = get_nsec_time() - start_time;

    return run_result_t<GroupT>(time_delta, answer);
}

template<form_t Form, compression_t Comp, typename GroupT, typename FieldT>
run_result_t<GroupT> profile_multiexp_stream_with_precompute(
    const std::string &tag, const std::vector<FieldT> &scalars)
{
    const size_t num_elements = scalars.size();
    const size_t c = bdlo12_signed_optimal_c(num_elements);
    const std::string filename =
        base_elements_filename<Form, Comp>(tag, num_elements, c);

    struct stat s;
    if (stat(filename.c_str(), &s)) {
        throw std::ifstream::failure("no file: " + filename);
    }

    std::ifstream in_s(
        filename.c_str(), std::ios_base::in | std::ios_base::binary);
    in_s.exceptions(
        std::ios_base::eofbit | std::ios_base::badbit | std::ios_base::failbit);

    GroupT answer;

    long long start_time = get_nsec_time();

    for (size_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
        in_s.seekg(0llu);
        answer = multi_exp_stream_with_precompute<Form, Comp, GroupT, FieldT>(
            in_s, scalars, c);
    }

    long long time_delta = get_nsec_time() - start_time;

    return run_result_t<GroupT>(time_delta, answer);
}

template<typename GroupT, typename FieldT>
void print_performance_csv(
    const std::string &tag,
    size_t expn_start,
    size_t expn_end_fast,
    size_t expn_end_naive,
    bool compare_answers)
{
    std::cout << "Profiling " << tag << "\n";
    printf(
        "\t%16s\t%16s\t%16s\t%16s\t%16s\t%16s\t%16s\n",
        "bos-coster",
        "djb",
        "djb_signed",
        "djb_signed_mixed",
        "from_stream",
        "from_stream_precompute",
        "naive");
    for (size_t expn = expn_start; expn <= expn_end_fast; expn++) {
        printf("%ld", expn);
        fflush(stdout);

        try {
            test_instances_t<GroupT> group_elements =
                read_group_elements<GroupT>(tag, 1 << expn);

            test_instances_t<FieldT> scalars =
                generate_scalars<FieldT>(1 << expn);

            run_result_t<GroupT> result_bos_coster =
                profile_multiexp<GroupT, FieldT, multi_exp_method_bos_coster>(
                    group_elements, scalars);
            printf("\t%16lld", result_bos_coster.first);
            fflush(stdout);

            run_result_t<GroupT> result_djb =
                profile_multiexp<GroupT, FieldT, multi_exp_method_BDLO12>(
                    group_elements, scalars);
            printf("\t%16lld", result_djb.first);
            fflush(stdout);

            if (compare_answers &&
                (result_bos_coster.second != result_djb.second)) {
                fprintf(stderr, "Answers NOT MATCHING (bos coster != djb)\n");
            }

            run_result_t<GroupT> result_djb_signed = profile_multiexp<
                GroupT,
                FieldT,
                multi_exp_method_BDLO12_signed>(group_elements, scalars);
            printf("\t%16lld", result_djb_signed.first);
            fflush(stdout);

            if (compare_answers &&
                (result_djb.second != result_djb_signed.second)) {
                fprintf(stderr, "Answers NOT MATCHING (djb != djb_signed)\n");
            }

            run_result_t<GroupT> result_djb_signed_mixed = profile_multiexp<
                GroupT,
                FieldT,
                multi_exp_method_BDLO12_signed,
                multi_exp_base_form_special>(group_elements, scalars);
            printf("\t%16lld", result_djb_signed_mixed.first);
            fflush(stdout);

            if (compare_answers &&
                (result_djb_signed.second != result_djb_signed_mixed.second)) {
                fprintf(
                    stderr,
                    "Answers NOT MATCHING (djb_signed != djb_signed_mixed)\n");
            }

            run_result_t<GroupT> result_stream =
                profile_multiexp_stream<FORM, COMP, GroupT, FieldT>(
                    tag, scalars);
            printf("\t%16lld", result_stream.first);
            fflush(stdout);

            if (compare_answers &&
                (result_djb_signed_mixed.second != result_stream.second)) {
                fprintf(
                    stderr,
                    "Answers NOT MATCHING (djb_signed_mixed != stream)\n");
            }

            run_result_t<GroupT> result_stream_precomp =
                profile_multiexp_stream_with_precompute<
                    FORM,
                    COMP,
                    GroupT,
                    FieldT>(tag, scalars);
            printf("\t%16lld", result_stream_precomp.first);
            fflush(stdout);

            if (compare_answers &&
                (result_stream.second != result_stream_precomp.second)) {
                fprintf(
                    stderr,
                    "Answers NOT MATCHING (stream != stream_precomp)\n");
            }

            if (expn <= expn_end_naive) {
                run_result_t<GroupT> result_naive =
                    profile_multiexp<GroupT, FieldT, multi_exp_method_naive>(
                        group_elements, scalars);
                printf("\t%16lld", result_naive.first);
                fflush(stdout);

                if (compare_answers &&
                    (result_bos_coster.second != result_naive.second)) {
                    fprintf(
                        stderr, "Answers NOT MATCHING (bos coster != naive)\n");
                }
            }

            printf("\n");

        } catch (const std::ifstream::failure &e) {
            std::cout << "(Generating files for 1<<" << expn << ")\n";
            create_base_element_files<GroupT>(tag, 1 << expn);
            continue;
        }
    }
}

int main(void)
{
    print_compilation_info();

    alt_bn128_pp::init_public_params();

    print_performance_csv<G1<alt_bn128_pp>, Fr<alt_bn128_pp>>(
        "alt_bn128_g1", 8, 20, 14, true);

    print_performance_csv<G2<alt_bn128_pp>, Fr<alt_bn128_pp>>(
        "alt_bn128_g2", 8, 20, 14, true);

    return 0;
}
