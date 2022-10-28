/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include <libff/algebra/curves/alt_bn128/alt_bn128_g1.hpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_g2.hpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_init.hpp>

namespace libff
{

bigint<alt_bn128_r_limbs> alt_bn128_modulus_r;
bigint<alt_bn128_q_limbs> alt_bn128_modulus_q;

alt_bn128_Fq alt_bn128_coeff_b;
alt_bn128_Fq2 alt_bn128_twist;
alt_bn128_Fq2 alt_bn128_twist_coeff_b;
alt_bn128_Fq alt_bn128_twist_mul_by_b_c0;
alt_bn128_Fq alt_bn128_twist_mul_by_b_c1;
alt_bn128_Fq2 alt_bn128_twist_mul_by_q_X;
alt_bn128_Fq2 alt_bn128_twist_mul_by_q_Y;

bigint<alt_bn128_q_limbs> alt_bn128_ate_loop_count;
bool alt_bn128_ate_is_loop_count_neg;
bigint<12 * alt_bn128_q_limbs> alt_bn128_final_exponent;
bigint<alt_bn128_q_limbs> alt_bn128_final_exponent_z;
bool alt_bn128_final_exponent_is_z_neg;

void init_alt_bn128_params()
{
    typedef bigint<alt_bn128_r_limbs> bigint_r;
    typedef bigint<alt_bn128_q_limbs> bigint_q;

    assert(
        sizeof(mp_limb_t) == 8 ||
        sizeof(mp_limb_t) == 4); // Montgomery assumes this

    /* parameters for scalar field Fr */

    alt_bn128_modulus_r = bigint_r("2188824287183927522224640574525727508854836"
                                   "4400416034343698204186575808495617");
    assert(alt_bn128_Fr::modulus_is_valid());
    if (sizeof(mp_limb_t) == 8) {
        alt_bn128_Fr::Rsquared =
            bigint_r("944936681149208446651664254269745548490766851729442924617"
                     "792859073125903783");
        alt_bn128_Fr::Rcubed =
            bigint_r("586654854594384522748989487204024472040386810557878410528"
                     "1690076696998248512");
        alt_bn128_Fr::inv = 0xc2e1f593efffffff;
    }
    if (sizeof(mp_limb_t) == 4) {
        alt_bn128_Fr::Rsquared =
            bigint_r("944936681149208446651664254269745548490766851729442924617"
                     "792859073125903783");
        alt_bn128_Fr::Rcubed =
            bigint_r("586654854594384522748989487204024472040386810557878410528"
                     "1690076696998248512");
        alt_bn128_Fr::inv = 0xefffffff;
    }
    alt_bn128_Fr::num_bits = 254;
    alt_bn128_Fr::euler = bigint_r("1094412143591963761112320287262863754427418"
                                   "2200208017171849102093287904247808");
    alt_bn128_Fr::s = 28;
    alt_bn128_Fr::t = bigint_r(
        "81540058820840996586704275553141814055101440848469862132140264610111");
    alt_bn128_Fr::t_minus_1_over_2 = bigint_r(
        "40770029410420498293352137776570907027550720424234931066070132305055");
    alt_bn128_Fr::multiplicative_generator = alt_bn128_Fr("5");
    alt_bn128_Fr::root_of_unity =
        alt_bn128_Fr("191032190679217139442913928276920700361456519573292863153"
                     "05642004821462161904");
    alt_bn128_Fr::nqr = alt_bn128_Fr("5");
    alt_bn128_Fr::nqr_to_t =
        alt_bn128_Fr("191032190679217139442913928276920700361456519573292863153"
                     "05642004821462161904");
    alt_bn128_Fr::static_init();

    /* parameters for base field Fq */

    alt_bn128_modulus_q = bigint_q("2188824287183927522224640574525727508869631"
                                   "1157297823662689037894645226208583");
    assert(alt_bn128_Fq::modulus_is_valid());
    if (sizeof(mp_limb_t) == 8) {
        alt_bn128_Fq::Rsquared =
            bigint_q("309661650298370392384356793683737445173554096841907652877"
                     "1170197431451843209");
        alt_bn128_Fq::Rcubed =
            bigint_q("149217865411596481859481527385630809590936198385102451777"
                     "10943249661917737183");
        alt_bn128_Fq::inv = 0x87d20782e4866389;
    }
    if (sizeof(mp_limb_t) == 4) {
        alt_bn128_Fq::Rsquared =
            bigint_q("309661650298370392384356793683737445173554096841907652877"
                     "1170197431451843209");
        alt_bn128_Fq::Rcubed =
            bigint_q("149217865411596481859481527385630809590936198385102451777"
                     "10943249661917737183");
        alt_bn128_Fq::inv = 0xe4866389;
    }
    alt_bn128_Fq::num_bits = 254;
    alt_bn128_Fq::euler = bigint_q("1094412143591963761112320287262863754434815"
                                   "5578648911831344518947322613104291");
    alt_bn128_Fq::s = 1;
    alt_bn128_Fq::t = bigint_q("10944121435919637611123202872628637544348155578"
                               "648911831344518947322613104291");
    alt_bn128_Fq::t_minus_1_over_2 =
        bigint_q("5472060717959818805561601436314318772174077789324455915672259"
                 "473661306552145");
    alt_bn128_Fq::multiplicative_generator = alt_bn128_Fq("3");
    alt_bn128_Fq::root_of_unity =
        alt_bn128_Fq("218882428718392752222464057452572750886963111572978236626"
                     "89037894645226208582");
    alt_bn128_Fq::nqr = alt_bn128_Fq("3");
    alt_bn128_Fq::nqr_to_t =
        alt_bn128_Fq("218882428718392752222464057452572750886963111572978236626"
                     "89037894645226208582");
    alt_bn128_Fq::static_init();

    /* parameters for twist field Fq2 */
    alt_bn128_Fq2::euler = bigint<2 * alt_bn128_q_limbs>(
        "2395475880083114212209940226083393703996261582655504112182239011270350"
        "4684318911872392052590971893598559411615740655013091812781706979347432"
        "3196511433944");
    alt_bn128_Fq2::s = 4;
    alt_bn128_Fq2::t = bigint<2 * alt_bn128_q_limbs>(
        "2994344850103892765262425282604242129995326978319380140227798764087938"
        "0855398639840490065738714866998199264519675818766364765977133724184290"
        "399563929243");
    alt_bn128_Fq2::t_minus_1_over_2 = bigint<2 * alt_bn128_q_limbs>(
        "1497172425051946382631212641302121064997663489159690070113899382043969"
        "0427699319920245032869357433499099632259837909383182382988566862092145"
        "199781964621");
    alt_bn128_Fq2::non_residue =
        alt_bn128_Fq("218882428718392752222464057452572750886963111572978236626"
                     "89037894645226208582");
    alt_bn128_Fq2::nqr = alt_bn128_Fq2(alt_bn128_Fq("2"), alt_bn128_Fq("1"));
    alt_bn128_Fq2::nqr_to_t = alt_bn128_Fq2(
        alt_bn128_Fq("503350371626262426731249255837998268717520073493487759859"
                     "9011485707452665730"),
        alt_bn128_Fq("314498342015008975724433667930697407966947188435857772134"
                     "235984660852259084"));
    alt_bn128_Fq2::Frobenius_coeffs_c1[0] = alt_bn128_Fq("1");
    alt_bn128_Fq2::Frobenius_coeffs_c1[1] =
        alt_bn128_Fq("218882428718392752222464057452572750886963111572978236626"
                     "89037894645226208582");
    alt_bn128_Fq2::static_init();

    /* parameters for Fq6 */
    alt_bn128_Fq6::non_residue =
        alt_bn128_Fq2(alt_bn128_Fq("9"), alt_bn128_Fq("1"));
    alt_bn128_Fq6::Frobenius_coeffs_c1[0] =
        alt_bn128_Fq2(alt_bn128_Fq("1"), alt_bn128_Fq("0"));
    alt_bn128_Fq6::Frobenius_coeffs_c1[1] = alt_bn128_Fq2(
        alt_bn128_Fq("215754636382808430103983242694308260992690442743472168272"
                     "12613867836435027261"),
        alt_bn128_Fq("103076015958737097001522842738161122640692301306164367556"
                     "25194854815875713954"));
    alt_bn128_Fq6::Frobenius_coeffs_c1[2] = alt_bn128_Fq2(
        alt_bn128_Fq("218882428718392752200424452601091531672777074144720616417"
                     "14758635765020556616"),
        alt_bn128_Fq("0"));
    alt_bn128_Fq6::Frobenius_coeffs_c1[3] = alt_bn128_Fq2(
        alt_bn128_Fq("377200088191985377643369518671385823900907359381719577177"
                     "3381919316419345261"),
        alt_bn128_Fq("223659549596724518828170124820318179512106890260586122785"
                     "5261137820944008926"));
    alt_bn128_Fq6::Frobenius_coeffs_c1[4] = alt_bn128_Fq2(
        alt_bn128_Fq(
            "2203960485148121921418603742825762020974279258880205651966"),
        alt_bn128_Fq("0"));
    alt_bn128_Fq6::Frobenius_coeffs_c1[5] = alt_bn128_Fq2(
        alt_bn128_Fq("184290212234778536576607920343698658391145044464312347263"
                     "92080002137598044644"),
        alt_bn128_Fq("934404577999832033381242022323798102950601212407552567920"
                     "8581902008406485703"));
    alt_bn128_Fq6::Frobenius_coeffs_c2[0] =
        alt_bn128_Fq2(alt_bn128_Fq("1"), alt_bn128_Fq("0"));
    alt_bn128_Fq6::Frobenius_coeffs_c2[1] = alt_bn128_Fq2(
        alt_bn128_Fq("258191134446700933526731111546880309955166560507619674086"
                     "7805258568234346338"),
        alt_bn128_Fq("199377569717756479879959321699293419943146406529649494483"
                     "13374472400716661030"));
    alt_bn128_Fq6::Frobenius_coeffs_c2[2] = alt_bn128_Fq2(
        alt_bn128_Fq(
            "2203960485148121921418603742825762020974279258880205651966"),
        alt_bn128_Fq("0"));
    alt_bn128_Fq6::Frobenius_coeffs_c2[3] = alt_bn128_Fq2(
        alt_bn128_Fq("532447920244990354272678339550621448192825776240064327978"
                     "0343368557297135718"),
        alt_bn128_Fq("162089003807376930849194951273343879813937264198568887999"
                     "17914180988844123039"));
    alt_bn128_Fq6::Frobenius_coeffs_c2[4] = alt_bn128_Fq2(
        alt_bn128_Fq("218882428718392752200424452601091531672777074144720616417"
                     "14758635765020556616"),
        alt_bn128_Fq("0"));
    alt_bn128_Fq6::Frobenius_coeffs_c2[5] = alt_bn128_Fq2(
        alt_bn128_Fq("139818523249223623442523112342822575072163877898209836420"
                     "40889267519694726527"),
        alt_bn128_Fq("762982839116520937157738419325082020168425524177380907714"
                     "6787135900891633097"));

    /* parameters for Fq12 */

    alt_bn128_Fq12::non_residue =
        alt_bn128_Fq2(alt_bn128_Fq("9"), alt_bn128_Fq("1"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[0] =
        alt_bn128_Fq2(alt_bn128_Fq("1"), alt_bn128_Fq("0"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[1] = alt_bn128_Fq2(
        alt_bn128_Fq("837611886576382149658397386762636409258990606586829877690"
                     "9617916018768340080"),
        alt_bn128_Fq("164698233230778082238891372411765367990092866461081699356"
                     "59301613961712198316"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[2] = alt_bn128_Fq2(
        alt_bn128_Fq("218882428718392752200424452601091531672777074144720616417"
                     "14758635765020556617"),
        alt_bn128_Fq("0"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[3] = alt_bn128_Fq2(
        alt_bn128_Fq("116974234963581543048257829225847253129123834411595050387"
                     "94027105778954184319"),
        alt_bn128_Fq("303847389135065887422783454877609941456349188919719272345"
                     "083954437860409601"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[4] = alt_bn128_Fq2(
        alt_bn128_Fq("218882428718392752200424452601091531672777074144720616417"
                     "14758635765020556616"),
        alt_bn128_Fq("0"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[5] = alt_bn128_Fq2(
        alt_bn128_Fq("332130463059433280824180905495836122032247737529120626188"
                     "4409189760185844239"),
        alt_bn128_Fq("572226693789653288578005195895834823114337370010937299937"
                     "4820235121374419868"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[6] = alt_bn128_Fq2(
        alt_bn128_Fq("218882428718392752222464057452572750886963111572978236626"
                     "89037894645226208582"),
        alt_bn128_Fq("0"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[7] = alt_bn128_Fq2(
        alt_bn128_Fq("135121240060754537256624318776309109961064050914295248857"
                     "79419978626457868503"),
        alt_bn128_Fq("541841954876146699835726850408073828968702451118965372702"
                     "9736280683514010267"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[8] = alt_bn128_Fq2(
        alt_bn128_Fq(
            "2203960485148121921418603742825762020974279258880205651966"),
        alt_bn128_Fq("0"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[9] = alt_bn128_Fq2(
        alt_bn128_Fq("101908193754811209174206228226725497757839277161383186238"
                     "95010788866272024264"),
        alt_bn128_Fq("215843954827042093348236222903796651472399619683781043903"
                     "43953940207365798982"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[10] = alt_bn128_Fq2(
        alt_bn128_Fq(
            "2203960485148121921418603742825762020974279258880205651967"),
        alt_bn128_Fq("0"));
    alt_bn128_Fq12::Frobenius_coeffs_c1[11] = alt_bn128_Fq2(
        alt_bn128_Fq("185669382412449424140045966902989138683738337820066174008"
                     "04628704885040364344"),
        alt_bn128_Fq("161659759339427423364663537862989268575529374571884506633"
                     "14217659523851788715"));

    /* choice of short Weierstrass curve and its twist */

    alt_bn128_coeff_b = alt_bn128_Fq("3");
    alt_bn128_twist = alt_bn128_Fq2(alt_bn128_Fq("9"), alt_bn128_Fq("1"));
    alt_bn128_twist_coeff_b = alt_bn128_coeff_b * alt_bn128_twist.inverse();
    alt_bn128_twist_mul_by_b_c0 =
        alt_bn128_coeff_b * alt_bn128_Fq2::non_residue;
    alt_bn128_twist_mul_by_b_c1 =
        alt_bn128_coeff_b * alt_bn128_Fq2::non_residue;
    alt_bn128_twist_mul_by_q_X = alt_bn128_Fq2(
        alt_bn128_Fq("215754636382808430103983242694308260992690442743472168272"
                     "12613867836435027261"),
        alt_bn128_Fq("103076015958737097001522842738161122640692301306164367556"
                     "25194854815875713954"));
    alt_bn128_twist_mul_by_q_Y = alt_bn128_Fq2(
        alt_bn128_Fq("282156518219453684454815956169350265935961718524412036707"
                     "8079554186484126554"),
        alt_bn128_Fq("350584376791155637868703030998424884554024350989925964101"
                     "3678093033130930403"));

    /* choice of group G1 */

    // Identities
    alt_bn128_G1::G1_zero = alt_bn128_G1(
        alt_bn128_Fq::zero(), alt_bn128_Fq::one(), alt_bn128_Fq::zero());
    alt_bn128_G1::G1_one =
        alt_bn128_G1(alt_bn128_Fq("1"), alt_bn128_Fq("2"), alt_bn128_Fq::one());

    // Curve coeffs
    alt_bn128_G1::coeff_a = alt_bn128_Fq::zero();
    alt_bn128_G1::coeff_b = alt_bn128_coeff_b;

    // Cofactor
    alt_bn128_G1::h = bigint<alt_bn128_G1::h_limbs>("1");

    // WNAF
    alt_bn128_G1::wnaf_window_table.resize(0);
    alt_bn128_G1::wnaf_window_table.push_back(11);
    alt_bn128_G1::wnaf_window_table.push_back(24);
    alt_bn128_G1::wnaf_window_table.push_back(60);
    alt_bn128_G1::wnaf_window_table.push_back(127);

    alt_bn128_G1::fixed_base_exp_window_table.resize(0);
    // window 1 is unbeaten in [-inf, 4.99]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(1);
    // window 2 is unbeaten in [4.99, 10.99]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(5);
    // window 3 is unbeaten in [10.99, 32.29]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(11);
    // window 4 is unbeaten in [32.29, 55.23]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(32);
    // window 5 is unbeaten in [55.23, 162.03]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(55);
    // window 6 is unbeaten in [162.03, 360.15]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(162);
    // window 7 is unbeaten in [360.15, 815.44]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(360);
    // window 8 is unbeaten in [815.44, 2373.07]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(815);
    // window 9 is unbeaten in [2373.07, 6977.75]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(2373);
    // window 10 is unbeaten in [6977.75, 7122.23]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(6978);
    // window 11 is unbeaten in [7122.23, 57818.46]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(7122);
    // window 12 is never the best
    alt_bn128_G1::fixed_base_exp_window_table.push_back(0);
    // window 13 is unbeaten in [57818.46, 169679.14]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(57818);
    // window 14 is never the best
    alt_bn128_G1::fixed_base_exp_window_table.push_back(0);
    // window 15 is unbeaten in [169679.14, 439758.91]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(169679);
    // window 16 is unbeaten in [439758.91, 936073.41]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(439759);
    // window 17 is unbeaten in [936073.41, 4666554.74]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(936073);
    // window 18 is never the best
    alt_bn128_G1::fixed_base_exp_window_table.push_back(0);
    // window 19 is unbeaten in [4666554.74, 7580404.42]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(4666555);
    // window 20 is unbeaten in [7580404.42, 34552892.20]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(7580404);
    // window 21 is never the best
    alt_bn128_G1::fixed_base_exp_window_table.push_back(0);
    // window 22 is unbeaten in [34552892.20, inf]
    alt_bn128_G1::fixed_base_exp_window_table.push_back(34552892);

    /* choice of group G2 */

    // Identities
    alt_bn128_G2::G2_zero = alt_bn128_G2(
        alt_bn128_Fq2::zero(), alt_bn128_Fq2::one(), alt_bn128_Fq2::zero());

    alt_bn128_G2::G2_one = alt_bn128_G2(
        alt_bn128_Fq2(
            alt_bn128_Fq("10857046999023057135944570762232829481370756359578518"
                         "086990519993285655852781"),
            alt_bn128_Fq("11559732032986387107991004021392285783925812861821192"
                         "530917403151452391805634")),
        alt_bn128_Fq2(
            alt_bn128_Fq("84956539231234314176049732474892724384181905872636001"
                         "48770280649306958101930"),
            alt_bn128_Fq("40823678758634336813322034031454355683168513275934012"
                         "08105741076214120093531")),
        alt_bn128_Fq2::one());

    // Curve coeffs
    alt_bn128_G2::coeff_a = alt_bn128_Fq2::zero();
    alt_bn128_G2::coeff_b = alt_bn128_twist_coeff_b;

    // Cofactor
    // [Sage excerpt]
    // u = 4965661367192848881
    // h2 = (36 * u^4) + (36 * u^3) + (30 * u^2) + 6*u + 1; h2
    // #
    // 21888242871839275222246405745257275088844257914179612981679871602714643921549
    alt_bn128_G2::h =
        bigint<alt_bn128_G2::h_limbs>("2188824287183927522224640574525727508884"
                                      "4257914179612981679871602714643921549");

    // WNAF
    alt_bn128_G2::wnaf_window_table.resize(0);
    alt_bn128_G2::wnaf_window_table.push_back(5);
    alt_bn128_G2::wnaf_window_table.push_back(15);
    alt_bn128_G2::wnaf_window_table.push_back(39);
    alt_bn128_G2::wnaf_window_table.push_back(109);

    alt_bn128_G2::fixed_base_exp_window_table.resize(0);
    // window 1 is unbeaten in [-inf, 5.10]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(1);
    // window 2 is unbeaten in [5.10, 10.43]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(5);
    // window 3 is unbeaten in [10.43, 25.28]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(10);
    // window 4 is unbeaten in [25.28, 59.00]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(25);
    // window 5 is unbeaten in [59.00, 154.03]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(59);
    // window 6 is unbeaten in [154.03, 334.25]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(154);
    // window 7 is unbeaten in [334.25, 742.58]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(334);
    // window 8 is unbeaten in [742.58, 2034.40]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(743);
    // window 9 is unbeaten in [2034.40, 4987.56]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(2034);
    // window 10 is unbeaten in [4987.56, 8888.27]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(4988);
    // window 11 is unbeaten in [8888.27, 26271.13]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(8888);
    // window 12 is unbeaten in [26271.13, 39768.20]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(26271);
    // window 13 is unbeaten in [39768.20, 106275.75]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(39768);
    // window 14 is unbeaten in [106275.75, 141703.40]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(106276);
    // window 15 is unbeaten in [141703.40, 462422.97]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(141703);
    // window 16 is unbeaten in [462422.97, 926871.84]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(462423);
    // window 17 is unbeaten in [926871.84, 4873049.17]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(926872);
    // window 18 is never the best
    alt_bn128_G2::fixed_base_exp_window_table.push_back(0);
    // window 19 is unbeaten in [4873049.17, 5706707.88]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(4873049);
    // window 20 is unbeaten in [5706707.88, 31673814.95]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(5706708);
    // window 21 is never the best
    alt_bn128_G2::fixed_base_exp_window_table.push_back(0);
    // window 22 is unbeaten in [31673814.95, inf]
    alt_bn128_G2::fixed_base_exp_window_table.push_back(31673815);

    /* pairing parameters */

    alt_bn128_ate_loop_count = bigint_q("29793968203157093288");
    alt_bn128_ate_is_loop_count_neg = false;
    alt_bn128_final_exponent = bigint<12 * alt_bn128_q_limbs>(
        "5524842336132240963126171267831731470973821037629576541888827343141969"
        "1083990754121397450276154062981700960854865468034362770115382944674781"
        "0907373256841551006201639677726139946029199968412598804882391702273019"
        "0836532720475663165843655597764930274954582383739028759376599435048732"
        "2055416155052592630230333174746351564471187665317712957830319109590090"
        "9191624817826566688241804408081892785725967931714097716709526092261278"
        "0719525601711114440720492291235650574837501614600243533462841672824527"
        "5621766233552881351913980829117053907212538123081572907154486160275093"
        "6964829313608137325426383735122175229541155376346436093930287402089517"
        "4269731789175697133847480818272554725769374714961957527271882614356332"
        "7123871013173609629979816885292554054934233077527987700678435480142224"
        "972257378356168517961881648003769500551542616236243107224563832474448"
        "0");
    alt_bn128_final_exponent_z = bigint_q("4965661367192848881");
    alt_bn128_final_exponent_is_z_neg = false;
}
} // namespace libff
