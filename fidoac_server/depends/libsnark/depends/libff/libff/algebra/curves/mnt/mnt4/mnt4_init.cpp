/** @file
 *****************************************************************************

 Implementation of interfaces for initializing MNT4.

 See mnt4_init.hpp .

 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include <libff/algebra/curves/mnt/mnt4/mnt4_g1.hpp>
#include <libff/algebra/curves/mnt/mnt4/mnt4_g2.hpp>
#include <libff/algebra/curves/mnt/mnt4/mnt4_init.hpp>

namespace libff
{

// bigint<mnt4_r_limbs> mnt4_modulus_r = mnt46_modulus_A;
// bigint<mnt4_q_limbs> mnt4_modulus_q = mnt46_modulus_B;

mnt4_Fq2 mnt4_twist;
mnt4_Fq2 mnt4_twist_coeff_a;
mnt4_Fq2 mnt4_twist_coeff_b;
mnt4_Fq mnt4_twist_mul_by_a_c0;
mnt4_Fq mnt4_twist_mul_by_a_c1;
mnt4_Fq mnt4_twist_mul_by_b_c0;
mnt4_Fq mnt4_twist_mul_by_b_c1;
mnt4_Fq mnt4_twist_mul_by_q_X;
mnt4_Fq mnt4_twist_mul_by_q_Y;

bigint<mnt4_q_limbs> mnt4_ate_loop_count;
bool mnt4_ate_is_loop_count_neg;
bigint<4 * mnt4_q_limbs> mnt4_final_exponent;
bigint<mnt4_q_limbs> mnt4_final_exponent_last_chunk_abs_of_w0;
bool mnt4_final_exponent_last_chunk_is_w0_neg;
bigint<mnt4_q_limbs> mnt4_final_exponent_last_chunk_w1;

void init_mnt4_params()
{
    typedef bigint<mnt4_r_limbs> bigint_r;
    typedef bigint<mnt4_q_limbs> bigint_q;

    assert(
        sizeof(mp_limb_t) == 8 ||
        sizeof(mp_limb_t) == 4); // Montgomery assumes this

    /* parameters for scalar field Fr */
    mnt4_modulus_r = bigint_r("475922286169261325753349249653048451545124878552"
                              "823515553267735739164647307408490559963137");
    assert(mnt4_Fr::modulus_is_valid());
    if (sizeof(mp_limb_t) == 8) {
        mnt4_Fr::Rsquared =
            bigint_r("163983144722506446826715124368972380525894397127205577781"
                     "234305496325861831001705438796139");
        mnt4_Fr::Rcubed =
            bigint_r("207236281459091063710247635236340312578688659363066707916"
                     "716212805695955118593239854980171");
        mnt4_Fr::inv = 0xbb4334a3ffffffff;
    }
    if (sizeof(mp_limb_t) == 4) {
        mnt4_Fr::Rsquared =
            bigint_r("163983144722506446826715124368972380525894397127205577781"
                     "234305496325861831001705438796139");
        mnt4_Fr::Rcubed =
            bigint_r("207236281459091063710247635236340312578688659363066707916"
                     "716212805695955118593239854980171");
        mnt4_Fr::inv = 0xffffffff;
    }
    mnt4_Fr::num_bits = 298;
    mnt4_Fr::euler = bigint_r("237961143084630662876674624826524225772562439276"
                              "411757776633867869582323653704245279981568");
    mnt4_Fr::s = 34;
    mnt4_Fr::t = bigint_r("2770232305450256248897344628657729199302411164115319"
                          "9339359284829066871159442729");
    mnt4_Fr::t_minus_1_over_2 =
        bigint_r("1385116152725128124448672314328864599651205582057659966967964"
                 "2414533435579721364");
    mnt4_Fr::multiplicative_generator = mnt4_Fr("10");
    mnt4_Fr::root_of_unity =
        mnt4_Fr("12063881782691317345876882948569009984537700803089161801010977"
                "2937363554409782252579816313");
    mnt4_Fr::nqr = mnt4_Fr("5");
    mnt4_Fr::nqr_to_t = mnt4_Fr("4062206042430904010564294587302981459372625525"
                                "08985450684842547562990900634752279902740880");
    mnt4_Fr::static_init();

    /* parameters for base field Fq */
    mnt4_modulus_q = bigint_q("475922286169261325753349249653048451545124879242"
                              "694725395555128576210262817955800483758081");
    assert(mnt4_Fq::modulus_is_valid());
    if (sizeof(mp_limb_t) == 8) {
        mnt4_Fq::Rsquared =
            bigint_q("273000478523237720910981655601160860640083126627235719712"
                     "980612296263966512828033847775776");
        mnt4_Fq::Rcubed =
            bigint_q("427298980065529822574935274648041073124704261331681436071"
                     "990730954930769758106792920349077");
        mnt4_Fq::inv = 0xb071a1b67165ffff;
    }
    if (sizeof(mp_limb_t) == 4) {
        mnt4_Fq::Rsquared =
            bigint_q("273000478523237720910981655601160860640083126627235719712"
                     "980612296263966512828033847775776");
        mnt4_Fq::Rcubed =
            bigint_q("427298980065529822574935274648041073124704261331681436071"
                     "990730954930769758106792920349077");
        mnt4_Fq::inv = 0x7165ffff;
    }
    mnt4_Fq::num_bits = 298;
    mnt4_Fq::euler = bigint_q("237961143084630662876674624826524225772562439621"
                              "347362697777564288105131408977900241879040");
    mnt4_Fq::s = 17;
    mnt4_Fq::t = bigint_q("3630998887399759870554727551674258816109656366292531"
                          "779446068791017229177993437198515");
    mnt4_Fq::t_minus_1_over_2 =
        bigint_q("1815499443699879935277363775837129408054828183146265889723034"
                 "395508614588996718599257");
    mnt4_Fq::multiplicative_generator = mnt4_Fq("17");
    mnt4_Fq::root_of_unity =
        mnt4_Fq("26470625057180008075806930236965430553012567552126397603405487"
                "8017580902343339784464690243");
    mnt4_Fq::nqr = mnt4_Fq("17");
    mnt4_Fq::nqr_to_t = mnt4_Fq("2647062505718000807580693023696543055301256755"
                                "21263976034054878017580902343339784464690243");
    mnt4_Fq::static_init();

    /* parameters for twist field Fq2 */
    mnt4_Fq2::euler = bigint<2 * mnt4_q_limbs>(
        "1132510112362881350982493452491542308959143818587889181068472142434191"
        "4242292413349746081746824985483306726003898571037009192086083701428188"
        "6963086681184370139950267830740466401280");
    mnt4_Fq2::s = 18;
    mnt4_Fq2::t = bigint<2 * mnt4_q_limbs>(
        "8640366457846689994678447360927904578850889729216683815524842395280391"
        "1150302225873917249655341991297200973540485924049447571457547770905980"
        "6542104196047745818712370534824115");
    mnt4_Fq2::t_minus_1_over_2 = bigint<2 * mnt4_q_limbs>(
        "4320183228923344997339223680463952289425444864608341907762421197640195"
        "5575151112936958624827670995648600486770242962024723785728773885452990"
        "3271052098023872909356185267412057");
    mnt4_Fq2::non_residue = mnt4_Fq("17");
    mnt4_Fq2::nqr = mnt4_Fq2(mnt4_Fq("8"), mnt4_Fq("1"));
    mnt4_Fq2::nqr_to_t = mnt4_Fq2(
        mnt4_Fq("0"),
        mnt4_Fq("29402818985595053196743631544512156561638230562612542604956687"
                "802791427330205135130967658"));
    mnt4_Fq2::Frobenius_coeffs_c1[0] = mnt4_Fq("1");
    mnt4_Fq2::Frobenius_coeffs_c1[1] =
        mnt4_Fq("47592228616926132575334924965304845154512487924269472539555512"
                "8576210262817955800483758080");
    mnt4_Fq2::static_init();

    /* parameters for Fq4 */
    mnt4_Fq4::non_residue = mnt4_Fq("17");
    mnt4_Fq4::Frobenius_coeffs_c1[0] = mnt4_Fq("1");
    mnt4_Fq4::Frobenius_coeffs_c1[1] =
        mnt4_Fq("76841632454535016156213515524733370693010820609768050046250116"
                "94147890954040864167002308");
    mnt4_Fq4::Frobenius_coeffs_c1[2] =
        mnt4_Fq("47592228616926132575334924965304845154512487924269472539555512"
                "8576210262817955800483758080");
    mnt4_Fq4::Frobenius_coeffs_c1[3] =
        mnt4_Fq("46823812292380782413772789810057511447582379718171792039093011"
                "6882062371863914936316755773");

    /* choice of short Weierstrass curve and its twist */
    mnt4_G1::coeff_a = mnt4_Fq("2");
    mnt4_G1::coeff_b = mnt4_Fq("42389453652668417828941601153388824002931810367"
                               "3896002803341544124054745019340795360841685");
    mnt4_twist = mnt4_Fq2(mnt4_Fq::zero(), mnt4_Fq::one());
    mnt4_twist_coeff_a =
        mnt4_Fq2(mnt4_G1::coeff_a * mnt4_Fq2::non_residue, mnt4_Fq::zero());
    mnt4_twist_coeff_b =
        mnt4_Fq2(mnt4_Fq::zero(), mnt4_G1::coeff_b * mnt4_Fq2::non_residue);
    mnt4_G2::twist = mnt4_twist;
    mnt4_G2::coeff_a = mnt4_twist_coeff_a;
    mnt4_G2::coeff_b = mnt4_twist_coeff_b;
    mnt4_twist_mul_by_a_c0 = mnt4_G1::coeff_a * mnt4_Fq2::non_residue;
    mnt4_twist_mul_by_a_c1 = mnt4_G1::coeff_a * mnt4_Fq2::non_residue;
    mnt4_twist_mul_by_b_c0 = mnt4_G1::coeff_b * mnt4_Fq2::non_residue.squared();
    mnt4_twist_mul_by_b_c1 = mnt4_G1::coeff_b * mnt4_Fq2::non_residue;
    mnt4_twist_mul_by_q_X =
        mnt4_Fq("47592228616926132575334924965304845154512487924269472539555512"
                "8576210262817955800483758080");
    mnt4_twist_mul_by_q_Y =
        mnt4_Fq("76841632454535016156213515524733370693010820609768050046250116"
                "94147890954040864167002308");

    /* choice of group G1 */
    // Identities
    mnt4_G1::G1_zero =
        mnt4_G1(mnt4_Fq::zero(), mnt4_Fq::one(), mnt4_Fq::zero());
    mnt4_G1::G1_one = mnt4_G1(
        mnt4_Fq("60760244141852568949126569781626075788424196370144486719385562"
                "369396875346601926534016838"),
        mnt4_Fq("36373285070258297826390277081514578445974772235707184397110767"
                "4179038674942891694705904306"),
        mnt4_Fq::one());

    // Cofactor
    mnt4_G1::h = bigint<mnt4_G1::h_limbs>("1");

    // WNAF
    mnt4_G1::wnaf_window_table.resize(0);
    mnt4_G1::wnaf_window_table.push_back(11);
    mnt4_G1::wnaf_window_table.push_back(24);
    mnt4_G1::wnaf_window_table.push_back(60);
    mnt4_G1::wnaf_window_table.push_back(127);

    mnt4_G1::fixed_base_exp_window_table.resize(0);
    // window 1 is unbeaten in [-inf, 5.09]
    mnt4_G1::fixed_base_exp_window_table.push_back(1);
    // window 2 is unbeaten in [5.09, 9.64]
    mnt4_G1::fixed_base_exp_window_table.push_back(5);
    // window 3 is unbeaten in [9.64, 24.79]
    mnt4_G1::fixed_base_exp_window_table.push_back(10);
    // window 4 is unbeaten in [24.79, 60.29]
    mnt4_G1::fixed_base_exp_window_table.push_back(25);
    // window 5 is unbeaten in [60.29, 144.37]
    mnt4_G1::fixed_base_exp_window_table.push_back(60);
    // window 6 is unbeaten in [144.37, 344.90]
    mnt4_G1::fixed_base_exp_window_table.push_back(144);
    // window 7 is unbeaten in [344.90, 855.00]
    mnt4_G1::fixed_base_exp_window_table.push_back(345);
    // window 8 is unbeaten in [855.00, 1804.62]
    mnt4_G1::fixed_base_exp_window_table.push_back(855);
    // window 9 is unbeaten in [1804.62, 3912.30]
    mnt4_G1::fixed_base_exp_window_table.push_back(1805);
    // window 10 is unbeaten in [3912.30, 11264.50]
    mnt4_G1::fixed_base_exp_window_table.push_back(3912);
    // window 11 is unbeaten in [11264.50, 27897.51]
    mnt4_G1::fixed_base_exp_window_table.push_back(11265);
    // window 12 is unbeaten in [27897.51, 57596.79]
    mnt4_G1::fixed_base_exp_window_table.push_back(27898);
    // window 13 is unbeaten in [57596.79, 145298.71]
    mnt4_G1::fixed_base_exp_window_table.push_back(57597);
    // window 14 is unbeaten in [145298.71, 157204.59]
    mnt4_G1::fixed_base_exp_window_table.push_back(145299);
    // window 15 is unbeaten in [157204.59, 601600.62]
    mnt4_G1::fixed_base_exp_window_table.push_back(157205);
    // window 16 is unbeaten in [601600.62, 1107377.25]
    mnt4_G1::fixed_base_exp_window_table.push_back(601601);
    // window 17 is unbeaten in [1107377.25, 1789646.95]
    mnt4_G1::fixed_base_exp_window_table.push_back(1107377);
    // window 18 is unbeaten in [1789646.95, 4392626.92]
    mnt4_G1::fixed_base_exp_window_table.push_back(1789647);
    // window 19 is unbeaten in [4392626.92, 8221210.60]
    mnt4_G1::fixed_base_exp_window_table.push_back(4392627);
    // window 20 is unbeaten in [8221210.60, 42363731.19]
    mnt4_G1::fixed_base_exp_window_table.push_back(8221211);
    // window 21 is never the best
    mnt4_G1::fixed_base_exp_window_table.push_back(0);
    // window 22 is unbeaten in [42363731.19, inf]
    mnt4_G1::fixed_base_exp_window_table.push_back(42363731);

    /* choice of group G2 */
    // Identities
    mnt4_G2::G2_zero =
        mnt4_G2(mnt4_Fq2::zero(), mnt4_Fq2::one(), mnt4_Fq2::zero());
    mnt4_G2::G2_one = mnt4_G2(
        mnt4_Fq2(
            mnt4_Fq("4383749262193500998549191000778096818427835091637909918478"
                    "67546339851681564223481322252708"),
            mnt4_Fq("3762095361550048011093551436092327860546447645971239327767"
                    "9280819942849043649216370485641")),
        mnt4_Fq2(
            mnt4_Fq("3743740900852896826835252103493693184297354644137066311854"
                    "3015118291998305624025037512482"),
            mnt4_Fq("4246214795988938826723931903374206805975846958923171976461"
                    "13820787463109735345923009077489")),
        mnt4_Fq2::one());

    // Cofactor
    mnt4_G2::h = bigint<mnt4_G2::h_limbs>(
        "4759222861692613257533492496530484515451248799325659352378425214132558"
        "78328503110407553025");

    // WNAF
    mnt4_G2::wnaf_window_table.resize(0);
    mnt4_G2::wnaf_window_table.push_back(5);
    mnt4_G2::wnaf_window_table.push_back(15);
    mnt4_G2::wnaf_window_table.push_back(39);
    mnt4_G2::wnaf_window_table.push_back(109);

    mnt4_G2::fixed_base_exp_window_table.resize(0);
    // window 1 is unbeaten in [-inf, 4.17]
    mnt4_G2::fixed_base_exp_window_table.push_back(1);
    // window 2 is unbeaten in [4.17, 10.12]
    mnt4_G2::fixed_base_exp_window_table.push_back(4);
    // window 3 is unbeaten in [10.12, 24.65]
    mnt4_G2::fixed_base_exp_window_table.push_back(10);
    // window 4 is unbeaten in [24.65, 60.03]
    mnt4_G2::fixed_base_exp_window_table.push_back(25);
    // window 5 is unbeaten in [60.03, 143.16]
    mnt4_G2::fixed_base_exp_window_table.push_back(60);
    // window 6 is unbeaten in [143.16, 344.73]
    mnt4_G2::fixed_base_exp_window_table.push_back(143);
    // window 7 is unbeaten in [344.73, 821.24]
    mnt4_G2::fixed_base_exp_window_table.push_back(345);
    // window 8 is unbeaten in [821.24, 1793.92]
    mnt4_G2::fixed_base_exp_window_table.push_back(821);
    // window 9 is unbeaten in [1793.92, 3919.59]
    mnt4_G2::fixed_base_exp_window_table.push_back(1794);
    // window 10 is unbeaten in [3919.59, 11301.46]
    mnt4_G2::fixed_base_exp_window_table.push_back(3920);
    // window 11 is unbeaten in [11301.46, 18960.09]
    mnt4_G2::fixed_base_exp_window_table.push_back(11301);
    // window 12 is unbeaten in [18960.09, 44198.62]
    mnt4_G2::fixed_base_exp_window_table.push_back(18960);
    // window 13 is unbeaten in [44198.62, 150799.57]
    mnt4_G2::fixed_base_exp_window_table.push_back(44199);
    // window 14 is never the best
    mnt4_G2::fixed_base_exp_window_table.push_back(0);
    // window 15 is unbeaten in [150799.57, 548694.81]
    mnt4_G2::fixed_base_exp_window_table.push_back(150800);
    // window 16 is unbeaten in [548694.81, 1051769.08]
    mnt4_G2::fixed_base_exp_window_table.push_back(548695);
    // window 17 is unbeaten in [1051769.08, 2023925.59]
    mnt4_G2::fixed_base_exp_window_table.push_back(1051769);
    // window 18 is unbeaten in [2023925.59, 3787108.68]
    mnt4_G2::fixed_base_exp_window_table.push_back(2023926);
    // window 19 is unbeaten in [3787108.68, 7107480.30]
    mnt4_G2::fixed_base_exp_window_table.push_back(3787109);
    // window 20 is unbeaten in [7107480.30, 38760027.14]
    mnt4_G2::fixed_base_exp_window_table.push_back(7107480);
    // window 21 is never the best
    mnt4_G2::fixed_base_exp_window_table.push_back(0);
    // window 22 is unbeaten in [38760027.14, inf]
    mnt4_G2::fixed_base_exp_window_table.push_back(38760027);

    /* pairing parameters */
    mnt4_ate_loop_count =
        bigint_q("689871209842287392837045615510547309923794944");
    mnt4_ate_is_loop_count_neg = false;
    mnt4_final_exponent = bigint<4 * mnt4_q_limbs>(
        "1077973603571099034307944903095920722789277838030318543579109081219034"
        "3983877286149717711641082558674308976086994539461051191727497797155906"
        "2689561855016270594656570874331111995170645233717143416875749097203441"
        "437192367065467706065411650403684877366879441766585988546560");
    mnt4_final_exponent_last_chunk_abs_of_w0 =
        bigint_q("689871209842287392837045615510547309923794945");
    mnt4_final_exponent_last_chunk_is_w0_neg = false;
    mnt4_final_exponent_last_chunk_w1 = bigint_q("1");
}

} // namespace libff
